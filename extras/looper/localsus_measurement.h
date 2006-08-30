/*****************************************************************************
*
* ALPS/looper: multi-cluster quantum Monte Carlo algorithms for spin systems
*
* Copyright (C) 2006 by Synge Todo <wistaria@comp-phys.org>
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

#ifndef LOOPER_LOCALSUS_MEASUREMENT_H
#define LOOPER_LOCALSUS_MEASUREMENT_H

#include <looper/measurement.h>
#include <alps/alea.h>
#include <cmath>
#include <string>

namespace looper {

struct local_susceptibility
{
  // TODO: improved estimator to be implemented

  typedef dumb_measurement<local_susceptibility> dumb;

  template<typename MC, typename VLAT, typename TIME>
  struct estimator
  {
    typedef MC   mc_type;
    typedef VLAT virtual_lattice_t;
    typedef TIME time_t;
    typedef typename alps::property_map<gauge_t,
              const typename virtual_lattice_t::virtual_graph_type,
              double>::type gauge_map_t;

    bool measure;
    bool bipartite;
    gauge_map_t gauge;

    template<typename M>
    void initialize(M& m, alps::Parameters const& params,
                    virtual_lattice_t const& vlat,
                    bool is_bipartite, bool is_signed,
                    bool /* use_improved_estimator */)
    {
      measure =
        params.value_or_default("MEASURE[Local Susceptibility]", false);
      if (!measure) {
        std::cerr << "Local Suscepbility will not be measured\n";
        return;
      }

      bipartite = is_bipartite;
      gauge = alps::get_or_default(gauge_t(), vlat.vgraph(), 0);

      add_vector_obs(m, "Local Magnetization",
                     alps::site_labels(vlat.rgraph()), is_signed);
      add_vector_obs(m, "Local Susceptibility",
                     alps::site_labels(vlat.rgraph()), is_signed);
      add_vector_obs(m, "Local Field Susceptibility",
                     alps::site_labels(vlat.rgraph()), is_signed);
      if (bipartite)
        add_vector_obs(m, "Staggered Local Susceptibility",
                       alps::site_labels(vlat.rgraph()), is_signed);
    }

    // improved estimator

    typedef typename dumb::template estimator<MC, VLAT, TIME>::estimate
      estimate;
    void init_estimate(estimate const&) const {}

    typedef typename dumb::template estimator<MC, VLAT, TIME>::collector
      collector;
    void init_collector(collector const&) const {}

    template<typename M, typename OP, typename FRAGMENT>
    void improved_measurement(M& /* m */,
                              virtual_lattice_t const& /* vlat */,
                              double /* beta */,
                              double /* sign */,
                              std::vector<int> const& /* spins */,
                              std::vector<OP> const& /* operators */,
                              std::vector<int> const& /* spins_c */,
                              std::vector<FRAGMENT> const& /* fragments */,
                              collector const& /* coll */) {}

    // normal estimator

    template<typename M, typename OP>
    void normal_measurement(M& m, virtual_lattice_t const& vlat,
                            double beta, double sign,
                            std::vector<int> const& spins,
                            std::vector<OP> const& operators,
                            std::vector<int>& spins_c)
    {
      if (!typename looper::is_path_integral<mc_type>::type()) return;
      if (!measure) return;

      int nrs = num_vertices(vlat.rgraph());

      std::valarray<double> lmag(0., nrs);
      double umag = 0;
      double smag = 0;
      typename virtual_lattice_t::virtual_site_iterator si, si_end;
      for (boost::tie(si, si_end) = vsites(vlat); si != si_end; ++si) {
        int r = rsite(vlat, *si);
        lmag[r] += 0.5-spins[*si];
        umag += 0.5-spins[*si];
        smag += gauge[*si] * (0.5-spins[*si]);
      }

      std::valarray<double> lmag_a(0., nrs); /* 0 * lmag; */
      double umag_a = 0; /* 0 * umag; */
      double smag_a = 0; /* 0 * smag; */
      std::copy(spins.begin(), spins.end(), spins_c.begin());
      double t = 0;
      for (typename std::vector<OP>::const_iterator oi = operators.begin();
           oi != operators.end(); ++oi) {
        if (oi->is_offdiagonal()) {
          proceed(typename is_path_integral<mc_type>::type(), t, *oi);
          if (oi->is_site()) {
            int s = oi->pos();
            int r = rsite(vlat, s);
            lmag_a[r] += t * lmag[r];
            umag_a += t * umag;
            smag_a += t * smag;
            spins_c[s] ^= 1;
            lmag[r] += 1-2*spins_c[s];
            umag += 1-2*spins_c[s];
            smag += gauge[s] * (1-2*spins_c[s]);
            lmag_a[r] -= t * lmag[r];
            umag_a -= t * umag;
            smag_a -= t * smag;
          } else {
            int s0 = vsource(oi->pos(), vlat);
            int s1 = vtarget(oi->pos(), vlat);
            int r0 = rsite(vlat, s0);
            int r1 = rsite(vlat, s1);
            lmag_a[r0] += t * lmag[r0];
            lmag_a[r1] += t * lmag[r1];
            umag_a += t * umag;
            smag_a += t * smag;
            spins_c[s0] ^= 1;
            spins_c[s1] ^= 1;
            lmag[r0] += 1-2*spins_c[s0];
            lmag[r1] += 1-2*spins_c[s1];
            umag += (1-2*spins_c[s0]) + (1-2*spins_c[s1]);
            smag += (gauge[s0] * (1-2*spins_c[s0])) +
              (gauge[s1] * (1-2*spins_c[s1]));
            lmag_a[r0] -= t * lmag[r0];
            lmag_a[r1] -= t * lmag[r1];
            umag_a -= t * umag;
            smag_a -= t * smag;
          }
        }
        proceed(typename is_sse<mc_type>::type(), t);
      }
      for (int r = 0; r < nrs; ++r) lmag_a[r] += lmag[r];
      umag_a += umag;
      smag_a += smag;

      m["Local Magnetization"] << std::valarray<double>(sign * lmag_a);
      m["Local Susceptibility"]
        << std::valarray<double>(sign * beta * umag_a * lmag_a);
      m["Local Susceptibility"]
        << std::valarray<double>(sign * beta * umag_a * lmag_a);
      m["Local Field Susceptibility"]
        << std::valarray<double>(sign * beta * power2(lmag_a));
      if (bipartite)
        m["Staggered Local Susceptibility"]
          << std::valarray<double>(sign * beta * smag_a * lmag_a);
    }
  };

  typedef dumb::evaluator evaluator;
};


