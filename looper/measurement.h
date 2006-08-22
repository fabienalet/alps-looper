/*****************************************************************************
*
* ALPS/looper: multi-cluster quantum Monte Carlo algorithms for spin systems
*
* Copyright (C) 1997-2006 by Synge Todo <wistaria@comp-phys.org>
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

#ifndef LOOPER_MEASUREMENT_H
#define LOOPER_MEASUREMENT_H

#include "divide_if_positive.h"
#include "lattice.h"
#include "power.h"
#include "type.h"

#include <alps/fixed_capacity_vector.h>
#include <alps/lattice/graph_traits.h>
#include <alps/alea.h>
#include <boost/call_traits.hpp>
#include <boost/mpl/bool.hpp>
#include <string>
#include <vector>

namespace looper {

//
// helper functions
//

inline void add_measurement(alps::ObservableSet& m, std::string const& name,
                            bool is_signed = false)
{
  if (!m.has(name))
    m << make_observable(alps::RealObservable(name), is_signed);
}

namespace measurement {

// for path integral
template<typename OP>
inline void proceed(boost::mpl::true_, double& t, OP const& op)
{ t = op.time(); }
template<typename OP>
inline void proceed(boost::mpl::false_, double, OP const&) {}

// for sse
inline void proceed(boost::mpl::true_, double& t) { t += 1; }
inline void proceed(boost::mpl::false_, double) {}


//
// traits classes
//

template<typename ESTIMATOR>
struct estimate
{
  typedef typename ESTIMATOR::estimate type;
};

template<typename ESTIMATOR, typename QMC, typename IMPROVE>
struct collector
{
  typedef typename ESTIMATOR::template collector<QMC, IMPROVE> type;
};

} // end namespace measurement


//
// accumulator for improved estimator
//

template<typename ESTIMATOR, typename TIME, typename FRAGMENT,
         typename IMPROVE>
struct accumulator
{
  typedef ESTIMATOR                                         estimator_t;
  typedef typename measurement::estimate<estimator_t>::type estimate_t;
  typedef typename estimator_t::virtual_lattice_t           virtual_lattice_t;
  typedef typename boost::call_traits<typename estimator_t::time_t>::param_type
    time_pt;
  typedef FRAGMENT                                          fragment_t;
  accumulator(std::vector<estimate_t> const&, int /* nc */,
              virtual_lattice_t const& /* vlat */,
              estimator_t const& /* emt */,
              std::vector<fragment_t> const& /* fr */) {}
  void start_s(int, time_pt, int, int) const {}
  void start_b(int, int, time_pt, int, int, int, int, int) const {}
  void term_s(int, time_pt, int, int) const {}
  void term_b(int, int, time_pt, int, int, int, int, int) const {}
  void at_bot(int, time_pt, int, int) const {}
  void at_top(int, time_pt, int, int) const {}
};

template<typename ESTIMATOR, typename TIME, typename FRAGMENT>
struct accumulator<ESTIMATOR, TIME, FRAGMENT, boost::mpl::true_>
{
  typedef ESTIMATOR                                         estimator_t;
  typedef typename measurement::estimate<estimator_t>::type estimate_t;
  typedef typename estimator_t::virtual_lattice_t           virtual_lattice_t;
  typedef typename boost::call_traits<typename estimator_t::time_t>::param_type
    time_pt;
  typedef FRAGMENT                                          fragment_t;
  accumulator(std::vector<estimate_t>& es, int nc,
              virtual_lattice_t const& vlat, estimator_t const& emt,
              std::vector<fragment_t> const& fr) :
    estimates(es), vlattice(vlat), estimator(emt), fragments(fr)
  {
    estimate_t e;
    emt.init_estimate(e);
    estimates.resize(0); estimates.resize(nc, e);
  }
  void start_s(int p, time_pt t, int s, int c)
  { estimates[fragments[p].id].start_s(vlattice, t, s, c); }
  void start_b(int p0, int p1, time_pt t, int b, int s0, int s1,
               int c0, int c1)
  {
    estimates[fragments[p0].id].start_bs(vlattice, t, b, s0, c0);
    estimates[fragments[p1].id].start_bt(vlattice, t, b, s1, c1);
  }
  void term_s(int p, time_pt t, int s, int c)
  { estimates[fragments[p].id].term_s(vlattice, t, s, c); }
  void term_b(int p0, int p1, time_pt t, int b, int s0, int s1,
              int c0, int c1)
  {
    estimates[fragments[p0].id].term_bs(vlattice, t, b, s0, c0);
    estimates[fragments[p1].id].term_bt(vlattice, t, b, s1, c1);
  }
  void at_bot(int p, time_pt t, int s, int c)
  { estimates[fragments[p].id].at_bot(vlattice, t, s, c); }
  void at_top(int p, time_pt t, int s, int c)
  { estimates[fragments[p].id].at_top(vlattice, t, s, c); }

