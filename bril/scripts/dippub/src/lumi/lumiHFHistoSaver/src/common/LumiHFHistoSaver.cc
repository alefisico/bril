#include <time.h>
#include <string.h>
#include <algorithm>
#include <toolbox/string.h>
#include <b2in/nub/Method.h>

#include <TH1D.h>
#include <TROOT.h>
#include <TError.h>
#include <TStyle.h>
#include <TCanvas.h>
#include <TPaveText.h>

#include "LumiHFHistoSaver.h"

XDAQ_INSTANTIATOR_IMPL(lumi::LumiHFHistoSaver)

using namespace lumi;

//_____________________________________________________________________________________________
LumiHFHistoSaver::LumiHFHistoSaver(xdaq::ApplicationStub* s) throw (xdaq::exception::Exception) :
   LumiApplication(s),
   eventing::api::Member(this)
{

   gErrorIgnoreLevel = kWarning;
   gROOT->SetBatch(true);
   gStyle->SetOptStat(0);

   // indicator of availability of trees for filling
   mFile = NULL;

   // request processing of messages from eventing in Receive()
   b2in::nub::bind(this, &LumiHFHistoSaver::Receive);

   // automatically go into running state
   MakeTransition("Start");
}

//_____________________________________________________________________________________________
void LumiHFHistoSaver::Receive(toolbox::mem::Reference* ref, xdata::Properties& plist)
   throw (b2in::nub::exception::Exception)
{
   // Saves arrived histograms into root or png files.

   std::string type = plist.getProperty("TYPE");

   // eventing may send control messages and other trash
   if (type.compare("histograms_section") != 0 && type.compare("histograms_board") != 0) {
      ref->release();
      return;
   }

   Histogram_t hist[NUM_HISTOS];
   memcpy(hist, ref->getDataLocation(), sizeof(Histogram_t) * NUM_HISTOS);
   ref->release();

   if (mCurrRun != (uint32_t) -1 && hist[0].cms_run < mCurrRun) {
      LOG4CPLUS_ERROR(getApplicationLogger(), "Histograms from older run, discarded");
      return;
   }

   // if run number changed
   if (hist[0].cms_run != mCurrRun) {
      if (mFile) CloseTrees();

      mCurrRun = hist[0].cms_run;
      OpenTrees(mCurrRun);
   }

   if (type.compare("histograms_board") == 0) {
      if (mFile) {
         std::string board_id = plist.getProperty("MSG");
         memcpy(&mBoardId, board_id.c_str(), std::min((size_t)29, board_id.size()));
         mBoardId[29] = '\0';

         for (size_t t = 0; t < NUM_HISTOS; t++) {
            memcpy(&mHist, &hist[t], sizeof(Histogram_t));
            mTreeBoards->Fill();
         }
      }

      MakePlots(hist);
   }
   else {
      if (mFile)
         for (size_t t = 0; t < NUM_HISTOS; t++) {
            memcpy(&mHist, &hist[t], sizeof(Histogram_t));
            mTreeIntegral->Fill();
         }
   }

}

//_____________________________________________________________________________________________
bool LumiHFHistoSaver::Start()
{
   // Go from stopped to running state.

   mCurrRun = (uint32_t) -1;

   // check that eventing bus is available + subscribe
   try {
      eventing::api::Bus* ebus = &this->getEventingBus("hf");
      ebus->subscribe("histograms_board");
      ebus->subscribe("histograms_section");
   } catch (eventing::api::exception::Exception& e){
      LOG4CPLUS_ERROR(getApplicationLogger(), toolbox::toString("FATAL: %s", e.what()));
      return false; // transition failed, stay in stopped state
   }

   return true;
}

