/**************************************************************************** 
*
* alps/looper: multi-cluster quantum Monte Carlo algorithm for spin systems
*              in path-integral and SSE representations
*
* $Id: sse.h 453 2003-10-21 06:07:38Z wistaria $
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

#ifndef LOOPER_SSE_H
#define LOOPER_SSE_H

#include "permutation.h"
#include "union_find.h"
#include "virtual_graph.h"
#include "weight.h"
#include <boost/throw_exception.hpp>
#include <cmath>
#include <stdexcept>

namespace looper {

class sse_node;

template<class G, class M, class W = default_weight> struct sse;

template<class G, class M, class W>
struct sse<virtual_graph<G>, M, W>
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

  static double energy_offset(const vg_type& vg, const model_type& model)
  {
    double offset = 0;
    typename alps::property_map<alps::bond_type_t, graph_type, int>::const_type
      bond_type(alps::get_or_default(alps::bond_type_t(), vg.graph, 0));
    edge_iterator ei_end = boost::edges(vg.graph).second;
    for (edge_iterator ei = boost::edges(vg.graph).first; ei != ei_end; ++ei)
      offset += model.bond(bond_type[*ei]).C +
	weight_type(model.bond(bond_type[*ei])).offset;
    return offset;
  }
}; // struct sse

} // end namespace looper

#endif // LOOPER_SSE_H