  std::vector<estimate_t>&       estimates;
  virtual_lattice_t const&       vlattice;
  estimator_t const&             estimator;
  std::vector<fragment_t> const& fragments;
};


//
// energy_estimator
//

struct energy_estimator
{
  template<typename M>
  static void initialize(M& m, bool is_signed)
  {
    add_measurement(m, "Energy", is_signed);
    add_measurement(m, "Energy Density", is_signed);
    add_measurement(m, "Energy^2", is_signed);
  }

  static void evaluate(alps::ObservableSet& m,
                       alps::ObservableSet const& m_in)
  {
    if (m_in.has("Inverse Temperature") && m_in.has("Number of Sites") &&
        m_in.has("Energy") && m_in.has("Energy^2")) {
      double beta = alps::RealObsevaluator(m_in["Inverse Temperature"]).mean();
      double nrs = alps::RealObsevaluator(m_in["Number of Sites"]).mean();
      alps::RealObsevaluator obse_e = m_in["Energy"];
      alps::RealObsevaluator obse_e2 = m_in["Energy^2"];
      alps::RealObsevaluator eval("Specific Heat");
      eval = power2(beta) * (obse_e2 - power2(obse_e)) / nrs;
      m.addObservable(eval);
    }
  }

  // normal estimator

  template<typename RG, typename VG>
  static void measurement(alps::ObservableSet& m,
                          virtual_lattice<RG, VG> const& vlat,
                          double beta, int nop, double sign, double ene)
  {
    m["Energy"] << sign * ene;
    m["Energy Density"] << sign * ene / num_sites(vlat.rgraph());
    m["Energy^2"] << sign * (power2(ene) - nop / power2(beta));
  }
};


//
// dumb_estimator
//

template<typename VLAT, typename TIME>
struct dumb_estimator
{
  typedef VLAT virtual_lattice_t;
  typedef TIME time_t;

  template<typename M>
  void initialize(M& /* m */, alps::Parameters const& /* params */,
                  virtual_lattice_t const& /* vlat */,
                  bool /* is_bipartite */, bool /* is_signed */,
                  bool /* use_improved_estimator */) {}
  static void evaluate(alps::ObservableSet& /* m */,
                       alps::Parameters const& /* params */,
                       alps::ObservableSet const& /* m_in */) {}

  // improved estimator

  struct estimate
  {
    void start_s(virtual_lattice_t const&, double, int, int) const {}
    void start_bs(virtual_lattice_t const&, double, int, int, int) const {}
    void start_bt(virtual_lattice_t const&, double, int, int, int) const {}
    void term_s(virtual_lattice_t const&, double, int, int) const {}
    void term_bs(virtual_lattice_t const&, double, int, int, int) const {}
    void term_bt(virtual_lattice_t const&, double, int, int, int) const {}
    void at_bot(virtual_lattice_t const&, double, int, int) const {}
    void at_top(virtual_lattice_t const&, double, int, int) const {}
  };
  void init_estimate(estimate& /* est */) const {}

  template<typename QMC, typename IMPROVE>
  struct collector
  {
    template<typename EST>
    collector operator+(EST const& /* est */) const { return *this; }
    template<typename M>
    void commit(M& /* m */, virtual_lattice_t const& /* vlat */,
                bool /* is_bipartite */, double /* beta */, int /* nop */,
                double /* sign */) const
    {}
  };
  template<typename QMC, typename IMPROVE>
  void init_collector(collector<QMC, IMPROVE>& /* coll */) const {}

  // normal estimator

  template<typename QMC, typename M, typename OP>
  void normal_measurement(M& /* m */,
                          virtual_lattice_t const& /* vlat */,
                          bool /* is_bipartite */,
                          bool /* use_imporved_estimator */,
                          double /* beta */,
                          double /* sign */,
                          std::vector<int> const& /* spins */,
                          std::vector<OP> const& /* operators */,
                          std::vector<int> const& /* spins_c */) {}
};


