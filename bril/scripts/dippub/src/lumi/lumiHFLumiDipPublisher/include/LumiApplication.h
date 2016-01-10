#ifndef LUMIAPPLICATION_H
#define LUMIAPPLICATION_H

#include <string>
#include <boost/thread.hpp>

#include <xdata/String.h>
#include <xdaq/Application.h>
#include <xoap/MessageReference.h>

class LumiApplication: public xdaq::Application {
  public:

   LumiApplication(xdaq::ApplicationStub* s) throw (xdaq::exception::Exception);
   virtual ~LumiApplication();

   // this method is not for public use (bound to SOAP messages)
   xoap::MessageReference fireTransition(xoap::MessageReference msg)
      throw (xoap::exception::Exception);

  protected:

   // state transitions to be redefined in daughter classes
   virtual bool Start() { return true; }  // true=success, false=stay in stopped state
   virtual bool Stop()  { return true; }  // true=success, false=stay in running state

   void MakeTransition(std::string transition);

  private:

   enum {STOPPED, RUNNING};

   // current state (STOPPED or RUNNING)
   int mState;

   // current state (Stopped/Starting/Running/Stopping) exported for external access
   xdata::String mStatePub;

   // serializes access to MakeTransition() as well as to the data members above
   boost::mutex  mMutex;
};

#endif
