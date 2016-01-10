#ifndef LUMIQUEUE_H
#define LUMIQUEUE_H

#include <boost/thread.hpp>

class LumiQueue {
  public:

   LumiQueue(size_t element_size, size_t num_elements);
   ~LumiQueue();

   int Push(void* data);
   int Pop(void* data, double timeout);

   void Reset();

  protected:

   boost::mutex               mMutex;     // serializes read/write access
   boost::condition_variable  mNotEmpty;  // signals when queue is not empty

   size_t  mCapacity;            // number of elements queue can fit
   size_t  mElementSize;         // element size in bytes

   char*   mData;                // pointer to queued data (= array of elements)

   size_t  mRi;                  // current read element index (SIZE_MAX = queue is full)
   size_t  mWi;                  // current write element index
};

#endif
