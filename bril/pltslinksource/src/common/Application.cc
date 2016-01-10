// $Id

/*************************************************************************
 * XDAsignallication Template                     						 *
 * Copyright (C) 2000-2009, CERN.			               				 *
 * All rights reserved.                                                  *
 * Authors: L. Orsini, A. Forrest, A. Petrucci							 *
 *                                                                       *
 * For the licensing terms see LICENSE.		                         	 *
 * For the list of contributors see CREDITS.   					         *
 *************************************************************************/

#include <sstream>
#include "cgicc/CgiDefs.h"
#include "cgicc/Cgicc.h"
#include "cgicc/HTTPHTMLHeader.h"
#include "cgicc/HTMLClasses.h"
#include "xgi/framework/Method.h"

#include "bril/pltslinksource/Application.h"
#include "bril/pltslinksource/WebUtils.h"
#include "bril/pltslinksource/exception/Exception.h"
#include "interface/bril/PLTSlinkTopics.hh"

#include "xdata/Properties.h"

#include "b2in/nub/Method.h"
#include "toolbox/mem/HeapAllocator.h"
#include "toolbox/mem/MemoryPoolFactory.h"
#include "toolbox/TimeVal.h"
#include "toolbox/TimeInterval.h"
#include "toolbox/task/TimerFactory.h"

#include "toolbox/task/WorkLoop.h"
#include "toolbox/task/WorkLoopFactory.h"

#include "xcept/tools.h"
#include "xdata/InfoSpaceFactory.h"

#include <unistd.h>
#include "bril/pltslinksource/zmq.hpp"

XDAQ_INSTANTIATOR_IMPL (bril::pltslinksource::Application)

using namespace interface::bril;

bril::pltslinksource::Application::Application (xdaq::ApplicationStub* s) throw (xdaq::exception::Exception)
: xdaq::Application(s),xgi::framework::UIManager(this),eventing::api::Member(this),m_applock(toolbox::BSem::FULL)
{
  xgi::framework::deferredbind(this,this,&bril::pltslinksource::Application::Default, "Default");
  m_poolFactory  = toolbox::mem::getMemoryPoolFactory();
  m_appDescriptor= getApplicationDescriptor();
  m_classname    = m_appDescriptor->getClassName();
  m_instanceid   = 0;
  std::cout << "here" << std::endl;
  if( m_appDescriptor->hasInstanceNumber() ){
    m_instanceid = m_appDescriptor->getInstance();        
  }     
  //	std::string memPoolName = m_classname + m_instance.toString() + std::string("_memPool");
  std::cout << "here2" << std::endl;
  toolbox::mem::HeapAllocator* allocator = new toolbox::mem::HeapAllocator();
  toolbox::net::URN urn("toolbox-mem-pool",m_classname.toString()+"_"+m_instanceid.toString()+"_memPool");
  try{
     std::cout << "Creating Memory Pool" << std::endl;
     m_memPool = m_poolFactory->createPool(urn,allocator);
  }catch(xcept::Exception & e){
     XCEPT_RETHROW(xdaq::exception::Exception, "Could not create Memory Pool", e);
  }
  try{
    // Listen to app infospace
    getApplicationInfoSpace()->fireItemAvailable("bus",&m_bus);
    getApplicationInfoSpace()->fireItemAvailable("topics",&m_topics);
    getApplicationInfoSpace()->addListener(this, "urn:xdaq-event:setDefaultValues");
    m_appURN.fromString(m_appDescriptor->getURN());
  }catch( xcept::Exception & e){
    XCEPT_RETHROW(xdaq::exception::Exception, "Failed to put parameters into info spaces", e);
  }

  try{
    
    std::string nid("Pltslinksource_mon");
    
    std::string monurnPull = createQualifiedInfoSpace(nid).toString();
    m_monInfoSpacePull = dynamic_cast<xdata::InfoSpace* >(xdata::getInfoSpaceFactory()->find(monurnPull));
    m_monInfoSpacePull->fireItemAvailable("className", &m_classname);
    m_monItemListPull.push_back("className");
    m_monInfoSpacePull->fireItemAvailable("appURN", &m_appURN);
    m_monItemListPull.push_back("appURN");
	  
    m_monInfoSpacePull->addGroupRetrieveListener(this);
	  
	  
  }catch(xdata::exception::Exception& e){
    std::stringstream msg;
    msg<<"Failed to fire monitorning items";
    LOG4CPLUS_ERROR(getApplicationLogger(), stdformat_exception_history(e));
    XCEPT_RETHROW(bril::pltslinksource::exception::Exception,msg.str(),e);
  }catch(xdaq::exception::Exception& e){
    std::stringstream msg;
    msg<<"Failed to create infospace";
    LOG4CPLUS_ERROR(getApplicationLogger(), msg.str());
    XCEPT_RETHROW(bril::pltslinksource::exception::Exception, stdformat_exception_history(e),e);
  }catch(std::exception &e){
    LOG4CPLUS_ERROR(getApplicationLogger(), "std error "+std::string(e.what()));
    XCEPT_RAISE(bril::pltslinksource::exception::Exception, e.what());
  }
  
}

bril::pltslinksource::Application::~Application ()
{  
}

