#ifndef LUMIDIPPUBLISHER_H
#define LUMIDIPPUBLISHER_H

#include <eventing/api/Member.h>
#include <b2in/nub/exception/Exception.h>

#include "LumiApplication.h"

// HF per lumi nibble/section luminosity information
struct Luminosity_t {
   double lumi;         // evaluated luminosity
   double elumi;        // its statistical error
   double timestamp1;   // approximate lumi nibble/section start time (UTC seconds since EPOCH)
   double timestamp2;   // approximate lumi nibble/section end time (UTC seconds since EPOCH)
};

// xDAQ's makefiles require a namespace for the main class
namespace lumi {

   class LumiDIPPublisher: public LumiApplication, public eventing::api::Member {
     public:

      XDAQ_INSTANTIATOR();

      LumiDIPPublisher(xdaq::ApplicationStub* s) throw (xdaq::exception::Exception);

      void Receive(toolbox::mem::Reference*, xdata::Properties&)
         throw (b2in::nub::exception::Exception);

     protected:

      virtual bool Start();
      virtual bool Stop();

   };

} // namespace lumi

#endif
