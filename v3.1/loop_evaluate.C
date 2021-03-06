/*****************************************************************************
*
* ALPS Project Applications
*
* Copyright (C) 2003-2004 by Synge Todo <wistaria@comp-phys.org>
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

#include "loop_worker.h"

#include <alps/osiris.h>
#include <alps/scheduler.h>
#include <boost/filesystem/operations.hpp>

void evaluate(const boost::filesystem::path& p) {
  alps::ProcessList nowhere;
  alps::scheduler::MCSimulation sim(nowhere,p);
  accumulate(sim);
  sim.checkpoint(p);
}

int main(int argc, char** argv)
{
#ifndef BOOST_NO_EXCEPTIONS
try {
#endif

  alps::scheduler::SimpleMCFactory<alps::scheduler::DummyMCRun> factory;
  alps::scheduler::init(factory);

  if (argc != 2) {
    std::cerr << "Usage: " << argv[0] << " inputfile\n";
    std::exit(-1);
  }
  boost::filesystem::path p =
    boost::filesystem::complete(boost::filesystem::path(argv[1]));
  evaluate(p);

#ifndef BOOST_NO_EXCEPTIONS
}
catch (std::exception& e)
{
  std::cerr << "Caught exception: " << e.what() << "\n";
  std::exit(-5);
}
#endif
}
