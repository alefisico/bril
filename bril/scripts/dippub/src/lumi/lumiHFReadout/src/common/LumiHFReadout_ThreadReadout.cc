#include <map>
#include <math.h>
#include <toolbox/string.h>
#include <hcal/uhtr/uHTR.hh>

#include "common.h"
#include "LumiHFReadout.h"

using namespace lumi;

//_____________________________________________________________________________________________
void program_1to1lut(hcal::uhtr::uHTR* uHTR)
{
   // Sets up trivial lookup tables (1 ADC count = 1 energy unit).

   std::vector<uint16_t> lut;

   for (uint16_t i = 0; i < 128; i++)
      lut.push_back(i);

   for (int fi = 0; fi < 24; fi++) // fibers
      for (int ch = 0; ch < 3; ch++) // channels
         uHTR->lumi_program_lut(fi, ch, lut);
}

//_____________________________________________________________________________________________
void configure_uhtr(LumiHFReadout* self, hcal::uhtr::uHTR* uHTR, size_t n, const char* id)
{
   // Configures uHTR with desired parameters and issues commands to start production of
   // histograms.

   LOG4CPLUS_INFO(self->getApplicationLogger(),
                  toolbox::toString("%s: configuring hw, resetting lumi links", id));

   // set crate and slot numbers; TODO: do we need this?
//    unsigned last = ip.rfind(".");
//    int slot = 0.25 * atoi(ip.substr(last+1).c_str());
//    int crate = atoi(ip.substr(ip.substr(0, last).rfind(".") + 1, last).c_str());
//    uHTR->setCrateSlot(crate, slot);

   int n_lhc_orb, n_cms_orb, lhc_threshold, cms1_th, cms2_th;
   std::vector<bool> mask_lhc, mask_cms;

   program_1to1lut(uHTR);

   uHTR->lumi_read_setup(n_lhc_orb, n_cms_orb, lhc_threshold, cms1_th, cms2_th, mask_lhc,
                         mask_cms);

   uHTR->lumi_setup(self->cfg.orbits_per_nibble, self->cfg.orbits_per_nibble, lhc_threshold,
                    self->cfg.cms1_threshold, self->cfg.cms2_threshold,
                    self->cfg.mask[n], self->cfg.mask[n]);

   uHTR->lumi_link_reset();

   usleep(1000); // 1ms

   uint32_t status;

   // try to re-reset lumi link if necessary to make it happy
   for (int tries = 0; tries < 10; tries++) {
      uint32_t errors;
      float lumi_bc0, trig_bc0;

      uHTR->lumi_link_status(0, status, lumi_bc0, trig_bc0, errors); // 0 = HF, 1 = BHM
      if (status == 0x3F) break; // everything is great!

      LOG4CPLUS_ERROR(self->getApplicationLogger(),
                      toolbox::toString("%s: link not happy, trying to re-reset", id));

      uHTR->lumi_link_reset();
      usleep(1000); // 1ms
   }

   // if the link is permanently not happy
   if (status != 0x3F)
      throw std::runtime_error("link re-reset failed");
}

