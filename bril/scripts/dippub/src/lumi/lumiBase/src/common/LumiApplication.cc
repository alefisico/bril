// Base class for lumi applications which is responsible for state transitions through SOAP
// requests. Two states are defined: stopped, running.

#include <xoap/Method.h>
#include <xoap/domutils.h>
#include <xoap/SOAPBody.h>
#include <xcept/Exception.h>
#include <xoap/SOAPEnvelope.h>
#include <xdaq/NamespaceURI.h>
#include <xoap/MessageFactory.h>
#include <xdata/InfoSpaceFactory.h>

#include "LumiApplication.h"

//_____________________________________________________________________________________________
LumiApplication::LumiApplication(xdaq::ApplicationStub* s)
   throw (xdaq::exception::Exception) :
   xdaq::Application(s)
{
   // Initialization.

   // NOTE: a separate variable (mStatePub) for external access is necessary because it can be
   // modified from outside of this class
   mState = STOPPED;
   mStatePub = "Stopped";

   // publish current state for external queries
   getApplicationInfoSpace()->fireItemAvailable("State", &mStatePub);

   // bindings to SOAP requests
   xoap::bind(this, &LumiApplication::fireTransition, "Start", XDAQ_NS_URI);
   xoap::bind(this, &LumiApplication::fireTransition, "Stop",  XDAQ_NS_URI);
}

//_____________________________________________________________________________________________
LumiApplication::~LumiApplication()
{
   if (mState == RUNNING)
      MakeTransition("Stop");
}

//_____________________________________________________________________________________________
xoap::MessageReference LumiApplication::fireTransition(xoap::MessageReference msg)
   throw (xoap::exception::Exception)
{
   // Parses msg and fires requested state transition.
   // This method is not for public use (it is bound to SOAP messages).
   //
   // NOTE: if msg contains several transitions, only the first one is performed.

   LOG4CPLUS_INFO(getApplicationLogger(), "SOAP message: state transition requested");

   // extract payload
   DOMNode* node = msg->getSOAPPart().getEnvelope().getBody().getDOMNode();
   DOMNodeList* bodyList = node->getChildNodes();

   // search for a transition to perform
   for (XMLSize_t i = 0; i < bodyList->getLength(); i++)
   {
      DOMNode* item = bodyList->item(i);

      if (item->getNodeType() != DOMNode::ELEMENT_NODE)
         continue;

      std::string cmd = xoap::XMLCh2String(item->getLocalName());

      if (cmd.compare("Start") != 0 && cmd.compare("Stop") != 0) {
         LOG4CPLUS_ERROR(getApplicationLogger(),
            "Code bug: LumiApplication::fireTransition() called on unknown transition request");
         XCEPT_RAISE(xoap::exception::Exception, "unknown transition request");
      }

      MakeTransition(cmd);

      // reply
      xoap::MessageReference reply = xoap::createMessage();
      xoap::SOAPEnvelope envelope = reply->getSOAPPart().getEnvelope();

      xoap::SOAPName respName = envelope.createName(cmd + "Response", "xdaq", XDAQ_NS_URI);
      xoap::SOAPBodyElement resp = envelope.getBody().addBodyElement(respName);

      // include current FSM state into the reply
      xoap::SOAPName stateName = envelope.createName("State", "xdaq", XDAQ_NS_URI);
      xoap::SOAPElement state = resp.addChildElement(stateName);

      boost::lock_guard<boost::mutex> lock(mMutex);
      state.addAttribute(stateName, (mState == RUNNING ? "Running" : "Stopped"));

      return reply;
   }

   LOG4CPLUS_ERROR(getApplicationLogger(),
      "Code bug: LumiApplication::fireTransition() called, but no transition request found");
   XCEPT_RAISE(xoap::exception::Exception, "no transition request found");
}

//_____________________________________________________________________________________________
void LumiApplication::MakeTransition(std::string transition)
{
   // Performs state transitions (stopped <=> running).
   //
   // transition = "Stop" or "Start".

   boost::lock_guard<boost::mutex> lock(mMutex);

   if (transition.compare("Start") == 0) {
      if (mState == STOPPED) {
         LOG4CPLUS_INFO(getApplicationLogger(), "Going into running state");

         mStatePub = "Starting";
         if (Start()) {
            mStatePub = "Running";
            mState = RUNNING;
            LOG4CPLUS_INFO(getApplicationLogger(), "In state: running");
         } else {
            mStatePub = "Stopped";
            LOG4CPLUS_ERROR(getApplicationLogger(), "Transition into running state failed");
            LOG4CPLUS_INFO(getApplicationLogger(), "In state: stopped");
         }
      }
      else
         LOG4CPLUS_INFO(getApplicationLogger(), "Already in running state, doing nothing");
   } // Start

   else if (transition.compare("Stop") == 0) {
      if (mState == RUNNING) {
         LOG4CPLUS_INFO(getApplicationLogger(), "Going into stopped state");

         mStatePub = "Stopping";
         if (Stop()) {
            mStatePub = "Stopped";
            mState = STOPPED;
            LOG4CPLUS_INFO(getApplicationLogger(), "In state: stopped");
         } else {
            mStatePub = "Running";
            LOG4CPLUS_ERROR(getApplicationLogger(), "Transition into stopped state failed");
            LOG4CPLUS_INFO(getApplicationLogger(), "In state: running");
         }
      }
      else
         LOG4CPLUS_INFO(getApplicationLogger(), "Already in stopped state, doing nothing");
   } // Stop

}
