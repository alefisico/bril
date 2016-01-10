// Thread-safe multiple writers => single reader FIFO queue of elements of fixed size.

#include <string.h>
#include <boost/date_time/posix_time/posix_time.hpp>

#include "LumiQueue.h"

#define SIZE_MAX ((size_t)-1)

//_____________________________________________________________________________________________
LumiQueue::LumiQueue(size_t element_size, size_t num_elements)
{
   // Queue initialization.

   mCapacity = num_elements;
   mElementSize = element_size;

   mRi = 0;
   mWi = 0;

   mData = new char[num_elements * element_size];
}

//_____________________________________________________________________________________________
LumiQueue::~LumiQueue()
{
   delete [] mData;
}

//_____________________________________________________________________________________________
int LumiQueue::Push(void* data)
{
   // Appends new element into the queue.
   // Returns 0 on success or -1 if the queue is already full.

   boost::lock_guard<boost::mutex> lock(mMutex);

   // check that the queue is not full
   if (mRi == SIZE_MAX) return -1;

   // copy data into the queue
   memcpy(&mData[mWi * mElementSize], data, mElementSize);

   // update read/write indices
   mWi++;
   if (mWi >= mCapacity) mWi = 0;
   if (mWi == mRi) mRi = SIZE_MAX;  // queue is now full

   // notify the reader
   mNotEmpty.notify_one();

   return 0;
}

//_____________________________________________________________________________________________
int LumiQueue::Pop(void* data, double timeout)
{
   // Extracts next element from the queue.
   // Returns 0 if the element is extracted or -1 otherwise.
   //
   // This function blocks while the queue is empty, until a specified timeout (in seconds) or
   // until a spurious wakeup.

   boost::unique_lock<boost::mutex> lock(mMutex);

   // wait for signal or timeout if the queue is empty
   if (mRi == mWi) {
      boost::posix_time::time_duration dt = boost::posix_time::seconds((long)timeout) +
                                            boost::posix_time::microseconds((long)((timeout - (long)timeout)*1e+6));
      mNotEmpty.timed_wait(lock, dt);

      // if the queue is still empty
      if (mRi == mWi) return -1;
   }

   // read off one element
   if (mRi == SIZE_MAX) mRi = mWi; // if queue is full
   memcpy(data, &mData[mRi * mElementSize], mElementSize);

   // update read index
   mRi++;
   if (mRi >= mCapacity) mRi = 0;

   return 0;
}

//_____________________________________________________________________________________________
void LumiQueue::Reset()
{
   // Empties the queue.

   boost::lock_guard<boost::mutex> lock(mMutex);

   mRi = mWi = 0;
}
