/*****************************************************************************
*
* ALPS/looper: multi-cluster quantum Monte Carlo algorithms for spin systems
*
* Copyright (C) 1997-2008 by Synge Todo <wistaria@comp-phys.org>
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

#include "loop_config.h"
#include "loop_factory.h"
#include <looper/cluster.h>
#include <looper/histogram.h>
#include <looper/montecarlo.h>
#include <looper/operator.h>
#include <looper/permutation.h>
#include <looper/type.h>
#include <looper/union_find.h>
#include <alps/plot.h>
#include <boost/regex.hpp>

#ifndef LOOPER_ONLY_PATH_INTEGRAL

namespace {

class loop_worker : private loop_config {
public:
  typedef looper::sse mc_type;

  typedef int time_t;
  typedef looper::local_operator<mc_type, loop_graph_t, time_t> local_operator_t;
  typedef std::vector<local_operator_t> operator_string_t;
  typedef operator_string_t::iterator operator_iterator;

  typedef looper::union_find::node cluster_fragment_t;
  typedef looper::cluster_info cluster_info_t;

  typedef looper::estimator<measurement_set, mc_type, lattice_t, time_t>::type estimator_t;

  loop_worker(alps::Parameters const& p, alps::ObservableSet& obs);

  bool is_thermalized() const { return true; }
  double progress() const { return mcs.progress(); }

  template<typename ENGINE>
  void run(ENGINE& eng, alps::ObservableSet& obs);

  void set_beta(double) const {
    boost::throw_exception(std::logic_error("sse_qwl"));
  }
  double weight_parameter() const {
    boost::throw_exception(std::logic_error("sse_qwl"));
    return 0;
  }
  static double log_weight(double, double) {
    boost::throw_exception(std::logic_error("sse_qwl"));
    return 0;
  }

  void save(alps::ODump& dp) const {
    dp << mcs << spins << operators << logf << histogram << histobs;
  }
  void load(alps::IDump& dp) {
    dp >> mcs >> spins >> operators >> logf >> histogram >> histobs;
  }

protected:
  template<typename ENGINE>
  void build(ENGINE& eng);

  template<typename ENGINE, typename FIELD, typename SIGN, typename IMPROVE>
  void flip(ENGINE& eng, alps::ObservableSet& obs);

private:
  // helpers
  lattice_t lattice;
  model_t model;

  // parameters
  bool use_improved_estimator;
  // Wang-Landau parameters
  alps::integer_range<int> exp_range;
  bool store_all_histograms;
  int min_visit;
  double flatness;

  // configuration (checkpoint)
  looper::wl_steps mcs;
  std::vector<int> spins;
  std::vector<local_operator_t> operators;
  // Wang-Landau configuration (checkpoint)
  double logf;

  // observables
  double sign;
  looper::wl_histogram histogram;
  looper::histogram_set<double> histobs;
  estimator_t estimator;

  // working vectors
  std::vector<int> spins_c;
  std::vector<local_operator_t> operators_p;
  std::vector<cluster_fragment_t> fragments;
  std::vector<int> current;
  std::vector<bool> to_flip;
  std::vector<cluster_info_t> clusters;
  std::vector<looper::estimate<estimator_t>::type> estimates;
  std::vector<int> perm;
};


//
// member functions of loop_worker
//

loop_worker::loop_worker(alps::Parameters const& p, alps::ObservableSet& obs) :
  lattice(p), model(p, lattice, /* is_path_integral = */ false),
  exp_range(p.value_or_default("EXPANSION_RANGE", "[0:500]")),
  mcs(p, exp_range), histogram(exp_range), histobs(exp_range) {

  model.check_parameter(/* support_longitudinal_field = */ false, support_negative_sign);

  use_improved_estimator = !model.has_field() && !p.defined("DISABLE_IMPROVED_ESTIMATOR");
  if (!use_improved_estimator)
    std::cout << "WARNING: improved estimator is disabled\n";

  // Wang Landau parameters
  if (exp_range.min() < 0)
    boost::throw_exception(std::invalid_argument(
      "minimum of expansion order must not be negative"));
  double f =
    p.value_or_default("INITIAL_MODIFICATION_FACTOR", mcs.use_zhou_bhatt() ? std::exp(1.) :
      std::exp(exp_range.max() * std::log(1.*num_sites(lattice.vg())) / mcs.block()));
  if (f <= 1)
    boost::throw_exception(std::invalid_argument(
      "initial modification factor must be larger than 1"));
  logf = std::log(f);
  if (mcs.use_zhou_bhatt()) {
    min_visit = static_cast<int>(1 / logf);
    flatness = p.value_or_default("FLATNESS_THRESHOLD", -1.);
  } else {
    min_visit = 0;
    flatness = p.value_or_default("FLATNESS_THRESHOLD", 0.2);
  }

  // configuration
  int nvs = num_sites(lattice.vg());
  spins.resize(nvs); std::fill(spins.begin(), spins.end(), 0 /* all up */);
  spins_c.resize(nvs);
  current.resize(nvs);
  perm.resize(max_virtual_sites(lattice));

  // measurements
  store_all_histograms =  p.defined("STORE_ALL_HISTOGRAMS");
  obs << make_observable(alps::SimpleRealObservable("Volume"));
  obs << make_observable(alps::SimpleRealObservable("Number of Sites"));
  obs << make_observable(alps::SimpleRealObservable("Energy Offset"));
  obs << make_observable(alps::SimpleRealVectorObservable("Partition Function Coefficient"));
  obs << make_observable(alps::SimpleRealVectorObservable("Histogram"));
  if (store_all_histograms) {
    for (int p = 0; p < mcs.iterations(); ++p) {
      std::string suffix = "(iteration #" + boost::lexical_cast<std::string>(p) + ")";
      obs << make_observable(alps::SimpleRealVectorObservable(
        "Partition Function Coefficient " + suffix));
      obs<< make_observable(alps::SimpleRealVectorObservable("Histogram " + suffix));
    }
  }
  obs.reset(true);
  if (model.is_signed()) histobs.add_histogram("Sign");
  estimator.initialize(histobs, p, lattice, model.is_signed(), use_improved_estimator);
}

