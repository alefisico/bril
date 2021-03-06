// $Id$

/*************************************************************************
 * XDAQ Application Template                     						 *
 * Copyright (C) 2000-2009, CERN.			               				 *
 * All rights reserved.                                                  *
 * Authors: L. Orsini, A. Forrest, A. Petrucci							 *
 *                                                                       *
 * For the licensing terms see LICENSE.		                         	 *
 * For the list of contributors see CREDITS.   					         *
 *************************************************************************/

#ifndef _bril_pltslinksource_Application_h_
#define _bril_pltslinksource_Application_h_

#include <string>

#include "xdaq/Application.h"

#include "xgi/Method.h"
#include "xgi/Output.h"
#include "xgi/exception/Exception.h"
#include "xgi/framework/UIManager.h"

#include "eventing/api/Subscriber.h"
#include "eventing/api/Publisher.h"
#include "eventing/api/Member.h"

#include "xdata/ActionListener.h"
#include "xdata/InfoSpace.h"
#include "xdata/Boolean.h"
#include "xdata/String.h"
#include "xdata/Float.h"
#include "xdata/Table.h"
#include "xdata/Serializable.h"

#include "b2in/nub/exception/Exception.h"

#include "toolbox/task/TimerListener.h"
#include "toolbox/mem/MemoryPoolFactory.h"
#include "toolbox/mem/Reference.h"
#include "xdata/UnsignedInteger32.h"
#include "xdata/UnsignedShort.h"
#include "toolbox/BSem.h"

namespace toolbox {
  namespace mem{
    class MemoryPoolFactory;
  }
}

namespace toolbox{
	namespace task{
		class WorkLoop;
	}
}

namespace bril
{
	namespace pltslinksource
	{
	  class Application : public xdaq::Application, public xgi::framework::UIManager,public eventing::api::Member, public xdata::ActionListener, public toolbox::ActionListener,public toolbox::task::TimerListener
	    {
	    public:
	      
	      XDAQ_INSTANTIATOR();
	      
	      Application (xdaq::ApplicationStub* s) throw (xdaq::exception::Exception);
	      ~Application ();
	      
	      void Default (xgi::Input * in, xgi::Output * out) throw (xgi::exception::Exception);
	      
	      //void sendMessage (xgi::Input * in, xgi::Output * out) throw (xgi::exception::Exception);
	      virtual void actionPerformed(xdata::Event& e);
	      virtual void actionPerformed(toolbox::Event& e);
	      virtual void timeExpired(toolbox::task::TimerEvent& e);
              void onMessage(toolbox::mem::Reference * ref, xdata::Properties & plist) throw (b2in::nub::exception::Exception);

	    protected:
	      
	      // We are keeping track of the number of messages we have sent
	      //size_t count_;
	      //void do_stoptimer();
	      //void do_starttimer();
	      void publishData(const std::string&topic,uint32_t fill,uint32_t run, uint32_t ls,uint32_t nibble,uint32_t timeorbit, uint32_t orbit, uint32_t channel, void* payloadPtr, size_t payloadsize);
	      void simPLTData(uint32_t fillnum, uint32_t runnum,uint32_t lsnum,uint32_t nbnum);
              void makePLTData(uint32_t fillnum,uint32_t runnum,uint32_t lsnum,uint32_t nbnum,void* messageBuffer,size_t messageSize);
	      void do_zmqclient();
	      bool reading(toolbox::task::WorkLoop* wl);
	      void setActiveChannels(uint32_t trigmask, uint32_t channelId);
	      toolbox::mem::MemoryPoolFactory *m_poolFactory;
	      xdaq::ApplicationDescriptor *m_appDescriptor;
	      xdata::String m_classname;
	      xdata::String m_appURN;
	      xdata::UnsignedInteger32 m_instanceid;
	      toolbox::mem::Pool* m_memPool;
	      std::string m_timername;

              toolbox::task::WorkLoop* m_readingwl;
	      xdata::Boolean m_simSource;
	      xdata::String m_signalTopic;
	      xdata::String m_bus;
	      xdata::String m_topics; 
	      std::set<std::string> m_topiclist;
	      
	      xdata::InfoSpace *m_monInfoSpacePull;
	      std::list<std::string> m_monItemListPull;

	      xdata::InfoSpace *m_monInfoSpacePush;
	      xdata::Float m_avgValue;
	      std::list<std::string> m_monItemListPush;
	      toolbox::BSem m_applock;


	      xdata::UnsignedInteger32 m_currentnib;
	      xdata::UnsignedInteger32 m_currentls;
	      xdata::UnsignedInteger32 m_currentrun;
	      xdata::UnsignedInteger32 m_currentfill;
	      unsigned int m_nibperls;
	      unsigned int m_lsperrun;
	      unsigned int m_activechannels[16];
	      unsigned int m_maskdata[2];
	      unsigned int m_nactivechannels[2];
	      xdata::Table m_montable;
	      std::map<std::string,xdata::Serializable*> m_monStatus;
	      std::map<std::string,xdata::Serializable*> m_monRunStatus;
	      void defineMonTable();
	      void initializeMonTable();

	    };
	}
}

#endif
