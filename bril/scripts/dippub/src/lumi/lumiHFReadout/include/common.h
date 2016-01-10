#ifndef LUMICOMMON_H
#define LUMICOMMON_H

#include <stdint.h>

// frequency of revolution of beams inside the LHC ring
#define REVOLUTION_FREQUENCY  11245.6

// nominal number of bunch crossings at the LHC
#define NUM_BXES 3564

// types of histograms (must start from 0 and must have no gaps in numbering) and their number
#define CMS1      0
#define CMS2      1
#define CMS_ET    2
#define CMS_VALID 3
#define NUM_HISTOS 4

// "histogram" with timestamping and integration information
struct Histogram_t {
   int32_t  uHTR_id;        // uHTR number (starting from 0); -1 = integrated over boards
   uint32_t hist_id;        // CMS1, CMS2, CMS_ET or CMS_VALID
   uint32_t num_atoms;      // number of histograms integrated together into this histogram
   uint32_t counter;        // consequtive lumi nibble or lumi section number
   uint32_t ocr_sync;       // if 1, "counter" is an absolute number from beginning of run

   // UTC timestamps and monotonic timestamps;
   // NOTE: monotonic timestamps are not affected by discontinuous jumps in the system time;
   // NOTE: "double" type gives approximately a microsecond precision
   double timestamp_before;      // seconds since EPOCH (UTC) before histogram creation
   double timestamp_after;       // seconds since EPOCH (UTC) after histogram creation
   double timestamp_mono_before; // seconds since some starting point before histogram creation
   double timestamp_mono_after;  // seconds since some starting point after histogram creation

   // from hcal::uhtr::uHTR::LumiHistogram
   uint64_t h[NUM_BXES];
   uint32_t orb_init;
   uint32_t orb_last;
   uint32_t lumi_nibble;
   uint32_t lumi_section;
   uint32_t cms_run;
   uint32_t lhc_fill;

   // - for atomic (read off from a single board) histogram, indicates its overflow status;
   // - for integral histogram, indicates total number of errors encountered while merging
   uint32_t num_errors;
};

// evaluated per lumi nibble/section luminosity information
struct Luminosity_t {
   double lumi;         // evaluated luminosity
   double elumi;        // its statistical error
   double timestamp1;   // approximate lumi nibble/section start time (UTC seconds since EPOCH)
   double timestamp2;   // approximate lumi nibble/section end time (UTC seconds since EPOCH)
};

// functions
extern double gettime_monotonic();
extern double gettime_utc();
extern void sleep(double time0);

#endif
