// Common pieces of code.

#include <time.h>

//_____________________________________________________________________________________________
double gettime_monotonic()
{
   // Returns monotonic time (in seconds) since some unspecified starting point. This time is
   // not affected by discontinuous jumps in the system time.

   struct timespec t;
   clock_gettime(CLOCK_MONOTONIC, &t);

   return (double)t.tv_sec + (double)t.tv_nsec * 1e-9;
}

//_____________________________________________________________________________________________
double gettime_utc()
{
   // Returns number of seconds since EPOCH for the current UTC time.
   // NOTE: double gives approximately a microsecond precision.

   struct timespec t;
   clock_gettime(CLOCK_REALTIME, &t);

   return (double)t.tv_sec + (double)t.tv_nsec * 1e-9;
}

//_____________________________________________________________________________________________
void sleep(double time0)
{
   // Suspends execution of the calling thread until time0 moment in time.
   //
   // time0 = time in seconds measured with respect to the CLOCK_MONOTONIC clock,
   // see gettime_monotonic().

   double dt = time0 - gettime_monotonic();

   while (dt > 0) {
      struct timespec t;
      t.tv_sec = (time_t) dt;
      t.tv_nsec = (long) ((dt - t.tv_sec) * 1e+9);

      if (nanosleep(&t, NULL) == 0)
         return; // successful sleep

      dt = time0 - gettime_monotonic();
   }
}