template<typename ENGINE>
void loop_worker::run(ENGINE& eng, alps::ObservableSet& obs) {
  // if (!mcs.can_work()) return;
  ++mcs;

  build(eng);

  //           FIELD               SIGN                IMPROVE
  flip<ENGINE, boost::mpl::true_,  boost::mpl::true_,  boost::mpl::true_ >(eng, obs);
  flip<ENGINE, boost::mpl::true_,  boost::mpl::true_,  boost::mpl::false_>(eng, obs);
  flip<ENGINE, boost::mpl::true_,  boost::mpl::false_, boost::mpl::true_ >(eng, obs);
  flip<ENGINE, boost::mpl::true_,  boost::mpl::false_, boost::mpl::false_>(eng, obs);
  flip<ENGINE, boost::mpl::false_, boost::mpl::true_,  boost::mpl::true_ >(eng, obs);
  flip<ENGINE, boost::mpl::false_, boost::mpl::true_,  boost::mpl::false_>(eng, obs);
  flip<ENGINE, boost::mpl::false_, boost::mpl::false_, boost::mpl::true_ >(eng, obs);
  flip<ENGINE, boost::mpl::false_, boost::mpl::false_, boost::mpl::false_>(eng, obs);

  if (!mcs.doing_multicanonical() && mcs() == mcs.block()) {
    if (histogram.check_flatness(flatness) && histogram.check_visit(min_visit)) {
      std::cout << "stage " << mcs.stage() << ": histogram becomes flat\n";
      histogram.subtract();
      if (store_all_histograms) {
        std::string suffix = "(iteration #" + boost::lexical_cast<std::string>(mcs.stage()) + ")";
        histogram.store(obs, "Partition Function Coefficient " + suffix,
          "Histogram " + suffix, mcs.doing_multicanonical());
      }
      logf = 0.5 * logf;
      if (mcs.use_zhou_bhatt()) min_visit *= 2;
      histogram.clear();
      mcs.next_stage();
    } else {
      std::cout << "stage " << mcs.stage() << ": histogram is not flat yet\n";
      histogram.subtract();
      mcs.reset_stage();
    }
  }
  if (mcs.doing_multicanonical() && mcs() == mcs.sweeps())
    histogram.store(obs, "Partition Function Coefficient", "Histogram",
      mcs.doing_multicanonical());
}


//
// diagonal update and cluster construction
//

