#ifndef LUMIHFLUMISAVER_H
#define LUMIHFLUMISAVER_H

#include <vector>
#include <stdio.h>
#include <dip/Dip.h>

#include <eventing/api/Member.h>
#include <b2in/nub/exception/Exception.h>

#include "common.h"
#include "LumiApplication.h"


// xDAQ's makefiles require a namespace for the main class
namespace lumi {


   // -------------------------------------------------------------------------
   // A.Lokhovitskiy 26.04.2015 <alokhovi@cern.ch> Added DIP publication to LHC
   // 
   class DIPLHCErrorHandler : public DipPublicationErrorHandler
   {
   private:
      Logger & _log;
   public:
      DIPLHCErrorHandler(Logger &log) : _log(log) { }
   private:
      void handleException(DipPublication* publication, DipException& e)
      {
         std::string message = "DIP publisher threw an exception: ";

         message += e.what();   LOG4CPLUS_ERROR(_log, message);
      }

   public:

      virtual ~DIPLHCErrorHandler() {};
   };
   // --------------------------------------------------------------------------


   class LumiHFLumiDipPublisher: public LumiApplication, public eventing::api::Member {
     public:

      XDAQ_INSTANTIATOR();

      LumiHFLumiDipPublisher(xdaq::ApplicationStub* s) throw (xdaq::exception::Exception);

      virtual ~LumiHFLumiDipPublisher();

      void Receive(toolbox::mem::Reference*, xdata::Properties&) throw (b2in::nub::exception::Exception);

     protected:

      virtual bool Start();
      virtual bool Stop();

      Luminosity_t  mLumi;

      double        mPeriod;      // (hardcoded) maximum drawing time region (in seconds)

      // -------------------------------------------------------------------------
      // A.Lokhovitskiy 26.04.2015 <alokhovi@cern.ch> Added DIP publication to LHC
      // 
      DipFactory        *_factory;

      DIPLHCErrorHandler _errorHandler;

      DipPublication    *_publication;

      int    _nbOfNibbles;
      double _sumIntegrationTime;
      double _avgLuminosity;
      double _avgLuminosityError;
      // -------------------------------------------------------------------------
   };

} // namespace lumi

#endif
