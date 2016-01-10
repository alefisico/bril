#include <time.h>
#include <string.h>
#include <toolbox/string.h>
#include <b2in/nub/Method.h>
#include <string>

#include <TH1F.h>
#include <TROOT.h>
#include <TError.h>
#include <TStyle.h>
#include <TString.h>
#include <TCanvas.h>
#include <TLegend.h>
#include <TGraphErrors.h>

#include "LumiHFLumiSaver.h"

XDAQ_INSTANTIATOR_IMPL(lumi::LumiHFLumiSaver)

using namespace lumi;

//_____________________________________________________________________________________________
size_t find_index(std::vector<GraphPoint_t>& arr, double time_min)
{
   // Quick search of first point with time >= time_min.
   //
   // NOTE: there is no guarantee that arr is time-ordered. In practice, the ordering is
   // respected quite well.

   size_t i1 = 0;
   size_t i2 = arr.size();

   while (i1 < i2) {
      size_t i = (i1 + i2)/2;

      if (arr[i].ts < time_min)
         i1 = (i1 < i ? i : i1 + 1);
      else
         i2 = i;
   }

   return i1;
}

//_____________________________________________________________________________________________
LumiHFLumiSaver::LumiHFLumiSaver(xdaq::ApplicationStub* s) throw (xdaq::exception::Exception) :
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
   
   gErrorIgnoreLevel = kWarning;
   gROOT->SetBatch(true);
   gStyle->SetOptStat(0);

   // indicator of availability of trees for filling
   mFile = NULL;

   mPeriod = 1800; // NOTE: hardcoded number
   mLastDrawTS = gettime_monotonic();

   // request processing of messages from eventing in Receive()
   b2in::nub::bind(this, &LumiHFLumiSaver::Receive);

   // automatically go into running state
   MakeTransition("Start");
}

