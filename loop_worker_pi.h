/*****************************************************************************
*
* ALPS/looper: multi-cluster quantum Monte Carlo algorithms for spin systems
*
* Copyright (C) 1997-2005 by Synge Todo <wistaria@comp-phys.org>
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

#ifndef LOOP_WORKER_PI_H
#define LOOP_WORKER_PI_H

#include "loop_worker.h"

struct cluster_info_pi
{
  cluster_info_pi(bool t = false)
    : to_flip(t), mag0(0), size(0), mag(0), length(0) {}
  bool to_flip;
  int mag0;
  int size;
  double mag;
  double length;
};

class qmc_worker_pi : public qmc_worker_base
{
public:
  typedef looper::path_integral             qmc_type;
  typedef qmc_worker_base                   super_type;
  typedef looper::local_operator<qmc_type>  local_operator;
  typedef looper::union_find::node_idx      cluster_fragment;
  typedef cluster_info_pi                   cluster_info;

  qmc_worker_pi(const alps::ProcessList& w, const alps::Parameters& p, int n);

  void dostep();

  void save(alps::ODump& dump) const {
    super_type::save(dump);
    dump << spins << operators;
  }
  void load(alps::IDump& dump) {
    super_type::load(dump);
    dump >> spins >> operators;
  }

private:
  int nrs; // number of real sites
  int nvs; // number of virtual sites
  double beta;

  std::vector<int> spins;
  std::vector<local_operator> operators;

  // working area
  std::vector<int> spins_c;
  std::vector<local_operator> operators_p;
  std::vector<cluster_fragment> fragments;
  std::vector<int> current;
  std::vector<cluster_info> clusters;
};

#endif // LOOP_WORKER_PI_H
