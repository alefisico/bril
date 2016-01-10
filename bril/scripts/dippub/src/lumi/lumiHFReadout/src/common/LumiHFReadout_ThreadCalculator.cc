#include <math.h>
#include <string.h>
#include <xdata/UnsignedInteger32.h>
#include <toolbox/mem/HeapAllocator.h>
#include <toolbox/mem/MemoryPoolFactory.h>

#include "common.h"
#include "LumiHFReadout.h"

// maximum numbers of simultaneously constructed nibbles and sections
#define NN 4
#define SS 3

using namespace lumi;

//_____________________________________________________________________________________________
void merge_histograms(LumiHFReadout* self, Histogram_t& dest, Histogram_t& src,
                      bool lumi_section = false)
{
   // Merges together histograms src and dest, saving result into dest.
   //
   // If lumi_section is true, src must be atomic, i.e. src.num_atoms = 1.

   // if dest is empty
   if (dest.num_atoms == 0) {
      memcpy(&dest, &src, sizeof(dest));

      if (lumi_section)
         dest.counter /= self->cfg.nibbles_per_section;

      return;
   }

   dest.num_atoms += src.num_atoms;
   dest.num_errors += src.num_errors;

   if (dest.timestamp_before > src.timestamp_before)
      dest.timestamp_before = src.timestamp_before;

   if (dest.timestamp_after < src.timestamp_after)
      dest.timestamp_after = src.timestamp_after;

   if (dest.timestamp_mono_before > src.timestamp_mono_before)
      dest.timestamp_mono_before = src.timestamp_mono_before;

   if (dest.timestamp_mono_after < src.timestamp_mono_after)
      dest.timestamp_mono_after = src.timestamp_mono_after;

   // merge bins
   for (size_t b = 0; b < NUM_BXES; b++)
      dest.h[b] += src.h[b];

   bool sync = (src.ocr_sync == 1 && dest.ocr_sync == 1);

   // check alignment of orb_init
   if ((!lumi_section && src.orb_init != dest.orb_init) ||
       (lumi_section && (src.orb_init - dest.orb_init) % self->cfg.orbits_per_nibble != 0)) {
      if (sync)
         LOG4CPLUS_ERROR(self->getApplicationLogger(),
                "Calculator: merging orbit-aligned histograms with not aligned hist.orb_init");
      dest.num_errors++;
   }

   // check alignment of orb_last
   if ((!lumi_section && src.orb_last != dest.orb_last) ||
      (lumi_section && (src.orb_last - dest.orb_last) % self->cfg.orbits_per_nibble != 0)) {
      if (sync)
         LOG4CPLUS_ERROR(self->getApplicationLogger(),
                "Calculator: merging orbit-aligned histograms with not aligned hist.orb_last");
      dest.num_errors++;
   }

   if (sync) {
      if ((!lumi_section && src.counter != dest.counter) ||
          (lumi_section && src.counter/self->cfg.nibbles_per_section != dest.counter)) {
         LOG4CPLUS_ERROR(self->getApplicationLogger(),
                   "Calculator: merging orbit-aligned histograms with different hist.counter");
         dest.num_errors++;
      }
   }

   if (!lumi_section && src.lumi_nibble != dest.lumi_nibble) {
//       if (sync) // TODO: uncomment
//          LOG4CPLUS_ERROR(self->getApplicationLogger(),
//                "Calculator: merging orbit-aligned histograms with different hist.lumi_nibble");
      dest.num_errors++;
   }

   if (src.lumi_section != dest.lumi_section) {
//       if (sync) // TODO: uncomment
//          LOG4CPLUS_ERROR(self->getApplicationLogger(),
//               "Calculator: merging orbit-aligned histograms with different hist.lumi_section");
      dest.num_errors++;
   }

   if (src.cms_run != dest.cms_run) {
//       if (sync) // TODO: uncomment
//          LOG4CPLUS_ERROR(self->getApplicationLogger(),
//                    "Calculator: merging orbit-aligned histograms with different hist.cms_run");
      dest.num_errors++;
   }

   if (src.lhc_fill != dest.lhc_fill) {
//       if (sync) // TODO: uncomment
//          LOG4CPLUS_ERROR(self->getApplicationLogger(),
//                   "Calculator: merging orbit-aligned histograms with different hist.lhc_fill");
      dest.num_errors++;
   }

   if (dest.ocr_sync == 0 && src.ocr_sync == 1) {
      dest.orb_init = src.orb_init;
      dest.orb_last = src.orb_last;
   }
   else if (sync || (src.ocr_sync == 0 && dest.ocr_sync == 0)) {
      if (dest.orb_init > src.orb_init)
         dest.orb_init = src.orb_init;

      if (dest.orb_last < src.orb_last)
         dest.orb_last = src.orb_last;
   }

   if (src.ocr_sync == 1 || dest.ocr_sync == 0) {
      dest.counter = src.counter;
      if (lumi_section)
         dest.counter /= self->cfg.nibbles_per_section;

      dest.lumi_nibble = src.lumi_nibble;
      dest.lumi_section = src.lumi_section;
      dest.cms_run = src.cms_run;
      dest.lhc_fill = src.lhc_fill;
   }

   if (src.ocr_sync == 1)
      dest.ocr_sync = 1;

   if (src.uHTR_id != dest.uHTR_id)
      dest.uHTR_id = -1;  // -1 = integrated over boards
}