template<typename ENGINE>
void loop_worker::build(ENGINE& eng) {
  // initialize spin & operator information
  int nop = operators.size();
  std::copy(spins.begin(), spins.end(), spins_c.begin());
  std::swap(operators, operators_p); operators.resize(0);

  // initialize cluster information (setup cluster fragments)
  int nvs = num_sites(lattice.vg());
  fragments.resize(0); fragments.resize(nvs);
  for (int s = 0; s < nvs; ++s) current[s] = s;

  boost::variate_generator<ENGINE&, boost::uniform_real<> >
    r_uniform(eng, boost::uniform_real<>());
  double bw = model.graph_weight();
  bool try_gap = true;
  for (operator_iterator opi = operators_p.begin(); try_gap || opi != operators_p.end();) {

    // diagonal update & labeling
    if (try_gap) {
      if ((nop+1) * r_uniform() < bw * histogram.accept_rate(nop, nop+1)) {
        loop_graph_t g = model.choose_graph(r_uniform);
        if ((is_bond(g) && is_compatible(g, spins_c[source(pos(g), lattice.vg())],
                                            spins_c[target(pos(g), lattice.vg())])) ||
            (is_site(g) && is_compatible(g, spins_c[pos(g)]))) {
          operators.push_back(local_operator_t(g));
          ++nop;
        } else {
          try_gap = false;
          histogram.visit(nop, logf, !mcs.doing_multicanonical());
          continue;
        }
      } else {
        try_gap = false;
        histogram.visit(nop, logf, !mcs.doing_multicanonical());
        continue;
      }
    } else {
      if (opi->is_diagonal()) {
        if (bw * r_uniform() < nop * histogram.accept_rate(nop, nop-1)) {
          --nop;
          ++opi;
          histogram.visit(nop, logf, !mcs.doing_multicanonical());
          continue;
        } else {
          if (opi->is_site()) {
            opi->assign_graph(model.choose_diagonal(r_uniform, opi->loc(), spins_c[opi->pos()]));
          } else {
            opi->assign_graph(model.choose_diagonal(r_uniform, opi->loc(),
              spins_c[source(opi->pos(), lattice.vg())],
              spins_c[target(opi->pos(), lattice.vg())]));
          }
        }
      } else {
        if (opi->is_bond())
          opi->assign_graph(model.choose_offdiagonal(r_uniform, opi->loc(),
            spins_c[source(opi->pos(), lattice.vg())],
            spins_c[target(opi->pos(), lattice.vg())]));
      }
      operators.push_back(*opi);
      ++opi;
      try_gap = true;
    }
    histogram.visit(nop, logf, !mcs.doing_multicanonical());

    operator_iterator oi = operators.end() - 1;
    if (oi->is_bond()) {
      int s0 = source(oi->pos(), lattice.vg());
      int s1 = target(oi->pos(), lattice.vg());
      if (oi->is_offdiagonal()) {
        spins_c[s0] ^= 1;
        spins_c[s1] ^= 1;
      }
      boost::tie(current[s0], current[s1], oi->loop0, oi->loop1) =
        reconnect(fragments, oi->graph(), current[s0], current[s1]);
    } else {
      int s = oi->pos();
      if (oi->is_offdiagonal()) spins_c[s] ^= 1;
      boost::tie(current[s], oi->loop0, oi->loop1) = reconnect(fragments, oi->graph(), current[s]);
    }
  }

  // symmetrize spins
  if (max_virtual_sites(lattice) == 1) {
    for (int i = 0; i < nvs; ++i) unify(fragments, i, current[i]);
  } else {
    BOOST_FOREACH(looper::real_site_descriptor<lattice_t>::type rs, sites(lattice.rg())) {
      looper::virtual_site_iterator<lattice_t>::type vsi, vsi_end;
      boost::tie(vsi, vsi_end) = sites(lattice, rs);
      int offset = *vsi;
      int s2 = *vsi_end - *vsi;
      for (int i = 0; i < s2; ++i) perm[i] = i;
      looper::partitioned_random_shuffle(perm.begin(), perm.begin() + s2,
        spins.begin() + offset, spins_c.begin() + offset, r_uniform);
      for (int i = 0; i < s2; ++i) unify(fragments, offset+i, current[offset+perm[i]]);
    }
  }
}


//
// cluster flip
//