struct site_type_susceptibility
{
  // TODO: improved estimator to be implemented

  typedef dumb_measurement<site_type_susceptibility> dumb;

  template<typename MC, typename VLAT, typename TIME>
  struct estimator
  {
    typedef MC   mc_type;
    typedef VLAT virtual_lattice_t;
    typedef TIME time_t;
    typedef typename alps::property_map<gauge_t,
              const typename virtual_lattice_t::virtual_graph_type,
              double>::type gauge_map_t;

    bool measure;
    bool bipartite;
    gauge_map_t gauge;
    unsigned int types;
    std::valarray<double> type_nrs;

    template<typename M>
    void initialize(M& m, alps::Parameters const& params,
                    virtual_lattice_t const& vlat,
                    bool is_bipartite, bool is_signed,
                    bool /* use_improved_estimator */)
    {
      measure =
        params.value_or_default("MEASURE[Site Type Susceptibility]", false);
      if (!measure) {
        std::cerr << "Site Type Suscepbility will not be measured\n";
        return;
      }

      bipartite = is_bipartite;
      gauge = alps::get_or_default(gauge_t(), vlat.vgraph(), 0);

      types = 0;
      typename virtual_lattice_t::real_site_iterator si, si_end;
      for (boost::tie(si, si_end) = sites(vlat.rgraph()); si != si_end; ++si) {
        int t = get(alps::site_type_t(), vlat.rgraph(), *si);
        if (t < 0)
          boost::throw_exception(std::runtime_error("negative site type"));
        if (t >= types) types = t + 1;
      }
      type_nrs.resize(types); type_nrs = 0;
      for (boost::tie(si, si_end) = sites(vlat.rgraph()); si != si_end; ++si)
        type_nrs[get(alps::site_type_t(), vlat.rgraph(), *si)] += 1;

      add_vector_obs(m, "Number of Sites of Each Type", is_signed);
      add_vector_obs(m, "Site Type Magnetization", is_signed);
      add_vector_obs(m, "Site Type Magnetization Density", is_signed);
      add_vector_obs(m, "Site Type Susceptibility", is_signed);
      add_vector_obs(m, "Site Type Field Susceptibility", is_signed);
      if (bipartite) {
        add_vector_obs(m, "Staggered Site Type Magnetization", is_signed);
        add_vector_obs(m, "Staggered Site Type Magnetization Density",
                       is_signed);
        add_vector_obs(m, "Staggered Site Type Susceptibility", is_signed);
        add_vector_obs(m, "Staggered Site Type Field Susceptibility",
                       is_signed);
      }
    }