//_____________________________________________________________________________________________
void push_to_eventing(LumiHFReadout* self, void* data, size_t data_size,
                      std::string type, std::string msg, eventing::api::Bus* ebus,
                      toolbox::mem::MemoryPoolFactory* mfactory, toolbox::mem::Pool* mpool)
{
   // Queues data to eventing bus.

   if (!ebus->canPublish()) {
      LOG4CPLUS_ERROR(self->getApplicationLogger(),
                      "Calculator: eventing bus not ready for publishing, data lost");
      return;
   }

   toolbox::mem::Reference* ref;

   try {
      ref = mfactory->getFrame(mpool, data_size);
   } catch(toolbox::mem::exception::Exception& e) {
      const char* fmt = "Failed to allocate memory buffer (%s), data lost";
      LOG4CPLUS_ERROR(self->getApplicationLogger(), toolbox::toString(fmt, e.what()));
      return;
   }

   memcpy(ref->getDataLocation(), data, data_size);

   xdata::Properties plist;
   plist.setProperty("TYPE", type);
   plist.setProperty("MSG", msg);

   try {
      ebus->publish(type, ref, plist);
   } catch(xcept::Exception& e) {
      const char* fmt = "eventing::api::Bus::publish() failed (%s), data lost";
      LOG4CPLUS_ERROR(self->getApplicationLogger(), toolbox::toString(fmt, e.what()));

      if (ref) ref->release();
   }
}

//_____________________________________________________________________________________________
void eval_send_lumi(LumiHFReadout* self, Histogram_t& hist, Histogram_t& valid,
                    std::string type, eventing::api::Bus* ebus,
                    toolbox::mem::MemoryPoolFactory* mfactory, toolbox::mem::Pool* mpool)
{
   // Evaluates luminosity from occupancy histogram and pushes it to eventing bus.
   // Does nothing if histo.num_atoms = 0.

   // do nothing if no data
   if (hist.num_atoms == 0)
      return;

   double nfired = 0;  // number of fired (above threshold) channels
   double ntotal = 0;  // total number of measured channels

   // evaluate occupancies
   for (size_t b = 0; b < NUM_BXES; b++) {
      nfired += hist.h[b];
      ntotal += valid.h[b];
   }

   if (ntotal < 0.5) {
      const char* fmt = "Calculator: ntotal = 0, %s cannot be evaluated";
      LOG4CPLUS_ERROR(self->getApplicationLogger(), toolbox::toString(fmt, type.c_str()));
      return;
   }

   double nempty = ntotal - nfired;

   if (nempty < 0.5) {
      const char* fmt = "Calculator: nfired >= ntotal, %s cannot be evaluated";
      LOG4CPLUS_ERROR(self->getApplicationLogger(), toolbox::toString(fmt, type.c_str()));
      return;
   }

   Luminosity_t data;

   data.lumi = -log(nempty/ntotal);
   data.elumi = sqrt(1.0/nempty - 1.0/ntotal);
   data.timestamp1 = hist.timestamp_before;
   data.timestamp2 = hist.timestamp_after;

   push_to_eventing(self, &data, sizeof(data), type, "", ebus, mfactory, mpool);
}

