/**************************************************************************** 
*
* alps/looper: multi-cluster quantum Monte Carlo algorithm for spin systems
*              in path-integral and SSE representations
*
* $Id: weight.h 476 2003-10-29 10:16:12Z wistaria $
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

#ifndef LOOPER_WEIGHT_H
#define LOOPER_WEIGHT_H

#include <looper/random_choice.h>

#include <algorithm> // for std::max, std::min
#include <cmath> // for std::abs

namespace looper {

//
// default graph weights for path-integral and SSE loop algorithms
//

class default_weight
{
public:
  default_weight() : density_(0), p_freeze_(0), pa_para_(0),
		     pa_anti_(0), p_reflect_(0), offset_(0) {}
  template<class P>
  default_weight(const P& p) : density_(0), p_freeze_(0), pa_para_(0),
			       pa_anti_(0), p_reflect_(0), offset_(0)
  {
    using std::abs; using std::max;
    double Jxy = abs(p.jxy()); // ignore negative signs
    double Jz = p.jz();
    if (Jxy + abs(Jz) > 1.0e-10) {
      density_ = max(abs(Jz) / 2, (Jxy + abs(Jz)) / 4);
      p_freeze_ = range_01(1 - Jxy / abs(Jz));
      pa_para_ = range_01((Jxy + Jz) / (Jxy + abs(Jz)));
      pa_anti_ = range_01((Jxy - Jz) / (Jxy + abs(Jz)));
      p_reflect_ = range_01((Jxy - Jz) / (2 * Jxy));
      offset_ = max(abs(Jz) / 4, Jxy / 4);
    }
  }

  double density() const { return density_; }
  double weight() const { return density_; }
  double p_freeze() const { return p_freeze_; }
  double p_accept_para() const { return pa_para_; }
  double p_accept_anti() const { return pa_anti_; }
  double p_accept(int c0, int c1) const {
    return (c0 ^ c1) ? pa_anti_ : pa_para_;
  }
  double p_reflect() const { return p_reflect_; }
  double offset() const { return offset_; }

protected:
  double range_01(double x) const {
    return std::min(std::max(x, 0.), 1.);
  }

private:
  double density_;
  double p_freeze_;
  double pa_para_;
  double pa_anti_;
  double p_reflect_;
  double offset_;
};
  

class uniform_bond_chooser
{
public:
  uniform_bond_chooser() : n_() {}
  template<class GRAPH, class MODEL>
  uniform_bond_chooser(const GRAPH& vg, const MODEL& m) : n_()
  {
    assert(m.num_bond_types() == 1);
    n_ = double(boost::num_edges(vg.graph));
  }

  template<class RNG>
  int choose(RNG& rng) const { return n_ * rng(); }

private:
  double n_;
};

class bond_chooser
{
public:
  bond_chooser() : rc_() {}
  template<class G, class M>
  bond_chooser(const G& vg, const M& m) : rc_() { init (vg, m); }
  template<class G, class M, class W>
  bond_chooser(const G& vg, const M& m, const W& w) : rc_() { init(vg, m, w); }

  template<class G, class M>
  void init(const G& vg, const M& m) { init(vg, m, default_weight()); }
  template<class G, class M, class W>
  void init(const G& vg, const M& m, const W&)
  {
    std::vector<double> w(0);
    typename boost::graph_traits<typename G::graph_type>::edge_iterator
      ei, ei_end;
    for (boost::tie(ei, ei_end) = boost::edges(vg.graph); ei != ei_end; ++ei)
      w.push_back(
        W(m.bond(boost::get(edge_type_t(), vg.graph, *ei))).weight());
    rc_.init(w);
  }

  template<class RNG>
  int choose(RNG& rng) const { return rc_(rng); }

private:
  random_choice<> rc_;
};

} // namespace looper

#endif // LOOPER_WEIGHT_H
