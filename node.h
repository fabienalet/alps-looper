/**************************************************************************** 
*
* alps/looper: multi-cluster quantum Monte Carlo algorithm for spin systems
*              in path-integral and SSE representations
*
* $Id: node.h 438 2003-10-17 03:56:37Z wistaria $
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

#ifndef LOOPER_NODE_H
#define LOOPER_NODE_H

#include <alps/osiris.h>
#include <boost/integer_traits.hpp>
#include <boost/static_assert.hpp>

namespace looper {

template<class U = uint32_t>
class node_flag
{
public:
  typedef U uint_type;
  
  BOOST_STATIC_CONSTANT(uint_type, B_ADDD = 0);
  BOOST_STATIC_CONSTANT(uint_type, B_REFL = 1);
  BOOST_STATIC_CONSTANT(uint_type, B_FREZ = 2);
  BOOST_STATIC_CONSTANT(uint_type, B_ANTI = 3); // only for XYZ
  BOOST_STATIC_CONSTANT(uint_type, B_CONF = 4);
  
  // set for newly-added node
  BOOST_STATIC_CONSTANT(uint_type, M_ADDD = 1 << B_ADDD);
  
  BOOST_STATIC_CONSTANT(uint_type, M_REFL = 1 << B_REFL); 
  BOOST_STATIC_CONSTANT(uint_type, M_FREZ = 1 << B_FREZ);
  BOOST_STATIC_CONSTANT(uint_type, M_ANTI = 1 << B_ANTI); // only for XYZ
  BOOST_STATIC_CONSTANT(uint_type, M_CONF = 1 << B_CONF);
  
  // bit mask for clear()
  BOOST_STATIC_CONSTANT(uint_type, M_CLEAR = M_ANTI | M_CONF);
  
private:
  BOOST_STATIC_ASSERT((boost::integer_traits<uint_type>::is_integral));
  BOOST_STATIC_ASSERT((boost::integer_traits<uint_type>::const_min == 0));
};

template<class U = uint32_t>
class node_type : private node_flag<U>
{
public:
  typedef U uint_type;
  
  node_type() : type_(0) {}
  
  bool is_refl() const { return type_ & M_REFL; }
  bool is_frozen() const { return type_ & M_FREZ; }
  bool is_new_node() const { return type_ & M_ADDD; }
  bool is_old_node() const { return !is_new_node(); }
  
  uint_type refl() const { return ( type_ >> B_REFL ) & 1; }
  uint_type frozen() const { return ( type_ >> B_FREZ ) & 1; }
  uint_type new_node() const { return ( type_ >> B_ADDD ) & 1; }
  uint_type old_node() const { return 1 ^ (( type_ >> B_ADDD ) & 1); }
  
  uint_type phase() const { return ( type_ >> B_ANTI ) & 1; }
  uint_type conf() const { return (type_ >> B_CONF) & 1; }
  
  void set_type(uint_type t) { type_ = t; }
  void clear() { type_ &= M_CLEAR; }
  
  void set_conf(uint_type c) {
    type_ = ((0xffffffff ^ M_CONF) & type_) | (c << B_CONF); }
  void flip_conf(uint_type c = 1) { type_ ^= (c << B_CONF); }
  
  void set_new(uint_type is_refl, uint_type is_frozen, uint_type conf,
               uint_type phase) {
    type_ = M_ADDD | (is_refl << B_REFL) | (is_frozen << B_FREZ) 
      | (conf << B_CONF) | (phase << B_ANTI);
  }
  void set_old(uint_type is_refl, uint_type is_frozen) {
    type_ |= (is_refl << B_REFL) | (is_frozen << B_FREZ);
  }
  
  void output(std::ostream& os) const {
    os << "refl = " << refl()
       << " frozen = " << frozen()
       << " new_node = " << new_node()
       << " phase = " << phase()
       << " conf = " << conf();
  }
  
  void save(alps::ODump& od) const { od << type_; }
  void load(alps::IDump& id) { id >> type_; }
  
private:
  BOOST_STATIC_ASSERT((boost::integer_traits<uint_type>::is_integral));
  BOOST_STATIC_ASSERT((boost::integer_traits<uint_type>::const_min == 0));
  
  uint_type type_;
};

} // end namespace looper

#endif // LOOPER_NODE_H