// -------------------------------------------------------------------------
// A.Lokhovitskiy 26.04.2015 <alokhovi@cern.ch> Added DIP publication to LHC
// 
LumiHFLumiSaver::~LumiHFLumiSaver()
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
void LumiHFLumiSaver::Receive(toolbox::mem::Reference* ref, xdata::Properties& plist)
   throw (b2in::nub::exception::Exception)
{
   // Receiver of luminosity data.

   std::string type = plist.getProperty("TYPE");

   // eventing may send control messages and other trash
   if (type.compare("luminosity_nibble") != 0 && type.compare("luminosity_section") != 0) {
      ref->release();
      return;
   }

   memcpy(&mLumi, ref->getDataLocation(), sizeof(Luminosity_t));
   ref->release();

   double ts_center = 0.5 * (mLumi.timestamp1 + mLumi.timestamp2);
   double deltaT = 0.5 * (mLumi.timestamp2 - mLumi.timestamp1);

   GraphPoint_t p;
   p.lumi = mLumi.lumi;
   p.elumi = mLumi.elumi;
   p.ts = ts_center;
   p.dt = deltaT;

   if (type.compare("luminosity_nibble") == 0) {
      if (mFile)
         mTreeNibble->Fill();

      // write timestamp + luminosity into text file
      FILE* txtfile = fopen("luminosity_nibble.txt", "w");

      if (txtfile) {
         fprintf(txtfile, "%.2f %.5g", ts_center, mLumi.lumi);
         if (fclose(txtfile) != 0)
            LOG4CPLUS_ERROR(getApplicationLogger(), "fclose() failed, code bug?");
      }
      else
         LOG4CPLUS_ERROR(getApplicationLogger(),
                         "fopen() failed, nothing will be written into luminosity_nibble.txt");
      mValsN.push_back(p);

      // -------------------------------------------------------------------------
      // A.Lokhovitskiy 26.04.2015 <alokhovi@cern.ch> Added DIP publication to LHC
      // 
      // 
      _nbOfNibbles++;
      _sumIntegrationTime += deltaT;
      _avgLuminosity      += mLumi.lumi;
      _avgLuminosityError += mLumi.elumi;

      if ( _nbOfNibbles % 4 == 0 )
      {
         DipData * data = _factory->createDipData();

         superlong /*DIP type*/ tsmillisecends = static_cast<superlong>(ts_center*1000.0);

         DipTimestamp timestamp(tsmillisecends);

         _avgLuminosity /= 4.0; _avgLuminosityError /= 4.0;

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
   else {
      if (mFile)
         mTreeSection->Fill();

      // write timestamp + luminosity into text file
      FILE* txtfile = fopen("luminosity_section.txt", "w");

      if (txtfile) {
         fprintf(txtfile, "%.2f %.5g", ts_center, mLumi.lumi);
         if (fclose(txtfile) != 0)
            LOG4CPLUS_ERROR(getApplicationLogger(), "fclose() failed, code bug?");
      }
      else
         LOG4CPLUS_ERROR(getApplicationLogger(),
                        "fopen() failed, nothing will be written into luminosity_section.txt");
      mValsS.push_back(p);
   } // luminosity_section

   // truncate arrays when necessary
   if (mValsN.size() > 0 &&
       mValsN.back().ts - mValsN.front().ts > 2 * mPeriod) {
      size_t i0 = find_index(mValsN, mValsN.back().ts - mPeriod);
      if (i0 > 0)
         mValsN.erase(mValsN.begin(), mValsN.begin() + i0);
   }

   if (mValsS.size() > 0 &&
       mValsS.back().ts - mValsS.front().ts > 2 * mPeriod) {
      size_t i0 = find_index(mValsS, mValsS.back().ts - mPeriod);
      if (i0 > 0)
         mValsS.erase(mValsS.begin(), mValsS.begin() + i0);
   }

   // write trees/make plot every 30 seconds
   if (gettime_monotonic() - mLastDrawTS >= 6) {
      mLastDrawTS = gettime_monotonic();

      mFile->cd();
      mTreeNibble->Write("", TObject::kOverwrite);
      mTreeSection->Write("", TObject::kOverwrite);

      MakePlot();
   }

}

//_____________________________________________________________________________________________
bool LumiHFLumiSaver::Start()
{
   // Go from stopped to running state.

   mValsN.clear();
   mValsS.clear();

   // indicator of availability of trees for filling
   mFile = NULL;

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

   InitTrees();

   return true;
}

//_____________________________________________________________________________________________
bool LumiHFLumiSaver::Stop()
{
   // Go from running into stopped state.
   // NOTE: there is no way to unsubscribe from eventing.

   if (mFile)
      CloseTrees();

   return true;
}

//_____________________________________________________________________________________________
void LumiHFLumiSaver::InitTrees()
{

   mFile = NULL;

   // determine current UTC date and time
   struct tm utc;
   time_t since_epoch = time(NULL);
   if (since_epoch < 0 || !gmtime_r(&since_epoch, &utc)) {
      LOG4CPLUS_ERROR(getApplicationLogger(),
                      "Failed to get current time, root file with trees not created");
      return;
   }

   // file name with UTC date and time
   std::string path = toolbox::toString("luminosity_utc%04d%02d%02d-%02d%02d%02d.root",
                                        1900 + utc.tm_year, 1 + utc.tm_mon, utc.tm_mday,
                                                   utc.tm_hour, utc.tm_min, utc.tm_sec);
   // create output root file with TTree
   mFile = TFile::Open(path.c_str(), "RECREATE");
   if (!mFile || mFile->IsZombie()) {
      LOG4CPLUS_ERROR(getApplicationLogger(),
                      "TFile::Open() failed, trees will not be written");
      if (mFile) delete mFile;
      mFile = NULL;
      return;
   }

   LOG4CPLUS_INFO(getApplicationLogger(), toolbox::toString("Output file: %s", path.c_str()));

   mTreeNibble = new TTree("LumiTreeNibble", "Per nibble luminosity");
   mTreeSection = new TTree("LumiTreeSection", "Per section luminosity");

   // associate tree branches with variables
   mTreeNibble->Branch("lumi",       &mLumi.lumi);
   mTreeNibble->Branch("elumi",      &mLumi.elumi);
   mTreeNibble->Branch("timestamp1", &mLumi.timestamp1);
   mTreeNibble->Branch("timestamp2", &mLumi.timestamp2);

   mTreeSection->Branch("lumi",       &mLumi.lumi);
   mTreeSection->Branch("elumi",      &mLumi.elumi);
   mTreeSection->Branch("timestamp1", &mLumi.timestamp1);
   mTreeSection->Branch("timestamp2", &mLumi.timestamp2);

   // FIXME: those calls not working
   mTreeNibble->SetAutoFlush(200);
   mTreeNibble->SetAutoSave(100000);
   mTreeSection->SetAutoFlush(5);
   mTreeSection->SetAutoSave(100000);
}

//_____________________________________________________________________________________________
void LumiHFLumiSaver::CloseTrees()
{

   mFile->cd();
   mTreeNibble->Write("", TObject::kOverwrite);
   mTreeSection->Write("", TObject::kOverwrite);

   delete mTreeNibble;
   delete mTreeSection;
   delete mFile;

   mFile = NULL;
}

//_____________________________________________________________________________________________
void LumiHFLumiSaver::MakePlot()
{

   size_t iN = 0;
   size_t iS = 0;

   if (mValsN.size() > 0)
      iN = find_index(mValsN, mValsN.back().ts - mPeriod);
   if (mValsS.size() > 0)
      iS = find_index(mValsS, mValsS.back().ts - mPeriod);

   if (mValsN.size() - iN < 10 && mValsS.size() - iS < 5)
      return;

   TCanvas c("luminosity", "luminosity", 1280, 720);
   c.SetLeftMargin(0.08);
   c.SetRightMargin(0.02);
   c.SetTopMargin(0.06);
   c.SetBottomMargin(0.07);
   c.SetLogy();
   c.SetGridx();
   c.SetGridy();

   double xmin = -1;
   double xmax = -1;
   double ymin = -1;
   double ymax = -1;

   TGraphErrors grN, grS1, grS2;

   for (size_t i = iN; i < mValsN.size(); i++) {
      grN.SetPoint(i - iN, mValsN[i].ts, mValsN[i].lumi);
      grN.SetPointError(i - iN, 0, mValsN[i].elumi);

      if (mValsN[i].ts - mValsN[i].dt < xmin || xmin < 0)
         xmin = mValsN[i].ts - mValsN[i].dt;
      if (mValsN[i].ts + mValsN[i].dt > xmax || xmax < 0)
         xmax = mValsN[i].ts + mValsN[i].dt;
      if (mValsN[i].lumi - mValsN[i].elumi < ymin || ymin < 0)
         ymin = mValsN[i].lumi - mValsN[i].elumi;
      if (mValsN[i].lumi + mValsN[i].elumi > ymax || ymax < 0)
         ymax = mValsN[i].lumi + mValsN[i].elumi;
   }

   for (size_t i = iS; i < mValsS.size(); i++) {
      grS1.SetPoint(i - iS, mValsS[i].ts, mValsS[i].lumi);
      grS1.SetPointError(i - iS, 0, mValsS[i].elumi);

      grS2.SetPoint(i - iS, mValsS[i].ts, mValsS[i].lumi);
      grS2.SetPointError(i - iS, mValsS[i].dt, 0);

      if (mValsS[i].ts - mValsS[i].dt < xmin || xmin < 0)
         xmin = mValsS[i].ts - mValsS[i].dt;
      if (mValsS[i].ts + mValsS[i].dt > xmax || xmax < 0)
         xmax = mValsS[i].ts + mValsS[i].dt;
      if (mValsS[i].lumi - mValsS[i].elumi < ymin || ymin < 0)
         ymin = mValsS[i].lumi - mValsS[i].elumi;
      if (mValsS[i].lumi + mValsS[i].elumi > ymax || ymax < 0)
         ymax = mValsS[i].lumi + mValsS[i].elumi;
   }

   TH1F* frame = c.DrawFrame(xmin, ymin * 0.5, xmax, ymax * 2);
   frame->SetTitle("");
   frame->SetYTitle("Luminosity");
   frame->SetTitleOffset(1.2, "X");
   frame->SetTitleOffset(1.1, "Y");
   frame->Draw();

   grN.SetLineColor(kRed);
   grN.SetMarkerStyle(20);
   grN.SetMarkerSize(0.3);
   grN.SetMarkerColor(kRed);

   grS1.SetLineColor(kBlue);
   grS1.SetMarkerStyle(20);
   grS1.SetMarkerSize(0.5);
   grS1.SetMarkerColor(kBlue);

   grS2.SetLineColor(kBlue);
   grS2.SetMarkerStyle(20);
   grS2.SetMarkerSize(0.5);
   grS2.SetMarkerColor(kBlue);

   if (iN < mValsN.size())
      grN.Draw("PZ");
   if (iS < mValsS.size()) {
      grS1.Draw("PZ");
      grS2.Draw("[]");
   }

   int nbins = frame->GetNbinsX();

   // convert seconds since EPOCH into human-readable strings
   struct tm res;
   time_t timestamp;

   timestamp = (time_t) xmin;
   localtime_r(&timestamp, &res);
   frame->GetXaxis()->SetBinLabel(1, Form("%02d:%02d:%02d", res.tm_hour, res.tm_min, res.tm_sec));

   timestamp = (time_t) (0.75 * xmin + 0.25 * xmax);
   localtime_r(&timestamp, &res);
   frame->GetXaxis()->SetBinLabel(nbins/4, Form("%02d:%02d:%02d", res.tm_hour, res.tm_min, res.tm_sec));

   timestamp = (time_t) (0.5 * xmin + 0.5 * xmax);
   localtime_r(&timestamp, &res);
   frame->GetXaxis()->SetBinLabel(nbins/2, Form("%02d:%02d:%02d", res.tm_hour, res.tm_min, res.tm_sec));

   timestamp = (time_t) (0.25 * xmin + 0.75 * xmax);
   localtime_r(&timestamp, &res);
   frame->GetXaxis()->SetBinLabel(0.75 * nbins, Form("%02d:%02d:%02d", res.tm_hour, res.tm_min, res.tm_sec));

   timestamp = (time_t) xmax;
   localtime_r(&timestamp, &res);
   frame->GetXaxis()->SetBinLabel(nbins, Form("%02d:%02d:%02d", res.tm_hour, res.tm_min, res.tm_sec));

   frame->SetBins(4, xmin, xmax);
   frame->GetXaxis()->LabelsOption("u");
//    frame->GetXaxis()->SetNdivisions(8, 8, 8, false);

   c.SaveAs("luminosity.png");
}