//
// estimator_adaptor
//

template<typename ESTIMATOR1, typename ESTIMATOR2>
struct composite_estimator
{
  typedef ESTIMATOR1 estimator1;
  typedef ESTIMATOR2 estimator2;
  typedef typename ESTIMATOR1::virtual_lattice_t          virtual_lattice_t;
  typedef typename ESTIMATOR1::time_t                     time_t;
  typedef typename boost::call_traits<time_t>::param_type time_pt;

  estimator1 emt1;
  estimator2 emt2;

  template<typename M>
  void initialize(M& m, alps::Parameters const& params,
                  virtual_lattice_t const& vlat,
                  bool is_bipartite, bool is_signed,
                  bool use_improved_estimator)
  {
    emt1.initialize(m, params, vlat, is_bipartite, is_signed,
                    use_improved_estimator);
    emt2.initialize(m, params, vlat, is_bipartite, is_signed,
                    use_improved_estimator);
  };

  static void evaluate(alps::ObservableSet& m,
                       alps::Parameters const& params,
                       alps::ObservableSet const& m_in)
  {
    estimator1::evaluate(m, params, m_in);
    estimator2::evaluate(m, params, m_in);
  }

  struct estimate : public estimator1::estimate, public estimator2::estimate
  {
    void start_s(virtual_lattice_t const& vlat, time_pt t, int s, int c)
    {
      estimator1::estimate::start_s(vlat, t, s, c);
      estimator2::estimate::start_s(vlat, t, s, c);
    }
    void start_bs(virtual_lattice_t const& vlat, time_pt t, int b, int s,
                  int c)
    {
      estimator1::estimate::start_bs(vlat, t, b, s, c);
      estimator2::estimate::start_bs(vlat, t, b, s, c);
    }
    void start_bt(virtual_lattice_t const& vlat, time_pt t, int b, int s,
                  int c)
    {
      estimator1::estimate::start_bt(vlat, t, b, s, c);
      estimator2::estimate::start_bt(vlat, t, b, s, c);
    }
    void term_s(virtual_lattice_t const& vlat, time_pt t, int s, int c)
    {
      estimator1::estimate::term_s(vlat, t, s, c);
      estimator2::estimate::term_s(vlat, t, s, c);
    }
    void term_bs(virtual_lattice_t const& vlat, time_pt t, int b, int s, int c)
    {
      estimator1::estimate::term_bs(vlat, t, b, s, c);
      estimator2::estimate::term_bs(vlat, t, b, s, c);
    }
    void term_bt(virtual_lattice_t const& vlat, time_pt t, int b, int s, int c)
    {
      estimator1::estimate::term_bt(vlat, t, b, s, c);
      estimator2::estimate::term_bt(vlat, t, b, s, c);
    }
    void at_bot(virtual_lattice_t const& vlat, time_pt t, int s, int c)
    {
      estimator1::estimate::at_bot(vlat, t, s, c);
      estimator2::estimate::at_bot(vlat, t, s, c);
    }
    void at_top(virtual_lattice_t const& vlat, time_pt t, int s, int c)
    {
      estimator1::estimate::at_top(vlat, t, s, c);
      estimator2::estimate::at_top(vlat, t, s, c);
    }
  };
  void init_estimate(estimate& est) const
  {
    emt1.init_estimate(est);
    emt2.init_estimate(est);
  }

  template<typename QMC, typename IMPROVE>
  struct collector
    : public estimator1::template collector<QMC, IMPROVE>,
      public estimator2::template collector<QMC, IMPROVE>
  {
    typedef typename estimator1::template collector<QMC, IMPROVE>
      base1;
    typedef typename estimator2::template collector<QMC, IMPROVE>
      base2;
    template<typename EST>
    collector operator+(EST const& est)
    {
      base1::operator+(est);
      base2::operator+(est);
      return *this;
    }
    template<typename M>
    void commit(M& m, virtual_lattice_t const& vlat, bool is_bipartite,
                double beta, int nop, double sign) const
    {
      base1::commit(m, vlat, is_bipartite, beta, nop, sign);
      base2::commit(m, vlat, is_bipartite, beta, nop, sign);
    }
  };
  template<typename QMC, typename IMPROVE>
  void init_collector(collector<QMC, IMPROVE>& coll) const
  {
    emt1.init_collector(coll);
    emt2.init_collector(coll);
  }

