/**************************************************************************** 
*
* alps/looper: multi-cluster quantum Monte Carlo algorithm for spin systems
*              in path-integral and SSE representations
*
* $Id: random.h 409 2003-10-10 10:06:40Z wistaria $
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

#ifndef LOOPER_RANDOM_H
#define LOOPER_RANDOM_H

#include <boost/random.hpp>
#include <boost/throw_exception.hpp>
#include <stdexcept>

namespace looper {

// fill duration [0,tmax] uniformly with density r

template<class RNG01, class ARRAY>
void fill_duration(RNG01& random_01, ARRAY& array, 
		   typename ARRAY::value_type r,
		   typename ARRAY::value_type tmax)
{
  if (r <= 0 || tmax < 0)
    boost::throw_exception(std::invalid_argument("invalid argument"));

  boost::exponential_distribution<typename ARRAY::value_type> exp_rng(r);
  array.clear();
  typename ARRAY::value_type t(0);
  while (true) {
    t += exp_rng(random_01);
    if (t >= tmax) break;
    array.push_back(t);
  }
}

} // end namespace looper

#endif // LOOPER_RANDOM_H
