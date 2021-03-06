/*****************************************************************************
*
* ALPS Project Applications
*
* Copyright (C) 1997-2005 by Synge Todo <wistaria@comp-phys.org>
*
* This software is part of the ALPS Applications, published under the ALPS
* Application License; you can use, redistribute it and/or modify it under
* the terms of the license, either version 1 or (at your option) any later
* version.
* 
* You should have received a copy of the ALPS Application License along with
* the ALPS Applications; see the file LICENSE.txt. If not, the license is also
* available from http://alps.comp-phys.org/.
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

#ifndef LOOPER_QMC_H
#define LOOPER_QMC_H

namespace looper {

template<class G, class MP>
struct qmc_parameter_base
{
  typedef G                           graph_type;
  typedef virtual_mapping<graph_type> mapping_type;
  typedef MP                          model_parameter_type;

  qmc_parameter_base(const graph_type& g, const model_parameter_type& mp,
                     double b)
    : rgraph(g), model(mp), vgraph(), vmap(), beta(b),
      is_bipartite(false), sz_conserved(true)
  {
    generate_virtual_graph(g, mp, vgraph, vmap);
    is_bipartite = alps::set_parity(vgraph);
  }

  const graph_type&           rgraph;
  const model_parameter_type& model;
  graph_type                  vgraph;
  mapping_type                vmap;
  double                      beta;
  bool                        is_bipartite;
  bool                        sz_conserved;
};

} // namespace looper

#endif // LOOPER_QMC_H
