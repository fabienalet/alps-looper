/**************************************************************************** 
*
* alps/looper: multi-cluster quantum Monte Carlo algorithm for spin systems
*              in path-integral and SSE representations
*
* $Id: weight.h 455 2003-10-22 01:04:57Z wistaria $
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

#include <algorithm> // for std::max, std::min
#include <cmath> // for std::abs

namespace looper {

//
// default graph weights for path-integral and SSE loop algorithms
//

struct default_weight
{
  double density;
  double p_freeze;
  double p_accept_para;
  double p_accept_anti;
  double p_reflect;
  double offset;
  
  default_weight() : density(0), p_freeze(0), p_accept_para(0),
		     p_accept_anti(0), p_reflect(0), offset(0) {}
  template<class P>
  default_weight(const P& p) : density(0), p_freeze(0), p_accept_para(0),
			       p_accept_anti(0), p_reflect(0), offset(0)
  {
    using std::abs; using std::max;
    double Jxy = abs(p.Jxy); // ignore negative signs
    double Jz = p.Jz;
    if (Jxy + abs(Jz) > 1.0e-10) {
      density = max(abs(Jz) / 2, (Jxy + abs(Jz)) / 4);
      p_freeze = range_01(1 - Jxy / abs(Jz));
      p_accept_para = range_01((Jxy + Jz) / (Jxy + abs(Jz)));
      p_accept_anti = range_01((Jxy - Jz) / (Jxy + abs(Jz)));
      p_reflect = range_01((Jxy - Jz) / (2 * Jxy));
      offset = max(abs(Jz) / 4, Jxy / 4);
    }
  }
  
  double p_accept(int c0, int c1) const {
    return (c0 ^ c1) ? p_accept_anti : p_accept_para;
  }
  
protected:
  double range_01(double x) const {
    return std::min(std::max(x, 0.), 1.);
  }
};
  
} // namespace looper

#endif // LOOPER_WEIGHT_H