//_____________________________________________________________________________________________
bool LumiHFHistoSaver::Stop()
{
   // Go from running into stopped state.

   // check that eventing bus is available + unsubscribe
   try {
      eventing::api::Bus* ebus = &this->getEventingBus("hf");
      ebus->unsubscribe("histograms_section");
      ebus->unsubscribe("histograms_board");
   } catch (eventing::api::exception::Exception& e){
      LOG4CPLUS_ERROR(getApplicationLogger(), toolbox::toString("FATAL: %s", e.what()));
      return false; // transition failed, stay in running state
   }

   if (mFile)
      CloseTrees();

   return true;
}

//_____________________________________________________________________________________________
void LumiHFHistoSaver::OpenTrees(uint32_t run)
{

   mFile = NULL;

   // determine current UTC date and time
   struct tm utc;
   time_t since_epoch = time(NULL);
   if (since_epoch < 0 || !gmtime_r(&since_epoch, &utc)) {
      LOG4CPLUS_ERROR(getApplicationLogger(), "Failed to get current time, trees not created");
      return;
   }

   // file name with UTC date and time + run number
   std::string path = toolbox::toString("histos_section_utc%04d%02d%02d-%02d%02d%02d_run%06d.root",
                                        1900 + utc.tm_year, 1 + utc.tm_mon, utc.tm_mday,
                                        utc.tm_hour, utc.tm_min, utc.tm_sec, run);

   // create output root file with TTrees
   mFile = TFile::Open(path.c_str(), "RECREATE");
   if (!mFile || mFile->IsZombie()) {
      LOG4CPLUS_ERROR(getApplicationLogger(),
                      "TFile::Open() failed, trees will not be written");
      if (mFile) delete mFile;
      mFile = NULL;
      return;
   }

   LOG4CPLUS_INFO(getApplicationLogger(), toolbox::toString("Output file: %s", path.c_str()));

   mTreeBoards = new TTree("HistosBoards", "Board histograms, integrated over LS");
   mTreeIntegral = new TTree("HistosIntegral", "LS histograms, integrated over boards");

   // associate tree branches with variables
   mTreeBoards->Branch("board_id",                &mBoardId, "board_id/C");
   mTreeBoards->Branch("uHTR_id",                 &mHist.uHTR_id);
   mTreeBoards->Branch("hist_id",                 &mHist.hist_id);
   mTreeBoards->Branch("num_atoms",               &mHist.num_atoms);
   mTreeBoards->Branch("counter",                 &mHist.counter);
   mTreeBoards->Branch("ocr_sync",                &mHist.ocr_sync);
   mTreeBoards->Branch("timestamp_before",        &mHist.timestamp_before);
   mTreeBoards->Branch("timestamp_after",         &mHist.timestamp_after);
   mTreeBoards->Branch("timestamp_mono_before",   &mHist.timestamp_mono_before);
   mTreeBoards->Branch("timestamp_mono_after",    &mHist.timestamp_mono_after);
   mTreeBoards->Branch("h",                        mHist.h, "h[3564]/l");
   mTreeBoards->Branch("orb_init",                &mHist.orb_init);
   mTreeBoards->Branch("orb_last",                &mHist.orb_last);
   mTreeBoards->Branch("lumi_nibble",             &mHist.lumi_nibble);
   mTreeBoards->Branch("lumi_section",            &mHist.lumi_section);
   mTreeBoards->Branch("cms_run",                 &mHist.cms_run);
   mTreeBoards->Branch("lhc_fill",                &mHist.lhc_fill);
   mTreeBoards->Branch("num_errors",              &mHist.num_errors);

   mTreeIntegral->Branch("uHTR_id",               &mHist.uHTR_id);
   mTreeIntegral->Branch("hist_id",               &mHist.hist_id);
   mTreeIntegral->Branch("num_atoms",             &mHist.num_atoms);
   mTreeIntegral->Branch("counter",               &mHist.counter);
   mTreeIntegral->Branch("ocr_sync",              &mHist.ocr_sync);
   mTreeIntegral->Branch("timestamp_before",      &mHist.timestamp_before);
   mTreeIntegral->Branch("timestamp_after",       &mHist.timestamp_after);
   mTreeIntegral->Branch("timestamp_mono_before", &mHist.timestamp_mono_before);
   mTreeIntegral->Branch("timestamp_mono_after",  &mHist.timestamp_mono_after);
   mTreeIntegral->Branch("h",                      mHist.h, "h[3564]/l");
   mTreeIntegral->Branch("orb_init",              &mHist.orb_init);
   mTreeIntegral->Branch("orb_last",              &mHist.orb_last);
   mTreeIntegral->Branch("lumi_nibble",           &mHist.lumi_nibble);
   mTreeIntegral->Branch("lumi_section",          &mHist.lumi_section);
   mTreeIntegral->Branch("cms_run",               &mHist.cms_run);
   mTreeIntegral->Branch("lhc_fill",              &mHist.lhc_fill);
   mTreeIntegral->Branch("num_errors",            &mHist.num_errors);

   mTreeBoards->SetAutoFlush(300);
   mTreeBoards->SetAutoSave(50000000);
   mTreeIntegral->SetAutoFlush(10);
   mTreeIntegral->SetAutoSave(50000000);
}