  template<typename QMC, typename M, typename OP>
  void normal_measurement(M& m, virtual_lattice_t const& vlat,
                          bool is_bipartite, bool use_improved_estimator,
                          double beta, double sign,
                          std::vector<int> const& spins,
                          std::vector<OP> const& operators,
                          std::vector<int>& spins_c)
  {
    emt1.normal_measurement<QMC>(m, vlat, is_bipartite, use_improved_estimator,
                                 beta, sign, spins, operators, spins_c);
    emt2.normal_measurement<QMC>(m, vlat, is_bipartite, use_improved_estimator,
                                 beta, sign, spins, operators, spins_c);
  }
};


//
// susceptibility_estimator
//

template<typename VLAT, typename TIME>
struct susceptibility_estimator
{
  typedef VLAT virtual_lattice_t;
  typedef TIME time_t;

  template<typename M>
  void initialize(M& m, alps::Parameters const& /* params */,
                  virtual_lattice_t const& /* vlat */,
                  bool is_bipartite, bool is_signed,
                  bool use_improved_estimator)
  {
    add_measurement(m, "Magnetization", is_signed);
    add_measurement(m, "Magnetization Density", is_signed);
    add_measurement(m, "Magnetization^2", is_signed);
    add_measurement(m, "Magnetization^4", is_signed);
    add_measurement(m, "Susceptibility", is_signed);
    if (use_improved_estimator) {
      add_measurement(m, "Generalized Magnetization^2", is_signed);
      add_measurement(m, "Generalized Magnetization^4", is_signed);
      add_measurement(m, "Generalized Susceptibility", is_signed);
    }
    if (is_bipartite) {
      add_measurement(m, "Staggered Magnetization", is_signed);
      add_measurement(m, "Staggered Magnetization Density", is_signed);
      add_measurement(m, "Staggered Magnetization^2", is_signed);
      add_measurement(m, "Staggered Magnetization^4", is_signed);
      add_measurement(m, "Staggered Susceptibility", is_signed);
      if (use_improved_estimator) {
        add_measurement(m, "Generalized Staggered Magnetization^2",
                        is_signed);
        add_measurement(m, "Generalized Staggered Magnetization^4",
                        is_signed);
        add_measurement(m, "Generalized Staggered Susceptibility",
                        is_signed);
      }
    }
  }

  static void evaluate(alps::ObservableSet& m,
                       alps::Parameters const& /* params */,
                       alps::ObservableSet const& m_in)
  {
    if (m_in.has("Magnetization^2") && m_in.has("Magnetization^4")) {
      alps::RealObsevaluator obse_m2 = m_in["Magnetization^2"];
      alps::RealObsevaluator obse_m4 = m_in["Magnetization^4"];
      alps::RealObsevaluator eval("Binder Ratio of Magnetization");
      eval = power2(obse_m2) / obse_m4;
      m.addObservable(eval);
    }
    if (m_in.has("Staggered Magnetization^2") &&
        m_in.has("Staggered Magnetization^4")) {
      alps::RealObsevaluator obse_m2 = m_in["Staggered Magnetization^2"];
      alps::RealObsevaluator obse_m4 = m_in["Staggered Magnetization^4"];
      alps::RealObsevaluator eval("Binder Ratio of Staggered Magnetization");
      eval = power2(obse_m2) / obse_m4;
      m.addObservable(eval);
    }
    if (m_in.has("Generalized Magnetization^2") &&
        m_in.has("Generalized Magnetization^4")) {
      alps::RealObsevaluator obse_m2 = m_in["Generalized Magnetization^2"];
      alps::RealObsevaluator obse_m4 = m_in["Generalized Magnetization^4"];
      alps::RealObsevaluator eval("Binder Ratio of Generalized Magnetization");
      eval = power2(obse_m2) / obse_m4;
      m.addObservable(eval);
    }
    if (m_in.has("Generalized Staggered Magnetization^2") &&
        m_in.has("Generalized Staggered Magnetization^4")) {
      alps::RealObsevaluator obse_m2 =
        m_in["Generalized Staggered Magnetization^2"];
      alps::RealObsevaluator obse_m4 =
        m_in["Generalized Staggered Magnetization^4"];
      alps::RealObsevaluator
        eval("Binder Ratio of Generalized Staggered Magnetization");
      eval = power2(obse_m2) / obse_m4;
      m.addObservable(eval);
    }
  }

  // improved estimator

