#include <climits>
#include <stdlib.h>
#include <toolbox/string.h>
#include <uhal/log/log.hpp>
#include <hcal/uhtr/uHTR.hh>
#include <uhal/ConnectionManager.hpp>

#include "common.h"
#include "LumiHFReadout.h"
#include "LumiConfiguration.h"

XDAQ_INSTANTIATOR_IMPL(lumi::LumiHFReadout)

using namespace lumi;

// threads
extern void ThreadCalculator(LumiHFReadout* self, LumiQueue* qcalc, eventing::api::Bus* ebus);
extern void ThreadReadout(LumiHFReadout* self, size_t n, LumiQueue* qcalc, hcal::uhtr::uHTR*);

//_____________________________________________________________________________________________
LumiHFReadout::LumiHFReadout(xdaq::ApplicationStub* s) throw (xdaq::exception::Exception) :
   LumiApplication(s),
   eventing::api::Member(this),
   mQueueCalculator(NUM_HISTOS * sizeof(Histogram_t), 360)
{

   // suppress useless printouts from uhal (uhal messages are logged)
   uhal::setLogLevelTo(uhal::Fatal());

   // automatically go into running state
   MakeTransition("Start");
}

//_____________________________________________________________________________________________
bool LumiHFReadout::Start()
{
   // Go from stopped to running state.
   //
   // NOTE: path to a configuration file can be given in LUMI_CONFIG_HFREADOUT environment
   // variable. Otherwise, the path is /opt/xdaq/etc/lumihfreadout.conf.

   // determine path to a configuration file
   const char* cfgpath = getenv("LUMI_CONFIG_HFREADOUT");
   if (!cfgpath)
      cfgpath = "/opt/xdaq/etc/lumihfreadout.conf";

   // cleanup from previous Start() call
   cfg.mask.clear();

   // read configuration from file
   try {
      LumiConfiguration config(cfgpath);

      cfg.orbits_per_nibble   = (int) config.GetInt("orbits_per_nibble");
      cfg.nibbles_per_section = (int) config.GetInt("nibbles_per_section");
      cfg.polls_per_nibble    = (int) config.GetInt("polls_per_nibble");
      cfg.cms1_threshold      = (int) config.GetInt("cms1_threshold");
      cfg.cms2_threshold      = (int) config.GetInt("cms2_threshold");

      cfg.addr = config.GetArrString("uHTR_addr");
      cfg.uid  = config.GetArrString("uHTR_uid");

      cfg.address_table = config.GetString("uHTR_address_table");

      cfg.histo_time_uncert = config.GetDouble("histo_time_uncertainty");
      cfg.stats_logging_period = (double) config.GetInt("stats_logging_period");

      if (config.GetInt("orbits_per_nibble") > INT_MAX || cfg.orbits_per_nibble <= 0)
         throw std::runtime_error("invalid orbits_per_nibble");

      if (config.GetInt("nibbles_per_section") > INT_MAX)
         throw std::runtime_error("invalid nibbles_per_section");
      if (cfg.nibbles_per_section < 2)
         throw std::runtime_error("nibbles_per_section < 2");

      if (cfg.polls_per_nibble < 3)
         throw std::runtime_error("polls_per_nibble < 3");
      if (config.GetInt("polls_per_nibble") > INT_MAX)
         throw std::runtime_error("invalid polls_per_nibble");

      if (config.GetInt("cms1_threshold") > INT_MAX || cfg.cms1_threshold < 0)
         throw std::runtime_error("invalid cms1_threshold");

      if (config.GetInt("cms2_threshold") > INT_MAX || cfg.cms2_threshold < 0)
         throw std::runtime_error("invalid cms2_threshold");

      if (cfg.histo_time_uncert <= 0)
         throw std::runtime_error("histo_time_uncert <= 0");

      if (cfg.stats_logging_period <= 0)
         throw std::runtime_error("stats_logging_period <= 0");

      if (cfg.addr.size() != cfg.uid.size())
         throw std::runtime_error("non-equal number of uHTR_addr and uHTR_uid statements");

      // bit masks
      std::vector<std::string> uHTR_masks = config.GetArrString("uHTR_mask");

      if (cfg.addr.size() != uHTR_masks.size())
         throw std::runtime_error("non-equal number of uHTR_addr and uHTR_mask statements");

      for (size_t b = 0; b < uHTR_masks.size(); b++) {
         if (uHTR_masks[b].size() != 24)
            throw std::runtime_error("size of uHTR_mask not equal to 24 characters");

         std::vector<bool> mask;

         for (size_t fi = 0; fi < 24; fi++) { // fibers
            char str[2] = {'\0', '\0'};
            str[0] = uHTR_masks[b][fi];

            char* ptr; // reference to first invalid character
            long int digit = strtol(str, &ptr, 10);

            if (digit < 0 || digit > 7 || *ptr != '\0')
               throw std::runtime_error("uHTR_mask must contain only 0-7 characters");

            for (size_t ch = 0; ch < 3; ch++) // channels
               mask.push_back(digit & 1<<ch);
         }

         cfg.mask.push_back(mask);
      } // board loop

   }
   catch (std::runtime_error& e) {
      const char* fmt = "Failed to read valid configuration from %s: %s";
      LOG4CPLUS_ERROR(getApplicationLogger(), toolbox::toString(fmt, cfgpath, e.what()));
      return false; // transition failed, stay in stopped state
   }

   // check that eventing bus is available
   eventing::api::Bus* ebus;
   try {
      ebus = &this->getEventingBus("hf");
   } catch (eventing::api::exception::Exception& e){
      LOG4CPLUS_ERROR(getApplicationLogger(), toolbox::toString("FATAL: %s", e.what()));
      return false; // transition failed, stay in stopped state
   }

   // NOTE: Jeremy suggests to initialize the uHTR classes as rarely as possible
   hcal::uhtr::uHTR* uHTRs[cfg.addr.size()];
   ssize_t n;

   // initialize handles to uHTR boards
   for (n = 0; n < (ssize_t)cfg.addr.size(); n++) {
      const char* id = cfg.uid[n].c_str();

      try {
         uhal::HwInterface hw = uhal::ConnectionManager::getDevice(id, cfg.addr[n],
                                                                   cfg.address_table);
         uHTRs[n] = new hcal::uhtr::uHTR(id, hw); // uHTR() makes a copy of hw
      } catch (std::exception& e) {
         LOG4CPLUS_ERROR(getApplicationLogger(), toolbox::toString("%s: %s", id, e.what()));

         // cleanup
         while (n > 0) {
            n--;
            delete uHTRs[n];
         }

         return false; // transition failed, stay in stopped state
      }
   }

   // log uid-to-address mapping
   for (n = 0; n < (ssize_t)cfg.addr.size(); n++)
      LOG4CPLUS_INFO(getApplicationLogger(),
                     toolbox::toString("%s: %s", cfg.uid[n].c_str(), cfg.addr[n].c_str()));

   // NOTE: memory barrier is not necessary for the read-only cfg structure initialized above
   // since all other threads which access this structure are started from the current thread

   // cleanup from previous executions
   mQueueCalculator.Reset();
   mThreadsContinue = true;
   mNumEnabled = 0;

   // spawn the calculator thread
   mThreads.create_thread(boost::bind(ThreadCalculator, this, &mQueueCalculator, ebus));

   // spawn readout threads;
   // NOTE: each thread is responsible for freeing memory allocated by uHTRs[n]
   for (n = 0; n < (ssize_t)cfg.addr.size(); n++)
      mThreads.create_thread(boost::bind(ThreadReadout, this, n, &mQueueCalculator, uHTRs[n]));

   return true;
}

//_____________________________________________________________________________________________
bool LumiHFReadout::Stop()
{
   // Go from running to stopped state.

   mMutex.lock();
   mThreadsContinue = false;
   mMutex.unlock();

   // wait until all threads terminate
   mThreads.join_all();

   return true;
}