//_____________________________________________________________________________________________
void LumiHFHistoSaver::CloseTrees()
{

   mFile->cd();
   mTreeBoards->Write("", TObject::kOverwrite);
   mTreeIntegral->Write("", TObject::kOverwrite);

   delete mTreeBoards;
   delete mTreeIntegral;
   delete mFile;

   mFile = NULL;
}

//_____________________________________________________________________________________________
void LumiHFHistoSaver::MakePlots(Histogram_t* hist)
{
   // Creates png files visualizing set of histograms.

   // NOTE: same order as in common.h
//    const char* type[NUM_HISTOS] = {"CMS1", "CMS2", "CMS_ET", "CMS_VALID"};

//    int uHTR = (int) histo.uHTR_id;
//    int hist_id = (int) histo.hist_id;

   TH1D cms1("cms1", "", NUM_BXES, 0, NUM_BXES);
   TH1D cms2("cms2", "", NUM_BXES, 0, NUM_BXES);
   TH1D cmsV("cmsV", "", NUM_BXES, 0, NUM_BXES);
   TH1D cmsE("cmsE", "", NUM_BXES, 0, NUM_BXES);

   for (size_t b = 0; b < NUM_BXES; b++) {
      cms1.SetBinContent(b + 1, hist[CMS1].h[b]);
      cms2.SetBinContent(b + 1, hist[CMS2].h[b]);
      cmsV.SetBinContent(b + 1, hist[CMS_VALID].h[b]);
      cmsE.SetBinContent(b + 1, hist[CMS_ET].h[b]);
   }

   // convert seconds since EPOCH into a human-readable string
   char buf[30];
   time_t timestamp = (time_t) hist[CMS1].timestamp_after;
   ctime_r(&timestamp, buf);

   //
   // CMS1 and CMS2
   //

   TCanvas c1("cms12", "cms12", 1000, 700);
   c1.SetLeftMargin(0.12);
   c1.SetRightMargin(0.02);
   c1.SetTopMargin(0.10);

   cms1.SetTitle(Form("CMS1 (black) and CMS2 (red), %s", mBoardId));
   cms1.SetXTitle("BX");
   cms1.SetYTitle("Entries");
   cms1.SetTitleOffset(1.6, "Y");

   cms1.SetLineColor(kBlack);
   cms2.SetLineColor(kRed);

   cms1.Draw();
   cms2.Draw("same");

   TPaveText txt1(0.65, 0.16, 0.9, 0.4, "NDC");
   txt1.SetFillStyle(1001);
   txt1.SetFillColor(kWhite);
   txt1.SetBorderSize(1);
   txt1.SetTextAlign(12);

   txt1.AddText(Form("timestamp: %s", buf));
   txt1.AddText(Form("section_size = %.2fs", hist[CMS1].timestamp_mono_after -
                                             hist[CMS1].timestamp_mono_before));
   txt1.AddText(Form("ocr_sync = %u", hist[CMS1].ocr_sync));
   txt1.AddText(Form("counter = %u", hist[CMS1].counter));
   txt1.AddText(Form("num_atoms = %u", hist[CMS1].num_atoms));
   txt1.AddText(Form("num_errors = %u", hist[CMS1].num_errors));
   txt1.AddText(Form("cms_run = %u", hist[CMS1].cms_run));
   txt1.Draw("same");

   c1.SaveAs(Form("cms12_%s.png", mBoardId));

   //
   // CMS_VALID
   //

   TCanvas c2("valid", "valid", 1000, 700);
   c2.SetLeftMargin(0.12);
   c2.SetRightMargin(0.02);
   c2.SetTopMargin(0.10);

   cmsV.SetTitle(Form("CMS_VALID, %s", mBoardId));
   cmsV.SetXTitle("BX");
   cmsV.SetYTitle("Entries");
   cmsV.SetTitleOffset(1.6, "Y");
   cmsV.SetLineColor(kBlack);
   cmsV.Draw();

   TPaveText txt2(0.65, 0.16, 0.9, 0.4, "NDC");
   txt2.SetFillStyle(1001);
   txt2.SetFillColor(kWhite);
   txt2.SetBorderSize(1);
   txt2.SetTextAlign(12);

   txt2.AddText(Form("timestamp: %s", buf));
   txt2.AddText(Form("section_size = %.2fs", hist[CMS_VALID].timestamp_mono_after -
                                             hist[CMS_VALID].timestamp_mono_before));
   txt2.AddText(Form("ocr_sync = %u", hist[CMS_VALID].ocr_sync));
   txt2.AddText(Form("counter = %u", hist[CMS_VALID].counter));
   txt2.AddText(Form("num_atoms = %u", hist[CMS_VALID].num_atoms));
   txt2.AddText(Form("num_errors = %u", hist[CMS_VALID].num_errors));
   txt2.AddText(Form("cms_run = %u", hist[CMS_VALID].cms_run));
   txt2.Draw("same");

   c2.SaveAs(Form("cms_valid_%s.png", mBoardId));

   //
   // CMS_ET
   //

   TCanvas c3("et", "et", 1000, 700);
   c3.SetLeftMargin(0.12);
   c3.SetRightMargin(0.02);
   c3.SetTopMargin(0.10);

   cmsE.SetTitle(Form("CMS_ET, %s", mBoardId));
   cmsE.SetXTitle("BX");
   cmsE.SetYTitle("Entries");
   cmsE.SetTitleOffset(1.6, "Y");
   cmsE.SetLineColor(kBlack);
   cmsE.Draw();

   TPaveText txt3(0.65, 0.16, 0.9, 0.4, "NDC");
   txt3.SetFillStyle(1001);
   txt3.SetFillColor(kWhite);
   txt3.SetBorderSize(1);
   txt3.SetTextAlign(12);

   txt3.AddText(Form("timestamp: %s", buf));
   txt3.AddText(Form("section_size = %.2fs", hist[CMS_ET].timestamp_mono_after -
                                             hist[CMS_ET].timestamp_mono_before));
   txt3.AddText(Form("ocr_sync = %u", hist[CMS_ET].ocr_sync));
   txt3.AddText(Form("counter = %u", hist[CMS_ET].counter));
   txt3.AddText(Form("num_atoms = %u", hist[CMS_ET].num_atoms));
   txt3.AddText(Form("num_errors = %u", hist[CMS_ET].num_errors));
   txt3.AddText(Form("cms_run = %u", hist[CMS_ET].cms_run));
   txt3.Draw("same");

   c3.SaveAs(Form("cms_et_%s.png", mBoardId));
}
