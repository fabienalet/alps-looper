/**************************************************************************** 
*
* alps/looper: multi-cluster quantum Monte Carlo algorithm for spin systems
*              in path-integral and SSE representations
*
* $Id: qmc_impl.h 521 2003-11-05 10:37:21Z wistaria $
*
* Copyright (C) 1997-2003 by Synge Todo <wistaria@comp-phys.org>
*
* Permission is hereby granted, free of charge, to any person or organization 
* obtaining a copy of the software covered by this license (the "Software") 
* to use, reproduce, display, distribute, execute, and transmit the Software, 
* and to prepare derivative works of the Software, and to permit others
* to do so for non-commerical academic use, all subject to the following:
*
* The copyright notice in the Software and this entire statement, including 
* the above license grant, this restriction and the following disclaimer, 
* must be included in all copies of the Software, in whole or in part, and 
* all derivative works of the Software, unless such copies or derivative 
* works are solely in the form of machine-executable object code generated by 
* a source language processor.
*
* In any scientific publication based in part or wholly on the Software, the
* use of the Software has to be acknowledged and the publications quoted
* on the web page http://www.alps.org/license/ have to be referenced.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
* FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT 
* SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE 
* FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE, 
* ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
* DEALINGS IN THE SOFTWARE.
*
**************************************************************************/

#ifndef QMC_IMPL_H
#define QMC_IMPL_H

#include <looper/copyright.h>
#include <looper/model.h>
#include <looper/path_integral.h>
#include <looper/sse.h>

#include <alps/alea.h>
#include <alps/scheduler.h>

template<class QMC>
class qmc_worker
{
public:
  typedef QMC                      qmc;
  typedef typename qmc::graph_type graph_type;
  typedef alps::RealObservable     measurement_type;

  template<class G, class MDL>
  qmc_worker(const G& rg, const MDL& model, double beta,
	     alps::ObservableSet& m) :
    param_(rg, model, beta), config_()
  {
    is_bipartite = alps::set_parity(param_.virtual_graph.graph);
    e_offset_ = qmc::energy_offset(param_);

    qmc::initialize(config_, param_);

    // measurements
    m << measurement_type("diagonal energy");
    m << measurement_type("diagonal energy (improved)");
    m << measurement_type("off-diagonal energy");
    m << measurement_type("energy");
    m << measurement_type("uniform magnetization"); ////
    m << measurement_type("uniform susceptibility");
    m << measurement_type("staggered magnetization"); ////
    m << measurement_type("staggered magnetization^2");
    m << measurement_type("staggered susceptibility");
  }
    
  template<class RNG>
  void step(RNG& rng, alps::ObservableSet& m)
  {
    qmc::check_and_resize(config_); // meaningful only for SSE
    qmc::generate_loops(config_, param_, rng);

    // measure improved quantities here
    
    qmc::flip_and_cleanup(config_, param_, rng);
      
    // measure unimproved quantities here
    double ez, exy;
    boost::tie(ez, exy) = qmc::energy(config_, param_);
    ez += e_offset_;
    m.template get<measurement_type>("diagonal energy") << ez;
    m.template get<measurement_type>("off-diagonal energy") << exy;
    m.template get<measurement_type>("energy") << ez + exy;

    double sz = qmc::uniform_sz(config_, param_);
    m.template get<measurement_type>("uniform magnetization") << sz; ////
    m.template get<measurement_type>("uniform susceptibility") <<
      param_.beta * param_.virtual_graph.num_real_vertices * sz * sz;

    double ss = qmc::staggered_sz(config_, param_);
    m.template get<measurement_type>("staggered magnetization") << ss; ////
    m.template get<measurement_type>("staggered magnetization^2") <<
      param_.virtual_graph.num_real_vertices * ss * ss;
    m.template get<measurement_type>("staggered susceptibility") << 
      qmc::staggered_susceptibility(config_, param_);
  }

  static void output_results(std::ostream& os, alps::ObservableSet& m)
  {
    os << m.template get<measurement_type>("diagonal energy").mean() << ' '
       << m.template get<measurement_type>("diagonal energy").error() << ' '
       << m.template get<measurement_type>("off-diagonal energy").mean() << ' '
       << m.template get<measurement_type>("off-diagonal energy").error() << ' '
       << m.template get<measurement_type>("energy").mean() << ' '
       << m.template get<measurement_type>("energy").error() << ' '
       << m.template get<measurement_type>("uniform susceptibility").mean() << ' '
       << m.template get<measurement_type>("uniform susceptibility").error() << ' '
       << m.template get<measurement_type>("staggered magnetization^2").mean() << ' '
       << m.template get<measurement_type>("staggered magnetization^2").error() << ' '
       << m.template get<measurement_type>("staggered susceptibility").mean() << ' '
       << m.template get<measurement_type>("staggered susceptibility").error() << ' ';
  }

  void save(alps::ODump& od) const { config_.save(od); }
  void load(alps::IDump& id) { config_.load(id); }

private:
  typename qmc::parameter_type param_;
  typename qmc::config_type    config_;
  bool                         is_bipartite;
  double                       e_offset_;
};


template<class QMC>
class worker : public alps::scheduler::LatticeModelMCRun<>
{
public:
  typedef alps::scheduler::LatticeModelMCRun<>::graph_type graph_type;

  worker(const alps::ProcessList& w, const alps::Parameters& p, int n) :
    alps::scheduler::LatticeModelMCRun<>(w, p, n),
    mdl_(p, graph(), simple_operators(), model()),
    mcs_(0), therm_(static_cast<unsigned int>(p["thermalization"])), 
    total_(static_cast<unsigned int>(p["MCS"])),
    qmc_(graph(), mdl_, 1.0 / static_cast<double>(p["temperature"]),
	 measurements) {}
  virtual ~worker() {}
    
  virtual void dostep() {
    ++mcs_;
    qmc_.step(random_01, measurements);
  }
  bool is_thermalized() const { return mcs_ >= therm_; }
  virtual double work_done() const {
    if (is_thermalized()) {
      return double(mcs_ - therm_) / (total_ - therm_);
    } else {
      return 0.;
    }
  }
  
  virtual void save(alps::ODump& od) const {
    od << mcs_;
    qmc_.save(od);
  }
  virtual void load(alps::IDump& id) {
    id >> mcs_;
    qmc_.load(id);
    if (where.empty()) measurements.compact();
  }

private:
  typename QMC::model_type mdl_;
  unsigned int mcs_, therm_, total_;
  qmc_worker<QMC> qmc_;
};
  
class factory : public alps::scheduler::Factory
{
  alps::scheduler::MCSimulation* make_task(const alps::ProcessList& w,
    const boost::filesystem::path& fn) const
  {
    return new alps::scheduler::MCSimulation(w, fn);
  }
  alps::scheduler::MCSimulation* make_task(const alps::ProcessList& w,
    const boost::filesystem::path& fn, const alps::Parameters&) const
  {
    return new alps::scheduler::MCSimulation(w, fn);
  }

  alps::scheduler::MCRun* make_worker(const alps::ProcessList& w,
				      const alps::Parameters& p, int n) const
  {
    if (!p.defined("representation") ||
	p["representation"] != "SSE") {
      return new worker<looper::path_integral<
                          looper::virtual_graph<looper::parity_graph_type>,
                          looper::xxz_model> >(w, p, n);
    } else {
      return new worker<looper::sse<
                          looper::virtual_graph<looper::parity_graph_type>,
	                  looper::xxz_model> >(w, p, n);
    }
  }

  void print_copyright(std::ostream& os) const
  { looper::print_copyright(os); }
};

#endif // QMC_IMPL_H
