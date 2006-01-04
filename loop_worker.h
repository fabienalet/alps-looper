/*****************************************************************************
*
* ALPS/looper: multi-cluster quantum Monte Carlo algorithms for spin systems
*
* Copyright (C) 1997-2005 by Synge Todo <wistaria@comp-phys.org>
*
* This software is published under the ALPS Application License; you
* can use, redistribute it and/or modify it under the terms of the
* license, either version 1 or (at your option) any later version.
* 
* You should have received a copy of the ALPS Application License
* along with this software; see the file LICENSE. If not, the license
* is also available from http://alps.comp-phys.org/.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
* FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT 
* SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE 
* FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE, 
* ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
* DEALINGS IN THE SOFTWARE.
*
*****************************************************************************/

#ifndef LOOP_WORKER_H
#define LOOP_WORKER_H

#include "loop_config.h"
#include <looper/model.h>
#include <alps/alea.h>
#include <alps/scheduler.h>

class qmc_worker_base
  : public alps::scheduler::LatticeModelMCRun<loop_config::lattice_graph_t>
{
public:
  typedef alps::scheduler::LatticeModelMCRun<loop_config::lattice_graph_t>
    super_type;
  typedef loop_config::lattice_graph_t             lattice_graph_t;
  typedef loop_config::loop_graph_t                loop_graph_t;
  typedef loop_graph_t::location_t                 location_t;
  typedef looper::virtual_lattice<lattice_graph_t> virtual_lattice;
  typedef looper::graph_chooser<loop_graph_t, super_type::engine_type>
    graph_chooser;
  typedef loop_config::improved_estimator_t        improved_estimator_t;
  typedef loop_config::normal_estimator_t          normal_estimator_t;

  qmc_worker_base(alps::ProcessList const& w, alps::Parameters const& p,
                  int n, bool is_path_integral = true);
  virtual ~qmc_worker_base();

  virtual void dostep() { ++mcs_; }
  bool can_work() const
  { return mcs_ < mcs_therm_ || mcs_ - mcs_therm_ < mcs_sweep_.max(); }
  bool is_thermalized() const { return mcs_ >= mcs_therm_; }
  double work_done() const
  {
    return is_thermalized() ?
      (double(mcs_ - mcs_therm_) / mcs_sweep_.min()) : 0.;
  }
  unsigned int mcs() const { return mcs_; }

  lattice_graph_t const& rgraph() const { return super_type::graph(); }
  lattice_graph_t const& vgraph() const { return vlat_.graph(); }
  virtual_lattice const& vlattice() const { return vlat_; }

  double beta() const { return beta_; }
  double energy_offset() const { return energy_offset_; }

  bool is_frustrated() const { return is_frustrated_; }

  bool is_signed() const { return is_signed_; }
  int sign(boost::call_traits<site_descriptor>::param_type s) const
  { return sign_[s]; }

  bool has_field() const { return !field_.empty(); }
  std::vector<double> const& field() const { return field_; }

  bool use_improved_estimator() const { return use_improved_estimator_; }

  loop_graph_t choose_graph() const { return chooser_.graph(); }
  loop_graph_t choose_diagonal(location_t const& loc, int c) const
  { return chooser_.diagonal(loc, c); }
  loop_graph_t choose_diagonal(location_t const& loc, int c0, int c1) const
  { return chooser_.diagonal(loc, c0, c1); }
  loop_graph_t choose_offdiagonal(location_t const& loc) const
  { return chooser_.offdiagonal(loc); }
  double advance() const { return chooser_.advance(); }
  double total_graph_weight() const { return chooser_.weight(); }

  void accumulate();
  void accumulate(alps::ObservableSet const& m_in, alps::ObservableSet& m_out);

  virtual void save(alps::ODump& dp) const;
  virtual void load(alps::IDump& dp);

private:
  looper::integer_range<unsigned int> mcs_sweep_;
  unsigned int mcs_therm_;
  unsigned int mcs_; // to be dumped/restored

  virtual_lattice vlat_;

  double beta_;
  double energy_offset_;

  bool is_frustrated_;
  bool is_signed_;
  std::vector<int> sign_;
  bool use_improved_estimator_;
  std::vector<double> field_;

  graph_chooser chooser_;
};

template<class QMC> class qmc_worker;

#endif // LOOP_WORKER_H