    // improved estimator

    typedef typename dumb::template estimator<MC, VLAT, TIME>::estimate
      estimate;
    void init_estimate(estimate const&) const {}

    typedef typename dumb::template estimator<MC, VLAT, TIME>::collector
      collector;
    void init_collector(collector const&) const {}

    template<typename M, typename OP, typename FRAGMENT>
    void improved_measurement(M& /* m */,
                              virtual_lattice_t const& /* vlat */,
                              double /* beta */,
                              double /* sign */,
                              std::vector<int> const& /* spins */,
                              std::vector<OP> const& /* operators */,
                              std::vector<int> const& /* spins_c */,
                              std::vector<FRAGMENT> const& /* fragments */,
                              collector const& /* coll */) {}

    // normal estimator

    template<typename M, typename OP>
    void normal_measurement(M& m, virtual_lattice_t const& vlat,
                            double beta, double sign,
                            std::vector<int> const& spins,
                            std::vector<OP> const& operators,
                            std::vector<int>& spins_c)
    {
      if (!typename looper::is_path_integral<mc_type>::type()) return;
      if (!measure) return;

      m["Number of Sites of Each Type"] << type_nrs;

      std::valarray<double> tumag(0., types);
      std::valarray<double> tsmag(0., types);
      double umag = 0;
      double smag = 0;
      typename virtual_lattice_t::virtual_site_iterator si, si_end;
      for (boost::tie(si, si_end) = vsites(vlat); si != si_end; ++si) {
        int p = get(alps::site_type_t(), vlat.rgraph(), rsite(vlat, *si));
        tumag[p] += 0.5-spins[*si];
        tsmag[p] += gauge[*si] * (0.5-spins[*si]);
        umag += 0.5-spins[*si];
        smag += gauge[*si] * (0.5-spins[*si]);
      }

      std::valarray<double> tumag_a(0., types);
      std::valarray<double> tsmag_a(0., types);
      double umag_a = 0;
      double smag_a = 0;
      std::copy(spins.begin(), spins.end(), spins_c.begin());
      double t = 0;
      for (typename std::vector<OP>::const_iterator oi = operators.begin();
           oi != operators.end(); ++oi) {
        if (oi->is_offdiagonal()) {
          proceed(typename is_path_integral<mc_type>::type(), t, *oi);
          if (oi->is_site()) {
            int s = oi->pos();
            int p = get(alps::site_type_t(), vlat.rgraph(), rsite(vlat, s));
            tumag_a[p] += t * tumag[p];
            tsmag_a[p] += t * tsmag[p];
            umag_a += t * umag;
            smag_a += t * smag;
            spins_c[s] ^= 1;
            tumag[p] += 1-2*spins_c[s];
            tsmag[p] += gauge[s] * (1-2*spins_c[s]);
            umag += 1-2*spins_c[s];
            smag += gauge[s] * (1-2*spins_c[s]);
            tumag_a[p] -= t * tumag[p];
            tsmag_a[p] -= t * tsmag[p];
            umag_a -= t * umag;
            smag_a -= t * smag;
          } else {
            int s0 = vsource(oi->pos(), vlat);
            int s1 = vtarget(oi->pos(), vlat);
            int p0 = get(alps::site_type_t(), vlat.rgraph(), rsite(vlat, s0));
            int p1 = get(alps::site_type_t(), vlat.rgraph(), rsite(vlat, s1));
            if (p0 == p1) {
              tumag_a[p0] += t * tumag[p0];
              tsmag_a[p0] += t * tsmag[p0];
            } else {
              tumag_a[p0] += t * tumag[p0];
              tumag_a[p1] += t * tumag[p1];
              tsmag_a[p0] += t * tsmag[p0];
              tsmag_a[p1] += t * tsmag[p1];
            }
            umag_a += t * umag;
            smag_a += t * smag;
            spins_c[s0] ^= 1;
            spins_c[s1] ^= 1;
            tumag[p0] += 1-2*spins_c[s0];
            tumag[p1] += 1-2*spins_c[s1];
            tsmag[p0] += gauge[s0] * (1-2*spins_c[s0]);
            tsmag[p1] += gauge[s1] * (1-2*spins_c[s1]);
            umag += (1-2*spins_c[s0]) + (1-2*spins_c[s1]);
            smag += (gauge[s0] * (1-2*spins_c[s0])) +
              (gauge[s1] * (1-2*spins_c[s1]));
            if (p0 == p1) {
              tumag_a[p0] -= t * tumag[p0];
              tsmag_a[p0] -= t * tsmag[p0];
            } else {
              tumag_a[p0] -= t * tumag[p0];
              tumag_a[p1] -= t * tumag[p1];
              tsmag_a[p0] -= t * tsmag[p0];
              tsmag_a[p1] -= t * tsmag[p1];
            }
            umag_a -= t * umag;
            smag_a -= t * smag;
          }
        }
        proceed(typename is_sse<mc_type>::type(), t);
      }
      for (int p = 0; p < types; ++p) {
        tumag_a[p] += tumag[p];
        tsmag_a[p] += tsmag[p];
      }
      umag_a += umag;
      smag_a += smag;

      m["Site Type Magnetization"] << std::valarray<double>(sign * tumag_a);
      m["Site Type Magnetization Density"]
        << std::valarray<double>(sign * tumag_a / type_nrs);
      m["Site Type Susceptibility"]
        << std::valarray<double>(sign * beta * umag_a * tumag_a / type_nrs);
      m["Site Type Field Susceptibility"]
        << std::valarray<double>(sign * beta * power2(tumag_a) / type_nrs);
      if (bipartite) {
        m["Staggered Site Type Magnetization"]
          << std::valarray<double>(sign * tsmag_a);
        m["Staggered Site Type Magnetization Density"]
          << std::valarray<double>(sign * tsmag_a / type_nrs);
        m["Staggered Site Type Susceptibility"]
          << std::valarray<double>(sign * beta * smag_a * tsmag_a / type_nrs);
        m["Staggered Site Type Field Susceptibility"]
          << std::valarray<double>(sign * beta * power2(tsmag_a) / type_nrs);
      }
    }
  };

  typedef dumb::evaluator evaluator;
};

} // end namespace looper

#endif // LOOPER_TYPEDSUS_MEASUREMENT_H
