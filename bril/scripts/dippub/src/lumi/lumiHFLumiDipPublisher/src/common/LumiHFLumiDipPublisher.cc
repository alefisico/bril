#include <time.h>
#include <string.h>
#include <toolbox/string.h>
#include <b2in/nub/Method.h>
#include <string>

#include "LumiHFLumiDipPublisher.h"

XDAQ_INSTANTIATOR_IMPL(lumi::LumiHFLumiDipPublisher)

using namespace lumi;

//_____________________________________________________________________________________________
LumiHFLumiDipPublisher::LumiHFLumiDipPublisher(xdaq::ApplicationStub* s) throw (xdaq::exception::Exception) :
   LumiApplication(s),
   eventing::api::Member(this), _errorHandler(getApplicationLogger())
{
   // -------------------------------------------------------------------------
   // A.Lokhovitskiy 26.04.2015 <alokhovi@cern.ch> Added DIP publication to LHC
   // 
   _factory = NULL;    _publication = NULL;

   _nbOfNibbles =0;    _sumIntegrationTime=0.0; 

   _avgLuminosity=0.0; _avgLuminosityError=0.0;

   try
   {
      _factory     =  Dip::create("HFLumiDIPfactory");

      _publication = _factory->createDipPublication("dip/CMS/LHC/LumiScan2", &_errorHandler);
   }
   catch(const DipInternalError & e)
   {
      std::string message = "Failed to create DIP facory and a publication: ";

      message += e.what();

      LOG4CPLUS_ERROR(getApplicationLogger(), message);
   }   
   // -------------------------------------------------------------------------
   
   mPeriod = 1800; // NOTE: hardcoded number

   // request processing of messages from eventing in Receive()
   b2in::nub::bind(this, &LumiHFLumiDipPublisher::Receive);

   // automatically go into running state
   MakeTransition("Start");
}

// -------------------------------------------------------------------------
// A.Lokhovitskiy 26.04.2015 <alokhovi@cern.ch> Added DIP publication to LHC
// 
LumiHFLumiDipPublisher::~LumiHFLumiDipPublisher()
{
   try
   {
     if ( _factory != NULL && _publication != NULL )
     {
        _factory->destroyDipPublication(_publication);
     }
   }
   catch(const DipInternalError &)
   {
      // Ignore, the process is being terminated in any case.
   }
}
// -------------------------------------------------------------------------


//_____________________________________________________________________________________________
void LumiHFLumiDipPublisher::Receive(toolbox::mem::Reference* ref, xdata::Properties& plist)
   throw (b2in::nub::exception::Exception)
{
   // Receiver of luminosity data.

   std::string type = plist.getProperty("TYPE");

   // eventing may send control messages and other trash
   if (type.compare("luminosity_nibble") != 0 && type.compare("luminosity_section") != 0) 
   {
      ref->release(); return;
   }

   memcpy(&mLumi, ref->getDataLocation(), sizeof(Luminosity_t));

   ref->release();

   double ts_center = 0.5 * (mLumi.timestamp1 + mLumi.timestamp2);
   double deltaT = 0.5 * (mLumi.timestamp2 - mLumi.timestamp1);

   if (type.compare("luminosity_nibble") == 0) 
   {
      // -------------------------------------------------------------------------
      // A.Lokhovitskiy 26.04.2015 <alokhovi@cern.ch> Added DIP publication to LHC
      // 
      // 
      _nbOfNibbles++;
      _sumIntegrationTime += deltaT;
      _avgLuminosity      += mLumi.lumi;
      _avgLuminosityError += mLumi.elumi;

      if ( _nbOfNibbles % 2 == 0 )
      {
         DipData * data = _factory->createDipData();

         superlong /*DIP type*/ tsmillisecends = static_cast<superlong>(ts_center*1000.0);

         DipTimestamp timestamp(tsmillisecends);

         _avgLuminosity /= 2.0; _avgLuminosityError /= 2.0;

         data->insert(static_cast<DipDouble>(_avgLuminosity),     "CollRate"   );

         data->insert(static_cast<DipDouble>(_avgLuminosityError),"CollRateErr");

         //data->insert(static_cast<DipDouble>(_sumIntegrationTime),"IntTime"    );
         //
         // Hard coded integration time equal to 4 nibbes
         //
         data->insert(static_cast<DipDouble>(1.458),"IntTime"    );

         data->insert(static_cast<DipBool>(0),"Preferred");

         data->insert("HFLumi","Source");      

         _publication->send(*data,timestamp); delete data;

         _nbOfNibbles  =0;   _sumIntegrationTime=0.0; 

         _avgLuminosity=0.0; _avgLuminosityError=0.0;
      }
      // -------------------------------------------------------------------------
   } // luminosity_nibble
}

//_____________________________________________________________________________________________
bool LumiHFLumiDipPublisher::Start()
{
   // Go from stopped to running state.

   // check that eventing bus is available;
   // NOTE: there is no way to unsubscribe from eventing
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
bool LumiHFLumiDipPublisher::Stop()
{
   // Go from running into stopped state.
   // NOTE: there is no way to unsubscribe from eventing.

   return true;
}