//_____________________________________________________________________________________________
void ThreadReadout(LumiHFReadout* self, size_t n, LumiQueue* qcalc, hcal::uhtr::uHTR* uHTR)
{
   // Thread which is responsible for reading off histograms from a uHTR board through its
   // periodic polling using ipbus.
   //
   // n -- uHTR number.

   // format specifier for LOG4CPLUS
   const char* fmt;

   // lumi nibble size and time between polls (in seconds)
   double nibble_size = 1./REVOLUTION_FREQUENCY * self->cfg.orbits_per_nibble;
   double poll_period = nibble_size / self->cfg.polls_per_nibble;

   // human-readable string-identifier
   const char* id = self->cfg.uid[n].c_str();

   do {
      // catch all exceptions, start over from the very beginning if an exception caught
      try {
         configure_uhtr(self, uHTR, n, id);

         fmt = "%s: hw configuration successful, starting readout";
         LOG4CPLUS_INFO(self->getApplicationLogger(), toolbox::toString(fmt, id));

         self->IncreaseNumEnabled();

         Histogram_t hist[NUM_HISTOS];   // full set of histograms per lumi nibble
         bool already_got[NUM_HISTOS];   // indicator of already read off histogram types
         size_t nh = 0;                  // actual number elements in hist[]

         for (size_t i = 0; i < NUM_HISTOS; i++) {
            already_got[i] = false;
            hist[i].uHTR_id = n;
            hist[i].num_atoms = 1;
         }

         // orbit alignment status (influenced by the orbit counter reset signal in uHTR):
         //   0 = not aligned + absolute nibble number (from beginning of a run) is not known;
         //   1 = aligned on a common boundary + absolute nibble number is known
         uint32_t ocr_sync = 0;

         // - if ocr_sync=0, just a consequitive nibble number without guaranteed correctness
         //   of relative numbering of nibbles;
         // - if ocr_sync=1, absolute nibble number (counting from beginning of a run)
         uint32_t counter = 0;

         // to verify continuity of hist.orb_init
         int64_t orb_init = -1;

         // do not report about previously lost histograms after first successful readout
         bool report_overflow = false;

         std::map<std::string, bool> aM;  // availabiliby status of histograms
         std::map<std::string, bool> oM;  // overflow status of histograms

         // gathered statistics
         double st_time = gettime_monotonic();             // time of last report
         double st_poll_mean, st_poll_sigma, st_poll_max;  // poll times
         double st_read_mean, st_read_sigma, st_read_max;  // poll+readout times
         int npolls, nreads;

         st_poll_mean = st_poll_sigma = st_poll_max = 0;
         st_read_mean = st_read_sigma = st_read_max = 0;
         npolls = nreads = 0;

         // to detect if no histograms are being produced regularly
         double time_last_histo = gettime_monotonic();

         // times before previous poll
         double timestamp_mono0 = gettime_monotonic();
         double timestamp_utc0 = gettime_utc();

         while (self->ThreadsContinue()) {
            // complain if no histograms were produced during every nibble period
            if (gettime_monotonic() - time_last_histo > 1.5 * nibble_size) {
               fmt = "%s: no histograms produced during last nibble period";
               LOG4CPLUS_ERROR(self->getApplicationLogger(), toolbox::toString(fmt, id));

               // NOTE: do an extra link reset to avoid this subtle front/back issue
               fmt = "%s: resetting lumi links";
               LOG4CPLUS_INFO(self->getApplicationLogger(), toolbox::toString(fmt, id));
               uHTR->lumi_link_reset();

               ocr_sync = 0;
               orb_init = -1;
               time_last_histo = gettime_monotonic();
            }

            // log statistics and do performance checks
            if (gettime_monotonic() - st_time > self->cfg.stats_logging_period) {
               if (npolls > 0) {
                  st_poll_mean /= npolls;
                  st_poll_sigma = sqrt(st_poll_sigma/npolls - st_poll_mean * st_poll_mean);
               }
               if (nreads > 0) {
                  st_read_mean /= nreads;
                  st_read_sigma = sqrt(st_read_sigma/nreads - st_read_mean * st_read_mean);
               }

               fmt = "%s: stats: polling times: mean=%.2fms, sigma=%.2fms, max=%.2fms";
               LOG4CPLUS_INFO(self->getApplicationLogger(),
                              toolbox::toString(fmt, id, st_poll_mean * 1000,
                                                st_poll_sigma * 1000, st_poll_max * 1000));

               fmt = "%s: stats: readout times: mean=%.2fms, sigma=%.2fms, max=%.2fms";
               LOG4CPLUS_INFO(self->getApplicationLogger(),
                              toolbox::toString(fmt, id, st_read_mean * 1000,
                                                st_read_sigma * 1000, st_read_max * 1000));

               // performance checks
               if (st_read_max > 0.8 * self->cfg.histo_time_uncert) {
                  fmt = "%s: stats: readout was slow, data loss may happen in future";
                  LOG4CPLUS_WARN(self->getApplicationLogger(), toolbox::toString(fmt, id));
               }

               st_poll_mean = st_poll_sigma = st_poll_max = 0;
               st_read_mean = st_read_sigma = st_read_max = 0;
               npolls = nreads = 0;

               st_time = gettime_monotonic();
            }

            aM.clear();
            oM.clear();

            double timestamp_mono1 = timestamp_mono0;
            double timestamp_utc1 = timestamp_utc0;

            timestamp_mono0 = gettime_monotonic();
            timestamp_utc0 = gettime_utc();

            // poll uHTR for status of histograms
            uHTR->lumi_histo_status(aM, oM);

            double timestamp_utc2 = gettime_utc();
            double timestamp_mono2 = gettime_monotonic();

            // gather polling statistics
            double dt = timestamp_mono2 - timestamp_mono0;
            st_poll_mean += dt;
            st_poll_sigma += dt * dt;
            if (st_poll_max < dt) st_poll_max = dt;
            npolls++;

            int nhist = 0;
            const char* name[NUM_HISTOS] = {"CMS1", "CMS2", "CMS_ET", "CMS_VALID"};
            size_t      type[NUM_HISTOS] = { CMS1,   CMS2,   CMS_ET,   CMS_VALID };

            // loop over histogram types;
            // NOTE: almost always, full set of histograms becomes available after single poll
            for (size_t t = 0; t < NUM_HISTOS; t++) {
               // check availabiliby
               if (!aM[name[t]]) continue;
               nhist++;

               // read off new histogram
               hcal::uhtr::uHTR::LumiHistogram uhist;
               uHTR->lumi_read_histogram(name[t], uhist);

               hist[nh].timestamp_before = timestamp_utc1;
               hist[nh].timestamp_mono_before = timestamp_mono1;

               // lost any previous histograms?
               if (oM[name[t]] || uhist.overflow != 0) {
                  if (report_overflow) {
                     fmt = "%s: board reports about previously lost %s histogram";
                     LOG4CPLUS_ERROR(self->getApplicationLogger(),
                                     toolbox::toString(fmt, id, name[t]));
                  }

                  hist[nh].timestamp_after = gettime_utc();
                  hist[nh].timestamp_mono_after = gettime_monotonic();
                  hist[nh].num_errors = 1;

                  ocr_sync = 0;
                  orb_init = -1;
               } else {
                  hist[nh].timestamp_after = timestamp_utc2;
                  hist[nh].timestamp_mono_after = timestamp_mono2;
                  hist[nh].num_errors = 0;
               }

               // discard histogram(s) on too slow readout
               double dt = hist[nh].timestamp_mono_after - hist[nh].timestamp_mono_before;
               if (dt > self->cfg.histo_time_uncert) {
                  fmt = "%s: too slow readout (%.2fms), histogram discarded";
                  LOG4CPLUS_ERROR(self->getApplicationLogger(),
                                  toolbox::toString(fmt, id, dt * 1000));
                  ocr_sync = 0;
                  orb_init = -1;
                  continue;
               }

               // verify consistency of collected set of histograms
               if (nh > 0) {
                  if (hist[0].orb_init != uhist.orb_init ||
                      hist[0].orb_last != uhist.orb_last || already_got[t]) {
                     fmt = "%s: failed to collect full set of histograms, nibble discarded";
                     LOG4CPLUS_ERROR(self->getApplicationLogger(), toolbox::toString(fmt, id));

                     for (size_t i = 0; i < NUM_HISTOS; i++)
                        already_got[i] = false;

                     nh = 0;
                     ocr_sync = 0;
                     orb_init = -1;
                     counter++;
                  }
               }

               // verify continuity of orb_init
               if (nh == 0 && orb_init >= 0 && uhist.orb_init > 0 &&
                        uhist.orb_init != orb_init) {
                  fmt = "%s: unexpected non-continuity of orb_init/orb_last";
                  LOG4CPLUS_ERROR(self->getApplicationLogger(), toolbox::toString(fmt, id));
                  ocr_sync = 0;
                  orb_init = -1;
               }

               // hist.orb_init = 0 is considered as a fact of start of a new run.
               //
               // NOTE: if there is no new run for 4 days, hist.orb_init overflows and starts
               // over from zero or small positive number (if not aligned). Moreover, uHTR may
               // start working with zero orb_init. Those situations may be falsely taken as
               // beginning of a new run, but there is no way to fix that.
               if (uhist.orb_init == 0) {
                  ocr_sync = 1;
                  orb_init = -1;
                  counter = 0;
               }

               hist[nh].hist_id      = type[t];
               hist[nh].orb_init     = uhist.orb_init;
               hist[nh].orb_last     = uhist.orb_last;
               hist[nh].lumi_nibble  = uhist.lumi_nibble;
               hist[nh].lumi_section = uhist.lumi_section;
               hist[nh].cms_run      = uhist.cms_run;
               hist[nh].lhc_fill     = uhist.lhc_fill;

               size_t nBX = uhist.h.size();
               if (nBX > NUM_BXES) {
                  LOG4CPLUS_ERROR(self->getApplicationLogger(),
                                  toolbox::toString("%s: nBX > %i, histogram truncated", id,
                                                    NUM_BXES));
                  nBX = NUM_BXES;
               }
               for (size_t b = 0; b < nBX; b++)
                  hist[nh].h[b] = uhist.h[b];
               if (nBX < NUM_BXES) {
                  LOG4CPLUS_ERROR(self->getApplicationLogger(),
                                  toolbox::toString("%s: nBX < %i", id, NUM_BXES));
                  for (size_t b = nBX; b < NUM_BXES; b++)
                     hist[nh].h[b] = 0;
               }

               nh++;
               already_got[t] = true;

               // push full set of histograms to the calculator thread
               if (nh == NUM_HISTOS) {
                  for (size_t i = 0; i < NUM_HISTOS; i++) {
                     hist[i].ocr_sync = ocr_sync;
                     hist[i].counter = counter;
                  }

                  // NOTE: orb_last may be smaller than orb_init if uint32_t overflows; also,
                  // orb_last = 0 for every nibble after which an OCR signal follows
                  int64_t num_orbits = uhist.orb_last - uhist.orb_init;
                  if (num_orbits < 0 && uhist.orb_last != 0)
                     num_orbits += (int64_t) ((uint32_t) -1);

                  // discard histograms if they do not correspond to full range of orbits
                  if (num_orbits != self->cfg.orbits_per_nibble) {
                     if (report_overflow && uhist.orb_last != 0) {
                        fmt = "%s: discarding histograms with orb_last - orb_init != orbits_per_nibble";
                        LOG4CPLUS_ERROR(self->getApplicationLogger(),
                                        toolbox::toString(fmt, id));
                     }
                  }
                  // push histograms to the merger/calculator thread
                  else {
                     if (qcalc->Push(&hist) < 0) {
                        fmt = "%s: calculator queue full, histograms lost";
                        LOG4CPLUS_ERROR(self->getApplicationLogger(),
                                        toolbox::toString(fmt, id));
                     }
                  }

                  orb_init = hist[0].orb_last;
                  counter++;

                  nh = 0;
                  for (size_t i = 0; i < NUM_HISTOS; i++)
                     already_got[i] = false;
               }

            } // loop over histogram types

            // gather polling + whole readout cycle statistics
            if (nhist > 0) {
               double dt = gettime_monotonic() - timestamp_mono0;
               st_read_mean += dt;
               st_read_sigma += dt * dt;
               if (st_read_max < dt) st_read_max = dt;
               nreads++;
            }

            // reset the "no histograms" timer if a histogram arrived
            if (nhist > 0)
               time_last_histo = timestamp_mono2;

            // one-time change of report_overflow
            if (!report_overflow && nhist > 0)
               report_overflow = true;

            sleep(timestamp_mono0 + poll_period);

         } // polling loop

      } catch (std::exception& e) {
         LOG4CPLUS_ERROR(self->getApplicationLogger(),
                         toolbox::toString("%s: %s", id, e.what()));
      }

      self->DecreaseNumEnabled();

      // start over from the very beginning
      if (self->ThreadsContinue()) {
         sleep(gettime_monotonic() + 3.0); // sleep for 3 seconds

         LOG4CPLUS_INFO(self->getApplicationLogger(),
                        toolbox::toString("%s: starting over from the very beginning", id));
         continue;
      }

      break;
   } while (true);

   delete uHTR;
}