//_____________________________________________________________________________________________
void ThreadCalculator(LumiHFReadout* self, LumiQueue* qcalc, eventing::api::Bus* ebus)
{
   // Thread which is responsible for merging of histograms and evaluation of luminosity.
   //
   // uHTR boards are synchronized on common real-time clock, but their mutual alignment in
   // orbit numbers happens only after receiving the first OCR (orbit counter reset) signal
   // (which indicates start of a new run). Due to this, for universality and simplicity,
   // histograms are merged together into lumi nibbles and lumi sections based on their
   // timestamps.

   // give uHTR boards time to initialize
   sleep(gettime_monotonic() + 1);  // 1 second

   uint32_t nibbles_per_section = (uint32_t) self->cfg.nibbles_per_section;

   // lumi nibble size (in seconds)
   double nibble_size = 1./REVOLUTION_FREQUENCY * self->cfg.orbits_per_nibble;

   // total number of uHTR boards
   size_t num_boards = self->cfg.addr.size();

   // very complicated way to allocate memory (create a memory pool with unique name);
   // additional complication is that one has to pass the factory and the pool arguments
   // to the functions defined above
   toolbox::mem::MemoryPoolFactory* mfactory = toolbox::mem::getMemoryPoolFactory();
   xdaq::ApplicationDescriptor* ad = self->getApplicationDescriptor();
   toolbox::net::URN urn("toolbox-mem-pool", ad->getClassName() +
                         ((xdata::UnsignedInteger32)ad->getInstance()).toString() + "_mpool");
   // FIXME: a memory leak since no "delete" statement?
   toolbox::mem::Pool* mpool = mfactory->createPool(urn, new toolbox::mem::HeapAllocator());

   // histograms; up to NN nibbles and up to SS sections are collected simultaneously
   Histogram_t hist[NUM_HISTOS];                       // current set taken from the queue
   Histogram_t hist_nibble[NN][NUM_HISTOS];            // per nibble, integrated over boards
   Histogram_t hist_section[SS][NUM_HISTOS];           // per section, integrated over boards

   // per board histograms, integrated over section;
   // allocate memory on heap to avoid segfauls due to exceeding stack size
   Histogram_t (*hist_board)[SS][NUM_HISTOS] = new Histogram_t[num_boards][SS][NUM_HISTOS];

   // timeouts of construction of nibbles and sections
   double timeout_nibble[NN];
   double timeout_section[SS];

   // maximum end times of lumi sections
   double timeend_section[SS];

   // for evaluation of "centers of gravity"
   double timesum_nibble[NN];    // weighted sum of average timestamps in nibble
   double wsum_nibble[NN];       // sum of weights

   // to indicate which boards were already counted in nibbles
   bool already_merged[NN][num_boards];

   size_t ni = 0;    // (circular) index of first being constructed nibble
   size_t si = 0;    // (circular) index of first being constructed section
   size_t ntot = 0;  // number of currently being constructed nibbles
   size_t stot = 0;  // number of currently being constructed sections

   // if true, at least one board is orbit-aligned and knows absolute nibble numbers
   bool orbit_sync = false;

   while (self->ThreadsContinue()) {
      // determine maximum sleep time
      double delta = 1; // 1 second
      if (ntot > 0) {
         double dt = timeout_nibble[ni] - gettime_monotonic();
         if (dt < delta) delta = dt;
      }
      if (stot > 0) {
         double dt = timeout_section[si] - gettime_monotonic();
         if (dt < delta) delta = dt;
      }
      if (delta < 0) delta = 0;

      // wait for new set of histograms
      if (qcalc->Pop(&hist, delta) < 0) {
         // timeout, no histograms arrived

         double currtime = gettime_monotonic();

         // finish oldest lumi nibble
         if (ntot > 0 && currtime >= timeout_nibble[ni]) {
            const char* fmt = "Calculator: timeout, finishing nibble with %u/%u histograms";
            LOG4CPLUS_ERROR(self->getApplicationLogger(),
                            toolbox::toString(fmt, hist_nibble[ni][0].num_atoms, num_boards));

            eval_send_lumi(self, hist_nibble[ni][CMS1], hist_nibble[ni][CMS_VALID],
                           "luminosity_nibble", ebus, mfactory, mpool);

            ni = (ni + 1) % NN;
            ntot--;
         }

         // finish oldest lumi section
         if (stot > 0 && currtime >= timeout_section[si]) {
            const char* fmt = "Calculator: timeout, finishing section with %u/%u histograms";
            LOG4CPLUS_ERROR(self->getApplicationLogger(),
                            toolbox::toString(fmt, hist_section[si][0].num_atoms,
                                                   num_boards * nibbles_per_section));

            eval_send_lumi(self, hist_section[si][CMS1], hist_section[si][CMS_VALID],
                           "luminosity_section", ebus, mfactory, mpool);

            for (size_t b = 0; b < num_boards; b++)
               if (hist_board[b][si][0].num_atoms > 0)
                  push_to_eventing(self, hist_board[b][si], sizeof(Histogram_t) * NUM_HISTOS,
                                  "histograms_board", self->cfg.uid[b], ebus, mfactory, mpool);

            if (hist_section[si][0].num_atoms > 0)
               push_to_eventing(self, hist_section[si], sizeof(Histogram_t) * NUM_HISTOS,
                                "histograms_section", "integrated", ebus, mfactory, mpool);

            si = (si + 1) % SS;
            stot--;
         }

         continue;
      } // timeout, no histograms arrived

      // new set of histograms arrived

      // warn if lumi_nibble is not in the range 1-64 (feature of TCDS)
      for (size_t t = 0; t < NUM_HISTOS; t++)
         if (hist[t].lumi_nibble < 1 || hist[t].lumi_nibble > 64) {
            LOG4CPLUS_ERROR(self->getApplicationLogger(),
                            "Calculator: hist.lumi_nibble not in 1-64 range");
            break;
         }

      // re-evaluate nibble and section numbers (TCDS forces lumi section = 64 lumi nibbles)
      if (nibbles_per_section != 64)
         for (size_t t = 0; t < NUM_HISTOS; t++) {
            int64_t nibble_num = (int64_t)hist[t].lumi_section * 64 +
                                 (int64_t)hist[t].lumi_nibble - 1;
            hist[t].lumi_section = (uint32_t) (nibble_num / nibbles_per_section);
            hist[t].lumi_nibble  = (uint32_t) (nibble_num % nibbles_per_section) + 1;
         }

      size_t board = (size_t) hist[0].uHTR_id;
      double t0 = 0.5 * (hist[0].timestamp_mono_before + hist[0].timestamp_mono_after);

      // find positional number of best-suited lumi nibble using timestamps
      size_t n_best;
      for (n_best = ntot; n_best > 0; n_best--) {
         size_t ii = (ni + n_best - 1 + NN) % NN;  // index of previous nibble

         if (hist[0].ocr_sync != 1 && already_merged[ii][board])
            break;

         if (t0 - timesum_nibble[ii]/wsum_nibble[ii] > 0.67 * nibble_size)
            break;
      }

      // index of the best-suited nibble
      size_t ni_best = (ni + n_best) % NN;

      // this should never happen since histogram delivery is approximately time-ordered
      if (n_best < ntot &&
          t0 - timesum_nibble[ni_best]/wsum_nibble[ni_best] < -0.5 * nibble_size) {
         LOG4CPLUS_ERROR(self->getApplicationLogger(),
                         "Calculator: too old histograms, discarded; sw bug?");
         continue;
      }

      // cache may get full in case of multiple OCR signals received in a short time interval
      if (n_best == ntot && ntot == NN) {
         LOG4CPLUS_ERROR(self->getApplicationLogger(),
                         "Calculator: cache of nibbles full, discarding the oldest nibble");
         ni = (ni + 1) % NN;
         ntot--;
         n_best = ntot;
      }

      // initialization of new lumi nibble
      if (n_best == ntot) {
         for (size_t t = 0; t < NUM_HISTOS; t++)  // mark histograms empty
            hist_nibble[ni_best][t].num_atoms = 0;

         for (size_t b = 0; b < num_boards; b++)
            already_merged[ni_best][b] = false;

         // NOTE: there are delays in histogram delivery from the readout threads to the
         // calculator thread; possible variations in those delays are neglected in the
         // formula below
         timeout_nibble[ni_best] = gettime_monotonic() + nibble_size
                                                       + self->cfg.histo_time_uncert;
         timesum_nibble[ni_best] = 0;
         wsum_nibble[ni_best] = 0;

         ntot++;
      }

      // find positional number of best-suited lumi section using timestamps
      size_t s_best;
      for (s_best = stot; s_best > 0; s_best--) {
         size_t ii = (si + s_best - 1 + SS) % SS;  // index of previous section

         if (t0 > timeend_section[ii])
            break;
      }

      // index of best-suited section
      size_t si_best = (si + s_best) % SS;

      // force to start new lumi section if new run just started
      if (s_best < stot && hist[0].counter == 0 && hist[0].ocr_sync == 1 &&
          (hist_section[si_best][0].ocr_sync == 0 ||
           t0 > timeend_section[si_best] - nibble_size * (nibbles_per_section - 1))) {

         // update times of previous lumi section
         timeend_section[si_best] = t0 - 0.5 * nibble_size;
         timeout_section[si_best] = gettime_monotonic() + 0.5 * nibble_size
                                                        + self->cfg.histo_time_uncert;
         s_best++;
         si_best = (si + s_best) % SS;
      }

      // cache may get full in case of multiple OCR signals received in a short time interval
      if (s_best == stot && stot == SS) {
         LOG4CPLUS_ERROR(self->getApplicationLogger(),
                         "Calculator: cache of sections full, discarding the oldest section");
         si = (si + 1) % SS;
         stot--;
         s_best = stot;
      }

      // initialization of new lumi section
      if (s_best == stot) {
         for (size_t t = 0; t < NUM_HISTOS; t++) { // mark histograms empty
            hist_section[si_best][t].num_atoms = 0;

            for (size_t b = 0; b < num_boards; b++)
               hist_board[b][si_best][t].num_atoms = 0;
         }

         timeend_section[si_best] = t0 + nibble_size * (nibbles_per_section - 0.5);
         timeout_section[si_best] = gettime_monotonic() + nibble_size * nibbles_per_section
                                                        + self->cfg.histo_time_uncert;
         stot++;
      }

      // update section's times
      if (hist[0].ocr_sync == 1) {
         int nrest = (int)nibbles_per_section - (int)(hist[0].counter % nibbles_per_section);
         double timeout = gettime_monotonic() + nibble_size * nrest
                                              + self->cfg.histo_time_uncert;
         if (timeout_section[si_best] > timeout)
            timeout_section[si_best] = timeout;

         if (hist_section[si_best][0].ocr_sync == 0)
            timeend_section[si_best] = t0 + nibble_size * (nrest - 0.5);
      }

      // merge histograms into nibble
      for (size_t t = 0; t < NUM_HISTOS; t++)
         merge_histograms(self, hist_nibble[ni_best][hist[t].hist_id], hist[t], false);

      if (hist[0].ocr_sync == 0) {
         timesum_nibble[ni_best] += t0;
         wsum_nibble[ni_best] += 1;
      }
      else {
         timesum_nibble[ni_best] += 1e+5 * t0;
         wsum_nibble[ni_best] += 1e+5;
      }

      already_merged[ni_best][board] = true;

      // check whether current nibble is fully constructed
      if (hist_nibble[ni_best][0].num_atoms == (uint32_t) self->GetNumEnabled()) {
         // discard older nibbles if newer nibble is fully constructed
         if (n_best > 0) {
            LOG4CPLUS_ERROR(self->getApplicationLogger(),
                            "Calculator: newer nibble finished, discarding older nibble(s)");
            ntot = ntot - n_best;
            ni = ni_best;
         }

         if (hist_nibble[ni][0].ocr_sync == 1) {
            if (hist_nibble[ni][0].counter == 0)
               LOG4CPLUS_INFO(self->getApplicationLogger(),
                              "Calculator: OCR signal received, new run started");

            if (!orbit_sync)
               LOG4CPLUS_INFO(self->getApplicationLogger(),
                              "Calculator: from now on, absolute nibble numbers are known");
            orbit_sync = true;
         }
         else {
            if (orbit_sync)
               LOG4CPLUS_WARN(self->getApplicationLogger(),
                     "Calculator: from now on, absolute nibble numbers are not known anymore");
            orbit_sync = false;
         }

         eval_send_lumi(self, hist_nibble[ni][CMS1], hist_nibble[ni][CMS_VALID],
                        "luminosity_nibble", ebus, mfactory, mpool);

         ni = (ni + 1) % NN;
         ntot--;
      }

      // merge histograms into section
      for (size_t t = 0; t < NUM_HISTOS; t++) {
         uint32_t t2 = hist[t].hist_id;
         merge_histograms(self, hist_section[si_best][t2], hist[t], true);
         merge_histograms(self, hist_board[board][si_best][t2], hist[t], true);
      }

      // check whether current section is fully constructed
      if (hist_section[si_best][0].num_atoms ==
                                (uint32_t) self->GetNumEnabled() * nibbles_per_section) {
         // discard older sections if newer section is fully constructed
         if (s_best > 0) {
            LOG4CPLUS_ERROR(self->getApplicationLogger(),
                            "Calculator: newer section finished, discarding older sections");
            stot = stot - s_best;
            si = si_best;
         }

         eval_send_lumi(self, hist_section[si][CMS1], hist_section[si][CMS_VALID],
                        "luminosity_section", ebus, mfactory, mpool);

         for (size_t b = 0; b < num_boards; b++)
            if (hist_board[b][si][0].num_atoms > 0)
               push_to_eventing(self, hist_board[b][si], sizeof(Histogram_t) * NUM_HISTOS,
                                "histograms_board", self->cfg.uid[b], ebus, mfactory, mpool);

         if (hist_section[si][0].num_atoms > 0)
            push_to_eventing(self, hist_section[si], sizeof(Histogram_t) * NUM_HISTOS,
                             "histograms_section", "integrated", ebus, mfactory, mpool);

         si = (si + 1) % SS;
         stot--;
      }

   } // waiting loop

   delete [] hist_board;

   // FIXME: delete mpool?
}
