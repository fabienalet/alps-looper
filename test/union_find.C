/*****************************************************************************
*
* ALPS/looper: multi-cluster quantum Monte Carlo algorithms for spin systems
*
* Copyright (C) 1997-2009 by Synge Todo <wistaria@comp-phys.org>
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

#include <looper/union_find.h>
#include <cmath>
#include <boost/random.hpp>
#include <iostream>

#ifdef BOOST_NO_ARGUMENT_DEPENDENT_LOOKUP
using namespace looper::union_find;
#endif

const int n = 100;

int main()
{
  // random number generator
  boost::mt19937 eng(29833u);
  boost::variate_generator<boost::mt19937&, boost::uniform_real<> >
    rng(eng, boost::uniform_real<>());

  std::cout << "[[union find test]]\n";

  std::vector<looper::union_find::node> nodes(n);
  std::vector<looper::union_find::node_noweight> nodes_noweight(n);

  std::cout << "\n[making tree]\n";

  for (int i = 0; i < n; i++) {
    int i0 = static_cast<int>(n * rng());
    int i1 = static_cast<int>(n * rng());
    std::cout << "connecting node " << i0 << " to node " << i1 << std::endl;
    looper::union_find::unify(nodes, i0, i1);
    looper::union_find::unify(nodes_noweight, i0, i1);
  }

  std::cout << "\n[results]\n";

  for (int i = 0; i < n; i++) {
    if (nodes[i].is_root())
      std::cout << "node " << i << " is root and tree size is " << nodes[i].weight() << std::endl;
    else
      std::cout << "node " << i << "'s parent is " << nodes[i].parent()
                << " and its root is " << root_index(nodes, i) << std::endl;
  }

  for (int i = 0; i < n; i++) {
    if (nodes[i].is_root())
      std::cout << "node " << i << " is root\n";
    else
      std::cout << "node " << i << "'s parent is " << nodes[i].parent()
                << " and its root is " << root_index(nodes, i) << std::endl;
  }
}
