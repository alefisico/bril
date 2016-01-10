#ifndef LUMIHFHISTOSAVER_H
#define LUMIHFHISTOSAVER_H

#include <TFile.h>
#include <TTree.h>

#include <eventing/api/Member.h>
#include <b2in/nub/exception/Exception.h>

#include "common.h"
#include "LumiApplication.h"

// xDAQ's makefiles require a namespace for the main class
namespace lumi {

   class LumiHFHistoSaver: public LumiApplication, public eventing::api::Member {
     public:

      XDAQ_INSTANTIATOR();

      LumiHFHistoSaver(xdaq::ApplicationStub* s) throw (xdaq::exception::Exception);

      void Receive(toolbox::mem::Reference*, xdata::Properties&)
         throw (b2in::nub::exception::Exception);

     protected:

      virtual bool Start();
      virtual bool Stop();

      void OpenTrees(uint32_t run);
      void CloseTrees();
      void MakePlots(Histogram_t* hist);

      // data members
      TFile*       mFile;
      TTree*       mTreeBoards;
      TTree*       mTreeIntegral;

      Histogram_t  mHist;
      char         mBoardId[30];

      uint32_t     mCurrRun;
   };

} // namespace lumi

#endif