  struct estimate
  {
    double usize0, umag0, usize, umag;
    double ssize0, smag0, ssize, smag;
    void init()
    {
      usize0 = 0;
      umag0 = 0;
      usize = 0;
      umag = 0;
      ssize0 = 0;
      smag0 = 0;
      ssize = 0;
      smag = 0;
    }
    void start_s(virtual_lattice_t const& vlat, double t, int s, int c)
    {
      usize -= t * 0.5;
      umag  -= t * (0.5-c);
      double gg = gauge(vlat, s);
      ssize -= gg * t * 0.5;
      smag  -= gg * t * (0.5-c);
    }
    void start_bs(virtual_lattice_t const& vlat, double t, int, int s, int c)
    { start_s(vlat, t, s, c); }
    void start_bt(virtual_lattice_t const& vlat, double t, int, int s, int c)
    { start_s(vlat, t, s, c); }
    void term_s(virtual_lattice_t const& vlat, double t, int s, int c)
    {
      usize += t * 0.5;
      umag  += t * (0.5-c);
      double gg = gauge(vlat, s);
      ssize += gg * t * 0.5;
      smag  += gg * t * (0.5-c);
    }
    void term_bs(virtual_lattice_t const& vlat, double t, int, int s, int c)
    { term_s(vlat, t, s, c); }
    void term_bt(virtual_lattice_t const& vlat, double t, int, int s, int c)
    { term_s(vlat, t, s, c); }
    void at_bot(virtual_lattice_t const& vlat, double t, int s, int c)
    {
      start_s(vlat, t, s, c);
      usize0 += 0.5;
      umag0  += (0.5-c);
      double gg = gauge(vlat, s);
      ssize0 += gg * 0.5;
      smag0  += gg * (0.5-c);
    }
    void at_top(virtual_lattice_t const& vlat, double t, int s, int c)
    { term_s(vlat, t, s, c); }
  };
  void init_estimate(estimate& est) const { est.init(); }

  template<typename QMC, typename IMPROVE>
  struct collector
  {
    double usize2, umag2, usize4, umag4, usize, umag;
    double ssize2, smag2, ssize4, smag4, ssize, smag;
    void init()
    {
      usize2 = 0; umag2 = 0; usize4 = 0; umag4 = 0;
      usize = 0; umag = 0;
      ssize2 = 0; smag2 = 0; ssize4 = 0; smag4 = 0;
      ssize = 0; smag = 0;
    }
    template<typename EST>
    collector operator+(EST const& est)
    {
      usize2 += power2(est.usize0);
      umag2  += power2(est.umag0);
      usize4 += power4(est.usize0);
      umag4  += power4(est.umag0);
      usize  += power2(est.usize);
      umag   += power2(est.umag);
      ssize2 += power2(est.ssize0);
      smag2  += power2(est.smag0);
      ssize4 += power4(est.ssize0);
      smag4  += power4(est.smag0);
      ssize  += power2(est.ssize);
      smag   += power2(est.smag);
      return *this;
    }
    template<typename M>
    void commit(M& m, virtual_lattice_t const& vlat, bool is_bipartite,
                double beta, int nop, double sign) const
    {
      int nrs = num_sites(vlat.rgraph());
      m["Magnetization"] << 0.0;
      m["Magnetization Density"] << 0.0;
      m["Magnetization^2"] << sign * umag2;
      m["Magnetization^4"] << sign * (3 * umag2 * umag2 - 2 * umag4);
      m["Susceptibility"]
        << (typename is_sse<QMC>::type() ?
            sign * beta * (dip(umag, nop) + umag2) / (nop + 1) / nrs :
            sign * beta * umag / nrs);
      m["Generalized Magnetization^2"] << sign * usize2;
      m["Generalized Magnetization^4"]
        << sign * (3 * usize2 * usize2 - 2 * usize4);
      m["Generalized Susceptibility"]
        << (typename is_sse<QMC>::type() ?
            sign * beta * (dip(usize, nop) + usize2) / (nop + 1) / nrs :
            sign * beta * usize / nrs);
      if (is_bipartite) {
        m["Staggered Magnetization"] << 0.0;
        m["Staggered Magnetization Density"] << 0.0;
        m["Staggered Magnetization^2"] << sign * smag2;
        m["Staggered Magnetization^4"]
          << sign * (3 * smag2 * smag2 - 2 * smag4);
        m["Staggered Susceptibility"]
          << (typename is_sse<QMC>::type() ?
              sign * beta * (dip(smag, nop) + smag2) / (nop + 1) / nrs :
              sign * beta * smag /nrs);
        m["Generalized Staggered Magnetization^2"] << sign * ssize2;
        m["Generalized Staggered Magnetization^4"]
          << sign * (3 * ssize2 * ssize2 - 2 * ssize4);
        m["Generalized Staggered Susceptibility"]
          << (typename is_sse<QMC>::type() ?
              sign * beta * (dip(ssize, nop) + ssize2) / (nop + 1) / nrs :
              sign * beta * ssize / nrs);
      }
    }
  };
  template<typename QMC, typename IMPROVE>
  void init_collector(collector<QMC, IMPROVE>& coll) const { coll.init(); }

