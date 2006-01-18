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

#include <loop_worker.h>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/throw_exception.hpp>
#include <map>
#include <stdexcept>

class abstract_worker_creator
{
public:
  virtual ~abstract_worker_creator() {}
  virtual alps::scheduler::MCRun* create(const alps::ProcessList& w,
    const alps::Parameters& p, int n) const = 0;
};

template <typename WORKER>
class worker_creator : public abstract_worker_creator
{
public:
  typedef WORKER worker_type;
  virtual ~worker_creator() {}
  alps::scheduler::MCRun* create(const alps::ProcessList& w,
    const alps::Parameters& p, int n) const
  { return new worker_type(w, p, n); }
};

class qmc_factory
  : public alps::scheduler::Factory, private boost::noncopyable
{
public:
  alps::scheduler::MCSimulation* make_task(const alps::ProcessList& w,
    const boost::filesystem::path& fn) const;
  alps::scheduler::MCSimulation* make_task(const alps::ProcessList& w,
    const boost::filesystem::path& fn, const alps::Parameters&) const;
  alps::scheduler::MCRun* make_worker(const alps::ProcessList& w,
                                      const alps::Parameters& p, int n) const;

  void print_copyright(std::ostream& os) const;

  template<typename WORKER>
  bool register_worker(std::string const& name)
  {
    bool isnew = (creators_.find(name) == creators_.end());
    creators_[name] = pointer_type(new worker_creator<WORKER>());
    return isnew;
  }

  bool unregister_worker(std::string const& name)
  {
    map_type::iterator itr = creators_.find(name);
    if (itr == creators_.end()) return false;
    creators_.erase(itr);
    return true;
  }

  static qmc_factory* instance()
  {
    if (!ptr_) ptr_ = new qmc_factory;
    return ptr_;
  }

private:
  qmc_factory() {}
  ~qmc_factory() {}

  static qmc_factory* ptr_;

  typedef boost::shared_ptr<abstract_worker_creator> pointer_type;
  typedef std::map<std::string, pointer_type> map_type;
  map_type creators_;
};