template<typename ENGINE, typename FIELD, typename SIGN, typename IMPROVE>
void loop_worker::flip(ENGINE& eng, alps::ObservableSet& /* obs */) {
  if (model.is_signed() != SIGN() ||
      use_improved_estimator != IMPROVE()) return;

  boost::variate_generator<ENGINE&, boost::uniform_real<> >
    r_uniform(eng, boost::uniform_real<>());

  int nvs = num_sites(lattice.vg());
  int nop = operators.size();

  // assign cluster id
  int nc = 0;
  BOOST_FOREACH(cluster_fragment_t& f, fragments) if (f.is_root()) f.set_id(nc++);
  BOOST_FOREACH(cluster_fragment_t& f, fragments) f.set_id(cluster_id(fragments, f));
  to_flip.resize(nc);
  clusters.resize(0); clusters.resize(nc);

  std::copy(spins.begin(), spins.end(), spins_c.begin());
  cluster_info_t::accumulator<cluster_fragment_t, FIELD, SIGN, IMPROVE>
    weight(clusters, fragments, model.field(), model.bond_sign(), model.site_sign());
  looper::accumulator<estimator_t, cluster_fragment_t, IMPROVE>
    accum(estimates, nc, lattice, estimator, fragments);
  for (unsigned int s = 0; s < nvs; ++s) {
    weight.start_bottom(s, time_t(0), s, spins_c[s]);
    accum.start_bottom(s, time_t(0), s, spins_c[s]);
  }
  int t = 0;
  int negop = 0; // number of operators with negative weights
  BOOST_FOREACH(local_operator_t& op, operators) {
    if (op.is_bond()) {
      if (!op.is_frozen_bond_graph()) {
        int b = op.pos();
        int s0 = source(b, lattice.vg());
        int s1 = target(b, lattice.vg());
        weight.end_b(op.loop_l0(), op.loop_l1(), t, b, s0, s1, spins_c[s0], spins_c[s1]);
        accum.end_b(op.loop_l0(), op.loop_l1(), t, b, s0, s1, spins_c[s0], spins_c[s1]);
        if (op.is_offdiagonal()) {
          spins_c[s0] ^= 1;
          spins_c[s1] ^= 1;
          if (SIGN()) negop += model.bond_sign(op.pos());
        }
        weight.begin_b(op.loop_u0(), op.loop_u1(), t, b, s0, s1, spins_c[s0], spins_c[s1]);
        accum.begin_b(op.loop_u0(), op.loop_u1(), t, b, s0, s1, spins_c[s0], spins_c[s1]);
      }
    } else {
      if (!op.is_frozen_site_graph()) {
        int s = op.pos();
        weight.end_s(op.loop_l(), t, s, spins_c[s]);
        accum.end_s(op.loop_l(), t, s, spins_c[s]);
        if (op.is_offdiagonal()) {
          spins_c[s] ^= 1;
          if (SIGN()) negop += model.site_sign(op.pos());
        }
        weight.begin_s(op.loop_u(), t, s, spins_c[s]);
        accum.begin_s(op.loop_u(), t, s, spins_c[s]);
      }
    }
    ++t;
  }
  for (unsigned int s = 0; s < nvs; ++s) {
    weight.stop_top(current[s], time_t(nop), s, spins_c[s]);
    accum.stop_top(current[s], time_t(nop), s, spins_c[s]);
  }
  sign = ((negop & 1) == 1) ? -1 : 1;

  // accumulate cluster properties
  typename looper::collector<estimator_t>::type coll = get_collector(estimator);
  if (IMPROVE())
    BOOST_FOREACH(looper::estimate<estimator_t>::type const& est, estimates) { coll += est; }

  // determine whether clusters are flipped or not
  double improved_sign = sign;
  for (unsigned int c = 0; c < clusters.size(); ++c) {
    to_flip[c] = ((2*r_uniform()-1) < 0);
    if (SIGN() && IMPROVE() && (clusters[c].sign & 1) == 1) improved_sign = 0;
  }

  // improved measurement
  if (IMPROVE())
    estimator.improved_measurement(histobs, lattice, 1, improved_sign, spins, operators,
      spins_c, fragments, coll);

  // flip operators & spins
  BOOST_FOREACH(local_operator_t& op, operators)
    if (to_flip[fragments[op.loop_0()].id()] ^ to_flip[fragments[op.loop_1()].id()]) op.flip();
  for (int s = 0; s < nvs; ++s) if (to_flip[fragments[s].id()]) spins[s] ^= 1;

  //
  // measurement
  //

  obs["Volume"] << (double)lattice.volume();
  obs["Number of Sites"] << (double)num_sites(lattice.rg());
  obs["Energy Offset"] << model.energy_offset();
  histobs.set_position(nop);

  // sign
  if (SIGN()) {
    if (IMPROVE())
      histobs["Sign"] << improved_sign;
    else
      histobs["Sign"] << sign;
  }

  // normal measurement
  estimator.normal_measurement(histobs, lattice, 1, sign, spins, operators, spins_c);
}


//
// dynamic registration to the factories
//

const bool worker_registered =
  loop_factory::instance()->register_worker<loop_worker>("SSE QWL");

} // end namespace

#endif // LOOPER_ONLY_PATH_INTEGRAL