  template<typename QMC, typename M, typename OP>
  void normal_measurement(M& m, virtual_lattice_t const& vlat,
                          bool is_bipartite, bool use_improved_estimator,
                          double beta, double sign,
                          std::vector<int> const& spins,
                          std::vector<OP> const& operators,
                          std::vector<int>& spins_c)
  {
    if (use_improved_estimator) return;

    int nrs = num_sites(vlat.rgraph());
    int nop = operators.size();
    double umag = 0;
    double smag = 0;
    typename virtual_lattice_t::virtual_site_iterator si, si_end;
    for (boost::tie(si, si_end) = vsites(vlat); si != si_end; ++si) {
      umag += 0.5-spins[*si];
      smag += (0.5-spins[*si]) * gauge(vlat, *si);
    }
    m["Magnetization"] << sign * umag;
    m["Magnetization Density"] << sign * umag / nrs;
    m["Magnetization^2"] << sign * power2(umag);
    m["Magnetization^4"] << sign * power4(umag);
    if (is_bipartite) {
      m["Staggered Magnetization"] << sign * smag;
      m["Staggered Magnetization Density"] << sign * smag / nrs;
      m["Staggered Magnetization^2"] << sign * power2(smag);
      m["Staggered Magnetization^4"] << sign * power4(smag);
    }
    double umag_a = 0; /* 0 * umag; */
    double smag_a = 0; /* 0 * smag; */
    std::copy(spins.begin(), spins.end(), spins_c.begin());
    double t = 0;
    for (typename std::vector<OP>::const_iterator oi = operators.begin();
         oi != operators.end(); ++oi) {
      if (oi->is_offdiagonal()) {
        measurement::proceed(typename is_path_integral<QMC>::type(), t, *oi);
        umag_a += t * umag;
        smag_a += t * smag;
        if (oi->is_site()) {
          unsigned int s = oi->pos();
          spins_c[s] ^= 1;
          umag += 1-2*spins_c[s];
          smag += gauge(vlat, s) * (1-2*spins_c[s]);
        } else {
          unsigned int s0 = vsource(oi->pos(), vlat);
          unsigned int s1 = vtarget(oi->pos(), vlat);
          spins_c[s0] ^= 1;
          spins_c[s1] ^= 1;
          umag += 1-2*spins_c[s0] + 1-2*spins_c[s1];
          smag += gauge(vlat, s0) * (1-2*spins_c[s0])
            + gauge(vlat, s1) * (1-2*spins_c[s1]);
        }
        umag_a -= t * umag;
        smag_a -= t * smag;
      }
      measurement::proceed(typename is_sse<QMC>::type(), t);
    }
    if (typename is_path_integral<QMC>::type()) {
      umag_a += umag;
      m["Susceptibility"] << sign * beta * power2(umag_a) / nrs;
      smag_a += smag;
      if (is_bipartite)
        m["Staggered Susceptibility"] << sign * beta * power2(smag_a) / nrs;
    } else {
      umag_a += nop * umag;
      m["Susceptibility"]
        << sign * beta * (dip(power2(umag_a), nop) + power2(umag))
        / (nop + 1) / nrs;
      smag_a += nop * smag;
      if (is_bipartite)
        m["Staggered Susceptibility"]
          << sign * beta * (dip(power2(smag_a), nop) + power2(smag))
          / (nop + 1) / nrs;
    }
  }
};


//
// stiffness_estimator
//

template<typename VLAT, typename TIME, unsigned int MAX_DIM>
struct stiffness_estimator
{
  typedef VLAT virtual_lattice_t;
  typedef TIME time_t;

  unsigned int dim;

