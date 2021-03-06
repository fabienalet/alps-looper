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

#include <alps/config.h>
#if defined(LOOPER_ENABLE_OPENMP) && defined(ALPS_ENABLE_OPENMP_WORKER) && !defined(LOOPER_OPENMP)
# define LOOPER_OPENMP
#endif

#include "loop_config.h"
#include <looper/model_impl.h>

//
// explicit instantiation of spinmodel_helper::init() member function
//


template
void looper::spinmodel_helper<loop_config::lattice_graph_t, loop_config::loop_graph_t>::
  init(alps::Parameters const& p, looper::lattice_helper<loop_config::lattice_graph_t>& lat,
  bool is_path_integral);

#ifdef LOOPER_OPENMP
template
void looper::spinmodel_helper<loop_config::lattice_graph_t, loop_config::loop_graph_t>::
  init(alps::Parameters const& p, looper::lattice_helper<loop_config::lattice_graph_t>& lat,
  looper::simple_lattice_sharing& sharing, bool is_path_integral);
#endif
