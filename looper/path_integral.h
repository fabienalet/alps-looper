/**************************************************************************** 
*
* alps/looper: multi-cluster quantum Monte Carlo algorithm for spin systems
*              in path-integral and SSE representations
*
* $Id: path_integral.h 476 2003-10-29 10:16:12Z wistaria $
*
* Copyright (C) 1997-2003 by Synge Todo <wistaria@comp-phys.org>,
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

#ifndef LOOPER_PATH_INTEGRAL_H
#define LOOPER_PATH_INTEGRAL_H

#include <looper/amida.h>
#include <looper/fill_duration.h>
#include <looper/node.h>
#include <looper/permutation.h>
#include <looper/union_find.h>
#include <looper/virtual_graph.h>
#include <looper/weight.h>

#include <boost/integer_traits.hpp>
#include <boost/random/uniform_smallint.hpp>
#include <boost/random/variate_generator.hpp>
#include <boost/throw_exception.hpp>
#include <cmath>
#include <complex>
#include <stdexcept>

namespace looper {

template<bool HasCTime = false> class pi_node;

template<>
class pi_node<false> : public qmc_node
{
protected:
#ifdef M_PI
  BOOST_STATIC_CONSTANT(double, two_pi = (2.0 * M_PI));
#else
  BOOST_STATIC_CONSTANT(double, two_pi = (2.0 * 3.1415926535897932385));
#endif

public:
  typedef qmc_node                base_type;
  typedef double                  time_type;
  typedef std::complex<time_type> ctime_type;
  BOOST_STATIC_CONSTANT(bool, has_ctime = false);
  
  pi_node() : base_type(), time_(0.) {}
  
  time_type time() const { return time_; }
  ctime_type ctime() const { return std::exp(two_pi * time_); }
  void set_time(time_type t) { time_ = t; }
  
  std::ostream& output(std::ostream& os) const {
    return base_type::output(os) << " time = " << time_;
  }
  alps::ODump& save(alps::ODump& od) const {
    return base_type::save(od) << time_;
  }
  alps::IDump& load(alps::IDump& id) {
    return base_type::load(id) >> time_;
  }
  
private:
  time_type time_;
};

template<> 
class pi_node<true> : public pi_node<false>
{
public:
  typedef pi_node<false>          base_type;
  typedef base_type::time_type    time_type;
  typedef std::complex<time_type> ctime_type;
  BOOST_STATIC_CONSTANT(bool, has_ctime = true);
  
  pi_node() : base_type(), ctime_() {}
  
  ctime_type ctime() const { return ctime_; }
  void set_time(time_type t) {
    base_type::set_time(t);
  ctime_ = std::exp(two_pi * t);
  }
  
  std::ostream& output(std::ostream& os) const {
    return base_type::output(os) << " ctime = " << ctime_;
  }
  alps::ODump& save(alps::ODump& od) const { return base_type::save(od); }
  alps::IDump& load(alps::IDump& id) {
    base_type::load(id);
    ctime_ = std::exp(two_pi * time());
    return id;
  }
  
private:
  ctime_type ctime_;
};


template<class G, class M, class W = default_weight> struct path_integral;

template<class G, class M, class W>
struct path_integral<virtual_graph<G>, M, W>
{
  typedef virtual_graph<G>                      vg_type;
  typedef typename virtual_graph<G>::graph_type graph_type;
  typedef M                                     model_type;
  typedef W                                     weight_type;

  typedef typename boost::graph_traits<graph_type>::edge_iterator
                                                    edge_iterator;
  typedef typename boost::graph_traits<graph_type>::edge_descriptor
                                                    edge_descriptor;
  typedef typename boost::graph_traits<graph_type>::vertex_iterator
                                                    vertex_iterator;
  typedef typename boost::graph_traits<graph_type>::vertex_descriptor
                                                    vertex_descriptor;
  
  struct parameter_type
  {
    typedef virtual_graph<G> vg_type;
    typedef typename vg_type::graph_type graph_type;
    typedef typename vg_type::mapping_type mapping_type;
    typedef M model_type;

    parameter_type(const vg_type& vg, const model_type& m, double b)
      : virtual_graph(vg), graph(vg.graph), mapping(vg.mapping), model(m),
	beta(b) {}

    const vg_type&      virtual_graph;
    const graph_type&   graph;
    const mapping_type& mapping;
    const model_type&   model;
    const double        beta;
  };

  template<bool HasCTime = false>
  struct config_type
  {
    typedef amida<pi_node<HasCTime> >        wl_type;
    typedef typename wl_type::iterator       iterator;
    typedef typename wl_type::const_iterator const_iterator;
    typedef typename wl_type::value_type     node_type;

    wl_type wl;
    unsigned int num_loops0;
    unsigned int num_loops;
  };
  
  
  //
  // update functions
  //
  
  // initialize
  template<bool HasCTime>
  static void initialize(config_type<HasCTime>& config, const vg_type& vg)
  {
    config.wl.init(boost::num_vertices(vg.graph));
    
    vertex_iterator vi_end = boost::vertices(vg.graph).second;
    for (vertex_iterator vi = boost::vertices(vg.graph).first;
	 vi != vi_end; ++vi) {
      // all up
      config.wl.series(*vi).first ->conf() = 0;
      config.wl.series(*vi).second->conf() = 0;
      config.wl.series(*vi).first ->set_time(0.);
      config.wl.series(*vi).second->set_time(1.);
    }
  }

  template<bool HasCTime>
  static void initialize(config_type<HasCTime>& config,
			 const parameter_type& p)
  {
    initialize(config, p.virtual_graph);
  }

  template<bool HasCTime, class RNG>
  static void generate_loops(config_type<HasCTime>& config, const vg_type& vg,
			     const model_type& model, double beta,
			     RNG& uniform_01)
  {
    typedef typename config_type<HasCTime>::iterator  iterator;
    typedef typename config_type<HasCTime>::node_type node_type;
    
    //
    // labeling
    //
    
    edge_iterator ei_end = boost::edges(vg.graph).second;
    for (edge_iterator ei = boost::edges(vg.graph).first; ei != ei_end; ++ei) {
    
      int bond = boost::get(edge_index_t(), vg.graph, *ei); // bond index
      
      // setup iterators
      iterator itr0 = config.wl.series(boost::source(*ei, vg.graph)).first;
      iterator itr1 = config.wl.series(boost::target(*ei, vg.graph)).first;
      int c0 = itr0->conf();    
      int c1 = itr1->conf();
      
      // setup bond weight
      weight_type weight(model.bond(boost::get(edge_type_t(), vg.graph, *ei)));
      std::vector<double> trials;
      fill_duration(uniform_01, trials, beta * weight.density());
      
      // iteration up to t = 1
      std::vector<double>::const_iterator ti_end = trials.end();
      for (std::vector<double>::const_iterator ti = trials.begin();
	   ti != ti_end; ++ti) {
	while (itr0->time() < *ti) { 
	  if (itr0->bond() == bond) // labeling existing link
	    itr0->set_old(uniform_01() < weight.p_reflect());
	  if (itr0->is_old()) c0 ^= 1;
	  ++itr0;
	}
	while (itr1->time() < *ti) {
	  if (itr1->is_old()) c1 ^= 1;
	  ++itr1;
	}
	if (uniform_01() < weight.p_accept(c0, c1)) {
	  // insert new link
	  iterator itr_new =
	    config.wl.insert_link_prev(node_type(), itr0, itr1).first;
	  itr_new->set_time(*ti);
	  itr_new->set_bond(boost::get(edge_index_t(), vg.graph, *ei));
	  itr_new->set_new(c0 ^ c1, (uniform_01() < weight.p_freeze()));
	}
      }
      while (!itr0.at_top()) { 
	if (itr0->bond() == bond)	// labeling existing link
	  itr0->set_old(uniform_01() < weight.p_reflect());
	++itr0;
      }
    }
    //// std::cout << "labeling done.\n";
    //// std::cout << "num_links " << config.wl.num_links() << std::endl;
    
    //
    // cluster identification using union-find algorithm
    //
    
    vertex_iterator vi_end = boost::vertices(vg.graph).second;
    for (vertex_iterator vi = boost::vertices(vg.graph).first;
	 vi != vi_end; ++vi) {
      // setup iterators
      iterator itrD = config.wl.series(*vi).first;
      iterator itrU = boost::next(itrD);
      
      // iteration up to t = 1
      for (;; itrD = itrU++) {
	union_find::unify(segment_d(itrU), segment_u(itrD));
	if (itrU.leg() == 0 && itrU->is_frozen()) // frozen link
	  union_find::unify(itrU->loop_segment(0), itrU->loop_segment(1));
	if (itrU.at_top()) break; // finish
      }
    }
    
    // connect bottom and top with random permutation
    std::vector<int> r, c0, c1;
    for (int i = 0; i < vg.mapping.num_groups(); ++i) {
      int s2 = vg.mapping.num_virtual_vertices(i);
      int offset = *(vg.mapping.virtual_vertices(i).first);
      if (s2 == 1) {
	// S=1/2: just connect top and bottom
	union_find::unify(
          config.wl.series(offset).first ->loop_segment(0),
          config.wl.series(offset).second->loop_segment(0));
      } else {
	// S>=1
	r.resize(s2);
	c0.resize(s2);
	c1.resize(s2);
	vertex_iterator vi_end = vg.mapping.virtual_vertices(i).second;
	for (vertex_iterator vi = vg.mapping.virtual_vertices(i).first;
	     vi != vi_end; ++vi) {
	  r[*vi - offset] = *vi - offset;
	  c0[*vi - offset] = config.wl.series(*vi).first->conf();
	  c1[*vi - offset] = config.wl.series(*vi).second->conf();
	}
	restricted_random_shuffle(r.begin(), r.end(),
				  c0.begin(), c0.end(),
				  c1.begin(), c1.end(),
				  uniform_01);
	for (int j = 0; j < s2; ++j) {
	  union_find::unify(
	    config.wl.series(offset +   j ).first ->loop_segment(0),
	    config.wl.series(offset + r[j]).second->loop_segment(0));
	}
      }
    }
    
    // counting and indexing loops
    config.num_loops0 = 0;
    vi_end = boost::vertices(vg.graph).second;
    for (vertex_iterator vi = boost::vertices(vg.graph).first;
	 vi != vi_end; ++vi) {
      iterator itrB, itrT;
      boost::tie(itrB, itrT) = config.wl.series(*vi);
      if (itrB->loop_segment(0).root()->index == loop_segment::undefined)
	itrB->loop_segment(0).root()->index = (config.num_loops0)++;
      itrB->loop_segment(0).index = itrB->loop_segment(0).root()->index;
      if (itrT->loop_segment(0).root()->index == loop_segment::undefined)
	itrT->loop_segment(0).root()->index = (config.num_loops0)++;
      itrT->loop_segment(0).index = itrT->loop_segment(0).root()->index;
    }

    config.num_loops = config.num_loops0;
    vi_end = boost::vertices(vg.graph).second;
    for (vertex_iterator vi = boost::vertices(vg.graph).first;
	 vi != vi_end; ++vi) {
      // setup iterator
      iterator itr = boost::next(config.wl.series(*vi).first);
      
      // iteration up to t = 1
      while (!itr.at_top()) {
	if (itr.leg() == 0) {
	  if (itr->loop_segment(0).root()->index == loop_segment::undefined) {
	    itr->loop_segment(0).root()->index = config.num_loops;
	    itr->loop_segment(0).index = config.num_loops;
	    ++(config.num_loops);
	  } else {
	    itr->loop_segment(0).index = itr->loop_segment(0).root()->index;
	  }
	  if (itr->loop_segment(1).root()->index == loop_segment::undefined) {
	    itr->loop_segment(1).root()->index = config.num_loops;
	    itr->loop_segment(1).index = config.num_loops;
	    ++(config.num_loops);
	  } else {
	    itr->loop_segment(1).index = itr->loop_segment(1).root()->index;
	  }
	}
	++itr;
      }
    }
    //// std::cout << "identification done.\n";
  }

  template<bool HasCTime, class RNG>
  static void generate_loops(config_type<HasCTime>& config, 
			     const parameter_type& p,
			     RNG& uniform_01)
  {
    generate_loops(config, p.virtual_graph, p.model, p.beta, uniform_01);
  }

  template<bool HasCTime, class RNG>
  static void flip_and_cleanup(config_type<HasCTime>& config,
			       const vg_type& vg, RNG& uniform_01)
  {
    typedef typename config_type<HasCTime>::iterator iterator;
    typedef RNG rng_type;

    std::vector<int> flip(config.num_loops);
    std::generate(flip.begin(), flip.end(),
		  boost::variate_generator<rng_type&,
		                           boost::uniform_smallint<> >(
		    uniform_01, boost::uniform_smallint<>(0, 1)));
    
    // flip spins
    vertex_iterator vi_end = boost::vertices(vg.graph).second;
    for (vertex_iterator vi = boost::vertices(vg.graph).first;
	 vi != vi_end; ++vi) {
      iterator itrB, itrT;
      boost::tie(itrB, itrT) = config.wl.series(*vi);
      if (flip[itrB->loop_segment(0).index] == 1) itrB->flip_conf();
      itrB->clear_graph();
      if (flip[itrT->loop_segment(0).index] == 1) itrT->flip_conf();
      itrT->clear_graph();
      //// std::cout << itrT->conf() << ' ';
    }
    //// std::cout << std::endl;
    
    // upating links
    vi_end = boost::vertices(vg.graph).second;
    for (vertex_iterator vi = boost::vertices(vg.graph).first;
	 vi != vi_end; ++vi) {
      // setup iterator
      iterator itr = boost::next(config.wl.series(*vi).first);
      
      // iteration up to t = 1
      while (!itr.at_top()) {
	if (itr.leg() == 0) {
	  if (itr->is_new() ^
	      (flip[itr->loop_segment(0).index] ^
	       flip[itr->loop_segment(1).index] == 0)) {
	    ++itr;
	    config.wl.erase(boost::prior(itr));
	  } else {
	    itr->clear_graph();
	    ++itr;
	  }
	} else {
	  ++itr;
	}
      }
    }
  }

  template<bool HasCTime, class RNG>
  static void flip_and_cleanup(config_type<HasCTime>& config,
			       const parameter_type& p,
			       RNG& uniform_01)
  {
    flip_and_cleanup(config, p.virtual_graph, uniform_01);
  }

  // measurements

  static double energy_offset(const vg_type& vg, const M& model)
  {
    double offset = 0;
    typename alps::property_map<alps::bond_type_t, graph_type, int>::const_type
      bond_type(alps::get_or_default(alps::bond_type_t(), vg.graph, 0));
    edge_iterator ei_end = boost::edges(vg.graph).second;
    for (edge_iterator ei = boost::edges(vg.graph).first; ei != ei_end; ++ei)
      offset += model.bond(bond_type[*ei]).c();
    return offset / double(vg.num_real_edges);
  }
  static double energy_offset(const parameter_type& p)
  { return energy_offset(p.virtual_graph, p.model); }
  
  template<bool HasCTime>
  static double energy_z(const config_type<HasCTime>& config,
			 const vg_type& vg, const model_type& model)
  {
    typedef typename config_type<HasCTime>::const_iterator const_iterator;
    double ene = 0.;
    typename alps::property_map<alps::bond_type_t, graph_type, int>::const_type
      bond_type(alps::get_or_default(alps::bond_type_t(), vg.graph, 0));
    edge_iterator ei_end = boost::edges(vg.graph).second;
    for (edge_iterator ei = boost::edges(vg.graph).first; ei != ei_end; ++ei) {
      const_iterator
	itr0 = config.wl.series(boost::source(*ei, vg.graph)).first;
      const_iterator
	itr1 = config.wl.series(boost::target(*ei, vg.graph)).first;
      ene += model.bond(bond_type[*ei]).jz() *
	(0.5 - double(itr0->conf())) * (0.5 - double(itr1->conf()));
    }
    return ene / double(vg.num_real_vertices);
  }
  template<bool HasCTime>
  static double energy_z(const config_type<HasCTime>& config,
			 const parameter_type& p)
  { return energy_z(config, p.virtual_graph, p.model); }

  template<bool HasCTime>
  static double energy_z_imp(const config_type<HasCTime>& config,
			     const vg_type& vg, const model_type& model)
  {
    typedef typename config_type<HasCTime>::const_iterator const_iterator;
    double ene = 0.;
    typename alps::property_map<alps::bond_type_t, graph_type, int>::const_type
      bond_type(alps::get_or_default(alps::bond_type_t(), vg.graph, 0));
    edge_iterator ei_end = boost::edges(vg.graph).second;
    for (edge_iterator ei = boost::edges(vg.graph).first; ei != ei_end; ++ei) {
      const_iterator
	itr0 = config.wl.series(boost::source(*ei, vg.graph)).first;
      const_iterator
	itr1 = config.wl.series(boost::target(*ei, vg.graph)).first;
      if (itr0->loop_segment(0).index == itr1->loop_segment(0).index) {
	ene += model.bond(bond_type[*ei]).jz() *
	  (0.5 - double(itr0->conf())) * (0.5 - double(itr1->conf()));
      }
    }
    return ene / double(vg.num_real_vertices);
  }
  template<bool HasCTime>
  static double energy_z_imp(const config_type<HasCTime>& config,
			     const parameter_type& p)
  { return energy_z_imp(config, p.virtual_graph, p.model); }

  template<bool HasCTime>
  static double uniform_sz(const config_type<HasCTime>& config,
			   const vg_type& vg)
  {
    typedef typename config_type<HasCTime>::const_iterator const_iterator;
    double sz = 0.;
    vertex_iterator vi_end = boost::vertices(vg.graph).second;
    for (vertex_iterator vi = boost::vertices(vg.graph).first;
 	 vi != vi_end; ++vi) {
      const_iterator itr = config.wl.series(*vi).first;
      std::cout << (0.5 - double(itr->conf())) << ' ';
      sz += (0.5 - double(itr->conf()));
    }
    std::cout << std::endl;
    return sz / double(vg.num_real_vertices);
  }
  template<bool HasCTime>
  static double uniform_sz(const config_type<HasCTime>& config,
			   const parameter_type& p)
  { return uniform_sz(config, p.virtual_graph); }

}; // struct path_integral

} // namespace looper

#ifndef BOOST_NO_OPERATORS_IN_NAMESPACE
namespace looper {
#endif

template<bool HasCTime>
std::ostream& operator<<(std::ostream& os,
			 const looper::pi_node<HasCTime>& n) {
  n.output(os);
  return os;
}

#ifndef BOOST_NO_OPERATORS_IN_NAMESPACE
} // namespace looper
#endif

#endif // LOOPER_PATH_INTEGRAL_H