void bril::pltslinksource::Application::actionPerformed(xdata::Event& e){
  if( e.type() == "urn:xdaq-event:setDefaultValues"){           
    //count_=0;

    std::string timername(m_appURN.toString()+"_zmqstarter");
    try{
      toolbox::TimeVal zmqStart(2,0);
      toolbox::task::Timer *timer = toolbox::task::getTimerFactory()->createTimer(timername);
      //      toolbox::TimeVal start =toolbox::TimeVal::gettimeofday();
      timer->schedule(this, zmqStart, 0, timername);
    }catch(toolbox::exception::Exception& e){
      std::stringstream msg;
      msg << "failed to start timer" << timername;
      LOG4CPLUS_ERROR(getApplicationLogger(),msg.str()+stdformat_exception_history(e));
      XCEPT_RETHROW(bril::pltslinksource::exception::Exception,msg.str(),e);
    }
    
    
    if(!m_topics.value_.empty()){
      m_topiclist = toolbox::parseTokenSet(m_topics.value_,",");
    }
  }
}

void bril::pltslinksource::Application::actionPerformed(toolbox::Event& e){
  LOG4CPLUS_DEBUG(getApplicationLogger(), "Received toolbox event " << e.type());
  if ( e.type() == "eventing::api::BusReadyToPublish" ){
    std::string busname = (static_cast<eventing::api::Bus*>(e.originator()))->getBusName();
    std::stringstream msg;
    msg<< "Eventing bus '" << busname << "' is ready to publish";
    LOG4CPLUS_INFO(getApplicationLogger(),msg.str());
  }
}

/*
 * This is the default web page for this XDAQ application.
 */
void bril::pltslinksource::Application::Default (xgi::Input * in, xgi::Output * out) throw (xgi::exception::Exception)
{
  *out << "XDAQ PLT_source" << std::endl;
}

void bril::pltslinksource::Application::do_zmqclient(){

  //std::cout << "In zmq method" << std::endl;

  uint32_t NHits;
  uint32_t rows[500];
  uint32_t cols[500];
  uint32_t chas[500];
  uint32_t rocs[500];
  uint32_t adcs[500];

  zmq::context_t zmq_context(1);
  zmq::socket_t zmq_socket(zmq_context, ZMQ_SUB);

  int zmq_port = 7776;

  char address[1000];
  sprintf(address, "tcp://PLTSLINK:%d", zmq_port);

  std::cout << "zmq address " << address << std::endl;
  zmq_socket.connect(address);
  zmq_socket.setsockopt(ZMQ_SUBSCRIBE, "", 0);

  //  do_stoptimer();

  for(int iii = 0; iii<500; iii++){
    rows[iii] = 0;
    cols[iii] = 0;
    chas[iii] = 0;
    rocs[iii] = 0;
    adcs[iii] = 0;
  }
  NHits = 0;
  //std::cout << "before zmq loop" << std::endl;
  while(1){

    //std::cout << "waiting for message" << std::endl;                                                                                
    zmq::message_t zmq_message(2501*sizeof(uint32_t));
    if (zmq_socket.recv(&zmq_message)){

      char * messageBuffer[2501*sizeof(uint32_t)];
      memcpy(messageBuffer, (void*) zmq_message.data(), 2501*sizeof(uint32_t));


      //      std::cout << "Got a message with time " << time << " orbit " << orbit << " channel " << channel << " nibble " << nibble << " data " << (data[0]&0xfff) << " " << (data[1000]&0xfff) << " " << (data[3563]&0xfff) << std::endl;
      std::cout << "Publishing Data " << std::endl;

      publishData(pltslinkhistT::topicname(), 0, 0, 0, 0, 0, 0, 0, &messageBuffer,2501*sizeof(uint32_t));

    }
    
  }
}


void bril::pltslinksource::Application::publishData(const std::string&topic, uint32_t fill,uint32_t run, uint32_t ls,uint32_t nibble,uint32_t timeorbit, uint32_t orbit, uint32_t channel, void* payloadPtr, size_t payloadsize){
  LOG4CPLUS_DEBUG(getApplicationLogger(), "Publishing");
  toolbox::mem::Reference* bufRef = 0;
  try{      
    size_t totsize = pltslinkhistT::maxsize();
    bufRef = m_poolFactory->getFrame(m_memPool,totsize);
    bufRef->setDataSize(totsize);
    xdata::Properties plist;
    plist.setProperty("DATA_VERSION", DATA_VERSION);
    DatumHead* header = (DatumHead*)(bufRef->getDataLocation());
    toolbox::TimeVal t = toolbox::TimeVal::gettimeofday();
    header->setTime(fill,run,ls,nibble,t.sec(),t.millisec());
    header->setResource(DataSource::PLT,0,channel,StorageType::UINT32);
    header->setTotalsize(totsize);
    header->setFrequency(1);   
    memcpy(header->payloadanchor,payloadPtr,payloadsize);
    std::cout << "Try to publish" << std::endl;
    this->getEventingBus(m_bus.value_).publish(pltslinkhistT::topicname(), bufRef, plist);
  }catch (eventing::api::exception::Exception & e){
    // we are not doing any error handling in these examples. We just log the error
    LOG4CPLUS_ERROR(this->getApplicationLogger(), "Failed to publish, release message buffer");
    if(bufRef){
      bufRef->release();
      bufRef = 0;
    }
  }

}

 
void bril::pltslinksource::Application::timeExpired(toolbox::task::TimerEvent& e){
  std::cout << "Doing zmq client" << std::endl;
  do_zmqclient();
}



bool bril::pltslinksource::Application::reading(toolbox::task::WorkLoop* wl){
//  std::string topicname=m_classname.toString()+"Topic";
   return false;
}





