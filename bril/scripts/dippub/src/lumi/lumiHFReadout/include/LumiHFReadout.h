#ifndef LUMIHFREADOUT_H
#define LUMIHFREADOUT_H

#include <string>
#include <vector>
#include <boost/thread.hpp>
#include <eventing/api/Member.h>

#include "LumiQueue.h"
#include "LumiApplication.h"

using namespace std;

// xDAQ's makefiles require a namespace for the main class
namespace lumi {

   class LumiHFReadout: public LumiApplication, public eventing::api::Member {
     public:

      // configuration
      struct cfg_t {
         int orbits_per_nibble;       // lumi nibble size (integer number of orbits)
         int nibbles_per_section;     // lumi section size (integer number of lumi nibbles)
         int polls_per_nibble;        // number of times each uHTR board is polled every nibble
         int cms1_threshold;          // threshold for filling CMS1 occupancy histograms
         int cms2_threshold;          // threshold for filling CMS2 occupancy histograms

         vector<string> addr;         // addresses of uHTR boards
         vector<string> uid;          // short human-readable strings-identifiers
         vector<vector<bool> > mask;  // bit masks of disabled channels in fibers
         string address_table;        // path to file with name-to-address map of uHTR endpoints

         double histo_time_uncert;    // maximum tolerable uncertainty of creation time of histograms
         double stats_logging_period; // time interval (in seconds) between performance logs
      } cfg;

      XDAQ_INSTANTIATOR();

      LumiHFReadout(xdaq::ApplicationStub* s) throw (xdaq::exception::Exception);

      bool ThreadsContinue() {
         boost::lock_guard<boost::mutex> lock(mMutex);
         return mThreadsContinue;
      }

      void IncreaseNumEnabled() {
         boost::lock_guard<boost::mutex> lock(mMutex);
         mNumEnabled++;
      }

      void DecreaseNumEnabled() {
         boost::lock_guard<boost::mutex> lock(mMutex);
         mNumEnabled--;
      }

      int GetNumEnabled() {
         boost::lock_guard<boost::mutex> lock(mMutex);
         return mNumEnabled;
      }

     protected:

      virtual bool Start();
      virtual bool Stop();

      LumiQueue    mQueueCalculator;   // queue between readout and calculator threads
      bool         mThreadsContinue;   // if false, all threads are requested to terminate
      int          mNumEnabled;        // number of currently being polled uHTR boards
      boost::mutex mMutex;             // protects consistency of data members between threads

      boost::thread_group mThreads;

   }; // class LumiHFReadout

} // namespace lumi

#endif