  template<typename M>
  void initialize(M& m, alps::Parameters const& /* params */,
                  virtual_lattice_t const& vlat,
                  bool /* is_bipartite */,
                  bool is_signed,
                  bool /* use_improved_estimator */)
  {
    dim = boost::get_property(vlat.rgraph(), dimension_t());
    if (dim > MAX_DIM) {
      std::cerr << "Spatial dimension (=" << dim << ") is too large.  "
                << "Stiffness will be measured only for the first "
                << MAX_DIM << " dimensions\n";
      dim = MAX_DIM;
    }
    if (dim > 0) add_measurement(m, "Stiffness", is_signed);
  }
  static void evaluate(alps::ObservableSet& /* m */,
                       alps::Parameters const& /* params */,
                       alps::ObservableSet const& /* m_in */) {}

  // improved estimator

  struct estimate
  {
    alps::fixed_capacity_vector<double, MAX_DIM> winding;
    void init(int dim) { winding.resize(dim); }
    void start_s(virtual_lattice_t const&, double, int, int) const {}
    void start_bs(virtual_lattice_t const& vlat, double, int b, int,
                  int c)
    {
      alps::coordinate_type vr =
        get(bond_vector_relative_t(), vlat.rgraph(), rbond(vlat, b));
      for (int i = 0; i < winding.size(); ++i)
        winding[i] -= (1-2*c) * vr[i];
    }
    void start_bt(virtual_lattice_t const&, double, int, int, int) {}
    void term_s(virtual_lattice_t const&, double, int, int) const {}
    void term_bs(virtual_lattice_t const& vlat, double, int b, int,
                 int c)
    {
      alps::coordinate_type vr =
        get(bond_vector_relative_t(), vlat.rgraph(), rbond(vlat, b));
      for (int i = 0; i < winding.size(); ++i)
        winding[i] += (1-2*c) * vr[i];
    }
    void term_bt(virtual_lattice_t const&, double, int, int, int) {}
    void at_bot(virtual_lattice_t const&, double, int, int) const {}
    void at_top(virtual_lattice_t const&, double, int, int) const {}
  };
  void init_estimate(estimate& est) const { est.init(dim); }

  template<typename QMC, typename IMPROVE>
  struct collector
  {
    unsigned int dim;
    double w2;
    void init(unsigned int d) { dim = d; w2 = 0; }
    template<typename EST>
    collector operator+(EST const& est)
    {
      for (int i = 0; i < dim; ++i) w2 += power2(0.5 * est.winding[i]);
      return *this;
    }
    template<typename M>
    void commit(M& m, virtual_lattice_t const&, bool, double beta, int,
                double sign) const
    { if (dim > 0) m["Stiffness"] << sign * w2 / (beta * dim); }
  };
  template<typename QMC, typename IMPROVE>
  void init_collector(collector<QMC, IMPROVE>& coll) const { coll.init(dim); }

  // normal estimator

  template<typename QMC, typename M, typename OP>
  void normal_measurement(M& m, virtual_lattice_t const& vlat,
                          bool /* is_bipartite */, bool use_improved_estimator,
                          double beta, double sign,
                          std::vector<int> const& spins,
                          std::vector<OP> const& operators,
                          std::vector<int>& spins_c)
  {
    if (use_improved_estimator || dim == 0) return;

    std::valarray<double> winding(0., dim);
    std::copy(spins.begin(), spins.end(), spins_c.begin());
    for (typename std::vector<OP>::const_iterator oi = operators.begin();
         oi != operators.end(); ++oi) {
      if (oi->is_offdiagonal()) {
        if (oi->is_bond()) {
          double s = 1-2*spins_c[vsource(oi->pos(), vlat)];
          alps::coordinate_type vr =
            get(bond_vector_relative_t(), vlat.rgraph(),
                rbond(vlat, oi->pos()));
          for (int i = 0; i < dim; ++i) winding[i] += s * vr[i];
          spins_c[vsource(oi->pos(), vlat)] ^= 1;
          spins_c[vtarget(oi->pos(), vlat)] ^= 1;
        } else {
          spins_c[oi->pos()] ^= 1;
        }
      }
    }

    double w2 = 0;
    for (int i = 0; i < dim; ++i) w2 += power2(winding[i]);
    m["Stiffness"] << sign * w2 / (beta * dim);
  }
};

} // end namespace looper

#endif // LOOPER_MEASUREMENT_H
