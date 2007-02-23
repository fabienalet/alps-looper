/*****************************************************************************
*
* ALPS/looper: multi-cluster quantum Monte Carlo algorithms for spin systems
*
* Copyright (C) 1997-2007 by Synge Todo <wistaria@comp-phys.org>
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

#ifndef LOOPER_MODEL_IMPL_H
#define LOOPER_MODEL_IMPL_H

#include "model.h"
#include "model_parameter.h"
#include "weight.h"

#include <alps/model.h>
#include <boost/foreach.hpp>

namespace looper {

template<typename RG, typename LG>
void spinmodel_helper<RG, LG>::init(alps::Parameters const& p, lattice_helper<RG>& lat,
  bool is_path_integral) {
  typedef lattice_helper<RG> lattice_t;
  alps::model_helper<short> mh(lat.graph_helper(), p);
  model_parameter mp(p, lat.graph_helper(), mh);
  quantal_ = mp.is_quantal();
  frustrated_ = mp.is_frustrated();

  // virtual graph
  lat.generate_virtual_graph(mp, mp.has_d_term());

  // weight table
  weight_table wt(mp, lat, p.value_or_default("FORCE_SCATTER", frustrated_ ? 0.1 : 0.));
  chooser_.init(wt, is_path_integral);

  // signs
  site_sign_.clear();
  bond_sign_.clear();
  signed_ = mp.is_signed();
  if (signed_) {
    site_sign_.resize(num_sites(lat.vg()));
    BOOST_FOREACH(weight_table::site_weight_t const& s, wt.site_weights())
      site_sign_[s.first] = (s.second.sign < 0) ? 1 : 0;
    bond_sign_.resize(num_bonds(lat.vg()));
    BOOST_FOREACH(weight_table::bond_weight_t const& b, wt.bond_weights())
      bond_sign_[b.first] = (b.second.sign < 0) ? 1 : 0;
  }

  // external longitudinal field
  field_.clear();
  if (mp.has_field()) {
    field_.resize(num_sites(lat.vg()));
    BOOST_FOREACH(typename real_site_descriptor<lattice_t>::type rs, sites(lat.rg()))
      BOOST_FOREACH(typename virtual_site_descriptor<lattice_t>::type vs, sites(lat, rs))
        field_[vs] = mp.site(rs, lat.rg()).hz;
  }

  // energy offset
  offset_ = mp.energy_offset() + wt.energy_offset();
}

} // end namespace looper

#endif // LOOPER_MODEL_IMPL_H
