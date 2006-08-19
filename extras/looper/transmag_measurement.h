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

#ifndef TRANSMAG_MEASUREMENT_H
#define TRANSMAG_MEASUREMENT_H

#include <looper/measurement.h>
#include <alps/alea.h>
#include <cmath>
#include <string>

template<typename VLAT, typename TIME>
struct transverse_magnetization_estimator
{
  typedef VLAT virtual_lattice_t;
  typedef TIME time_t;

  template<typename M>
  void initialize(M& m, alps::Parameters const& /* params */,
                  virtual_lattice_t const& /* vlat */,
                  bool /* is_bipartite */, bool is_signed,
                  bool use_improved_estimator)
  {
    if (use_improved_estimator) {
      looper::add_measurement(m, "Transverse Magnetization",
                              is_signed);
      looper::add_measurement(m, "Transverse Magnetization Density",
                              is_signed);
    }
  }

  static void evaluate(alps::ObservableSet& /* m */,
                       alps::Parameters const& /* params */,
                       alps::ObservableSet const& /* m_in */) {}

  // improved estimator

  struct estimate
  {
    double length;
    bool closed;
    void init()
    {
      length = 0;
      closed = true;
    }
    void start_s(virtual_lattice_t const&, double t, int, int)
    {
      length -= t;
      closed = false;
    }
    void start_b(virtual_lattice_t const&, double t, int, int, int)
    { length -= t; }
    void term_s(virtual_lattice_t const&, double t, int, int)
    {
      length += t;
      closed = false;
    }
    void term_b(virtual_lattice_t const&, double t, int, int, int)
    { length += t; }
    void at_bot(virtual_lattice_t const&, double t, int, int)
    { length -= t; }
    void at_top(virtual_lattice_t const&, double t, int, int)
    { length += t; }
  };
  void init_estimate(estimate& es) const { es.init(); }

  template<typename QMC, typename IMPROVE>
  struct collector
  {
    double length;
    collector() : length(0) {}
    template<typename EST>
    collector operator+(EST const& cm)
    {
      if (!cm.closed) length += cm.length;
      return *this;
    }
    template<typename M>
    void commit(M& m, virtual_lattice_t const& vlat, bool, double,
                int, double sign) const
    {
      m["Transverse Magnetization"] << 0.5 * sign * length;
      m["Transverse Magnetization Density"] << 0.5 * sign * length /
        num_sites(vlat.rgraph());
    }
  };

  // normal estimator

  template<typename QMC, typename M, typename OP>
  void measure(M& /* m */,
               virtual_lattice_t const& /* vlat */,
               bool /* is_bipartite */,
               bool /* use_imporved_estimator */,
               double /* beta */,
               double /* sign */,
               std::vector<int> const& /* spins */,
               std::vector<OP> const& /* operators */,
               std::vector<int> const& /* spins_c */) {}
};

#endif // TRANSMAG_MEASUREMENT_H