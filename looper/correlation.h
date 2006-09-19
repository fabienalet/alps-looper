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

#ifndef LOOPER_CORRELATION_H
#define LOOPER_CORRELATION_H

#include "measurement.h"
#include <boost/optional.hpp>

namespace looper {

struct correlation
{
  typedef dumb_measurement<correlation> dumb;

  template<typename MC, typename LAT, typename TIME>
  struct estimator
  {
    typedef MC   mc_type;
    typedef LAT  lattice_t;
    typedef TIME time_t;
    typedef typename alps::property_map<real_site_t,
              const typename lattice_t::virtual_graph_type,
              typename real_site_descriptor<lattice_t>::type>::type
              real_site_map_t;
    typedef typename alps::property_map<gauge_t,
              const typename lattice_t::virtual_graph_type,
              double>::type gauge_map_t;
    typedef typename alps::property_map<coordinate_t,
              const typename lattice_t::real_graph_type,
              coordinate_type>::type coordinate_map_t;

    bool measure_correlation, measure_green_function,
      measure_structure_factor;
    bool improved;
    real_site_map_t real_site;
    gauge_map_t gauge;
    coordinate_map_t coordinate;
    boost::optional<int> origin;
    std::valarray<double> mltplcty;

    // working vector
    std::valarray<double> ucorr, scorr, gucorr, gscorr, gfunc, sfac;

    template<typename M>
    void initialize(M& m, alps::Parameters const& params,
                    lattice_t const& lat,
                    bool is_signed, bool use_improved_estimator)
    {
      measure_correlation =
        params.value_or_default("MEASURE[Correlations]", false);
      measure_green_function =
        params.value_or_default("MEASURE[Green Function]", false);
      measure_structure_factor =
        params.value_or_default("MEASURE[Structure Factor]", false);
      improved = use_improved_estimator;

      if (measure_green_function && !improved) {
        std::cerr << "Warning: Green funciton measurement is disabled\n";
        measure_green_function = false;
      }

      if (measure_correlation || measure_green_function ||
          measure_structure_factor) {
        real_site = alps::get_or_default(real_site_t(), lat.vg(), 0);
      }

      if (measure_correlation || measure_green_function) {
        gauge = alps::get_or_default(gauge_t(), lat.vg(), 0);
        if (params.defined("INITIAL_SITE"))
          origin = static_cast<int>(params["INITIAL_SITE"]);

        std::vector<std::string> label = distance_labels(lat, origin);
        if (!origin) {
          std::vector<unsigned int> m = distance_multiplicities(lat);
          mltplcty.resize(m.size());
          for (int i = 0; i < mltplcty.size(); ++i) mltplcty[i] = 1. / m[i];
        }
        if (measure_correlation || measure_green_function) {
          ucorr.resize(label.size());
          scorr.resize(label.size());
          if (improved) {
            gucorr.resize(label.size());
            gscorr.resize(label.size());
            gfunc.resize(label.size());
          }
        }
        if (measure_correlation) {
          add_vector_obs(m, "Spin Correlations", label, is_signed);
          if (is_bipartite(lat))
            add_vector_obs(m, "Staggered Spin Correlations", label, is_signed);
          if (improved) {
            add_vector_obs(m, "Generalized Spin Correlations", label,
                           is_signed);
            if (is_bipartite(lat))
              add_vector_obs(m, "Generalized Staggered Spin Correlations",
                             label, is_signed);
          }
        }
        if (measure_green_function)
          add_vector_obs(m, "Green's Function", label, is_signed);
      }

      if (measure_structure_factor) {
        coordinate = alps::get_or_default(coordinate_t(), lat.rg(), 0);
        sfac.resize(num_sites(lat.rg()));
        add_vector_obs(m, "Spin Structure Factor", momenta_labels(lat),
                       is_signed);
      }
    }

    // improved estimator

    typedef typename dumb::template estimator<MC, LAT, TIME>::estimate
      estimate;
    void init_estimate(estimate const&) const {}

    typedef typename dumb::template estimator<MC, LAT, TIME>::collector
      collector;
    void init_collector(collector const&) const {}

