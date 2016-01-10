#include <time.h>
#include <toolbox/string.h>
#include <b2in/nub/Method.h>

#include "LumiDIPPublisher.h"

XDAQ_INSTANTIATOR_IMPL(lumi::LumiDIPPublisher)

using namespace lumi;

//_____________________________________________________________________________________________
LumiDIPPublisher::LumiDIPPublisher(xdaq::ApplicationStub* s) throw (xdaq::exception::Exception) :
   LumiApplication(s),
   eventing::api::Member(this)
{

   // request processing of messages from eventing in Receive()
   b2in::nub::bind(this, &LumiDIPPublisher::Receive);

   // automatically go into running state
   MakeTransition("Start");
}

//_____________________________________________________________________________________________
void LumiDIPPublisher::Receive(toolbox::mem::Reference* ref, xdata::Properties& plist)
   throw (b2in::nub::exception::Exception)
{
   // Receiver of luminosity data.

   std::string type = plist.getProperty("TYPE");

   // eventing may send control messages and other trash
   if (type.compare("luminosity_nibble") != 0 && type.compare("luminosity_section") != 0) {
      ref->release();
      return;
   }

   Luminosity_t data;
   memcpy(&data, ref->getDataLocation(), sizeof(Luminosity_t));
   ref->release();

   // convert seconds since EPOCH into human-readable string
   struct tm t;
   time_t timestamp = (time_t) data.timestamp1;
   localtime_r(&timestamp, &t);

   if (type.compare("luminosity_nibble") == 0) {
      LOG4CPLUS_INFO(getApplicationLogger(),
                     toolbox::toString("LN: lumi=%f+-%f, ts=%.2f (%i:%i)",
                                       data.lumi, data.elumi, data.timestamp1, t.tm_hour, t.tm_min));
   }
   else {
      LOG4CPLUS_INFO(getApplicationLogger(),
                     toolbox::toString("LS: lumi=%f+-%f, ts=%.2f (%i:%i)",
                                       data.lumi, data.elumi, data.timestamp1, t.tm_hour, t.tm_min));
    }

}

//_____________________________________________________________________________________________
bool LumiDIPPublisher::Start()
{
   // Go from stopped to running state.

   // check that eventing bus is available + subscribe
   try {
      eventing::api::Bus* ebus = &this->getEventingBus("hf");
      ebus->subscribe("luminosity_nibble");
      ebus->subscribe("luminosity_section");
   } catch (eventing::api::exception::Exception& e){
      LOG4CPLUS_ERROR(getApplicationLogger(), toolbox::toString("FATAL: %s", e.what()));
      return false; // transition failed, stay in stopped state
   }

   return true;
}

//_____________________________________________________________________________________________
bool LumiDIPPublisher::Stop()
{
   // Go from running into stopped state.

   // check that eventing bus is available + unsubscribe
   try {
      eventing::api::Bus* ebus = &this->getEventingBus("hf");
      ebus->unsubscribe("luminosity_section");
      ebus->unsubscribe("luminosity_nibble");
   } catch (eventing::api::exception::Exception& e){
      LOG4CPLUS_ERROR(getApplicationLogger(), toolbox::toString("FATAL: %s", e.what()));
      return false; // transition failed, stay in running state
   }

   return true;
}