    template<typename M, typename OP, typename FRAGMENT>
    void improved_measurement(M& m,
                              lattice_t const& lat,
                              double /* beta */, double sign,
                              std::vector<int> const& spins,
                              std::vector<OP> const& /* operators */,
                              std::vector<int> const& /* spins_c */,
                              std::vector<FRAGMENT> const& fragments,
                              collector const& /* coll */)
    {
      if ((measure_correlation || measure_green_function) && improved) {
        ucorr = 0;
        scorr = 0;
        gucorr = 0;
        gscorr = 0;
        gfunc = 0;
        if (origin) {
          typename virtual_site_iterator<lattice_t>::type si0, si0_end;
          for (boost::tie(si0, si0_end) = sites(lat, origin.get());
               si0 != si0_end; ++si0) {
            double us = 1-2*spins[*si0];
            double ss = gauge[*si0] * (1-2*spins[*si0]);
            double gss = gauge[*si0];
            typename virtual_site_iterator<lattice_t>::type si1, si1_end;
            for (boost::tie(si1, si1_end) = sites(lat.vg()); si1 != si1_end;
                 ++si1) {
              if (fragments[*si0].id == fragments[*si1].id) {
                int r1 = real_site[*si1];
                ucorr[r1] += us * (1-2*spins[*si1]);
                scorr[r1] += ss * gauge[*si1] * (1-2*spins[*si1]);
                gucorr[r1] += 1;
                gscorr[r1] += gss * gauge[*si1];
              }
            }
          }
          ucorr *= (0.25 * sign);
          scorr *= (0.25 * sign);
          gucorr *= (0.25 * sign);
          gscorr *= (0.25 * sign);
        } else {
          typename virtual_site_iterator<lattice_t>::type si0, si0_end;
          for (boost::tie(si0, si0_end) = sites(lat.vg()); si0 != si0_end;
               ++si0) {
            int r0 = real_site[*si0];
            double us = 1-2*spins[*si0];
            double ss = gauge[*si0] * (1-2*spins[*si0]);
            double gss = gauge[*si0];
            typename virtual_site_iterator<lattice_t>::type si1, si1_end;
            for (boost::tie(si1, si1_end) = sites(lat.vg()); si1 != si1_end;
                 ++si1) {
              if (fragments[*si0].id == fragments[*si1].id) {
                int d = distance(lat, r0, real_site[*si1]);
                ucorr[d] += us * (1-2*spins[*si1]);
                scorr[d] += ss * gauge[*si1] * (1-2*spins[*si1]);
                gucorr[d] += 1;
                gscorr[d] += gss * gauge[*si1];
              }
            }
          }
          ucorr *= (0.25 * sign * mltplcty);
          scorr *= (0.25 * sign * mltplcty);
          gucorr *= (0.25 * sign * mltplcty);
          gscorr *= (0.25 * sign * mltplcty);
        }
        if (measure_correlation) {
          m["Spin Correlations"] << ucorr;
          m["Generalized Spin Correlations"] << gucorr;
          if (is_bipartite(lat)) {
            m["Staggered Spin Correlations"] << scorr;
            m["Generalized Staggered Spin Correlations"] << gscorr;
          }
        }
      }
    }

    template<typename M, typename OP>
    void normal_measurement(M& m, lattice_t const& lat,
                            double /* beta */, double sign,
                            std::vector<int> const& spins,
                            std::vector<OP> const& /* operators */,
                            std::vector<int>& /* spins_c */)
    {
      if (measure_correlation && !improved) {
        ucorr = 0;
        scorr = 0;
        if (origin) {
          typename virtual_site_iterator<lattice_t>::type si0, si0_end;
          for (boost::tie(si0, si0_end) = sites(lat, origin.get());
               si0 != si0_end; ++si0) {
            double us = 1-2*spins[*si0];
            double ss = gauge[*si0] * (1-2*spins[*si0]);
            typename virtual_site_iterator<lattice_t>::type si1, si1_end;
            for (boost::tie(si1, si1_end) = sites(lat.rg()); si1 != si1_end;
                 ++si1) {
              int r1 = real_site[*si1];
              ucorr[r1] += us * (1-2*spins[*si1]);
              scorr[r1] += ss * gauge[*si1] * (1-2*spins[*si1]);
            }
          }
          ucorr *= 0.25 * sign;
          scorr *= 0.25 * sign;
        } else {
          typename virtual_site_iterator<lattice_t>::type si0, si0_end;
          for (boost::tie(si0, si0_end) = sites(lat.vg()); si0 != si0_end;
               ++si0) {
            int r0 = real_site[*si0];
            double us = 1-2*spins[*si0];
            double ss = gauge[*si0] * (1-2*spins[*si0]);
            typename virtual_site_iterator<lattice_t>::type si1, si1_end;
            for (boost::tie(si1, si1_end) = sites(lat.vg()); si1 != si1_end;
                 ++si1) {
              int d = distance(lat, r0, real_site[*si1]);
              ucorr[d] += us * (1-2*spins[*si1]);
              scorr[d] += ss * gauge[*si1] * (1-2*spins[*si1]);
            }
          }
          ucorr *= 0.25 * sign * mltplcty;
          scorr *= 0.25 * sign * mltplcty;
        }
        m["Spin Correlations"] << ucorr;
        if (is_bipartite(lat))
          m["Staggered Spin Correlations"] << scorr;
      }

      if (measure_structure_factor) {
        sfac = 0;
        int k = 0;
        typename momentum_iterator<lattice_t>::type mit, mit_end;
        for (boost::tie(mit, mit_end) = momenta(lat); mit != mit_end;
             ++mit, ++k) {
          std::complex<double> val;
          typename virtual_site_iterator<lattice_t>::type si, si_end;
          for (boost::tie(si, si_end) = sites(lat.vg()); si != si_end;
               ++si)
            val += (0.5-spins[*si])  * mit.phase(coordinate[real_site[*si]]);
          sfac[k] = sign * power2(val) / num_sites(lat.rg());
        }
        m["Spin Structure Factor"] << sfac;
      }
    }
  };

  typedef dumb::evaluator evaluator;
};

} // end namespace looper

#endif // LOOPER_CORRELATION_H
