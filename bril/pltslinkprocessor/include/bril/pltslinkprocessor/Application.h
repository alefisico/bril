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

#ifndef _bril_pltslinkprocessor_Application_h_
#define _bril_pltslinkprocessor_Application_h_

#include <string>
#include <fstream>
#include <iostream>

#include "xdaq/Application.h"

#include "xgi/Method.h"
#include "xgi/Output.h"
#include "xgi/exception/Exception.h"
#include "xgi/framework/UIManager.h"
#include "eventing/api/Member.h"

#include "root/TH2F.h"
#include "root/TFile.h"

#include "xdata/ActionListener.h"
#include "xdata/InfoSpace.h"
#include "xdata/String.h"
#include "xdata/Vector.h"
#include "xdata/UnsignedInteger32.h"

#include "toolbox/mem/Reference.h"
#include "b2in/nub/exception/Exception.h"

#include "toolbox/task/TimerListener.h"

#include "toolbox/mem/MemoryPoolFactory.h"
#include "toolbox/mem/Reference.h"
#include "toolbox/squeue.h"

namespace toolbox{
  namespace task{
    class WorkLoop;
  }
}
namespace bril
{
  namespace pltslinkprocessor
  {
    class Application : public xdaq::Application,public xgi::framework::UIManager,public eventing::api::Member, public xdata::ActionListener,public toolbox::ActionListener, public toolbox::task::TimerListener
    {
      public:
	
	XDAQ_INSTANTIATOR();
	
	Application (xdaq::ApplicationStub* s) throw (xdaq::exception::Exception);
	~Application ();
	
	void Default (xgi::Input * in, xgi::Output * out) throw (xgi::exception::Exception);
	
	void onMessage (toolbox::mem::Reference * ref, xdata::Properties & plist) throw (b2in::nub::exception::Exception);
	
	virtual void actionPerformed(xdata::Event& e);
	
	virtual void actionPerformed(toolbox::Event& e);
	virtual void timeExpired(toolbox::task::TimerEvent& e);
        void automask(float totdata[16]);
    private:
	bool publishing(toolbox::task::WorkLoop* wl);
	void do_publish(const std::string& busname,const std::string& topicname,toolbox::mem::Reference* bufRef);
	void subscribeall();
	void do_zmqclient();
	void make_plots();	
    protected:
	

	void do_starttimer();
	void do_stoptimer();
	toolbox::mem::MemoryPoolFactory *m_poolFactory;
	xdaq::ApplicationDescriptor *m_appDescriptor;
	std::string m_classname;
	std::string m_timername;
	xdata::UnsignedInteger32 m_instanceid;
	toolbox::mem::Pool* m_memPool;
	
	xdata::UnsignedInteger32 m_currentfill;
	xdata::UnsignedInteger32 m_currentnib;
	xdata::UnsignedInteger32 m_prevnib;	
	xdata::UnsignedInteger32 m_currentls;
	xdata::UnsignedInteger32 m_currentrun;
	xdata::UnsignedInteger32 m_prevls;
        uint32_t m_nactivechannels;

	uint32_t m_aggchannels;

	uint32_t m_aggdata[16][3564]; //agg hist per channel 
        float m_aggzero[16][3564]; //agg zeroes per channel
        float m_logzero[16][3564]; //mu value per channel
        float m_totdata[16];
	float m_automasks[16];
	float m_rawbxlumi[3564];  //bxrawlumi averaged over 16 channels
	float m_bxlumi[3564];     //bxrawlumi*calib	
        float m_rawbxzero[3564];  
        float m_bxzero[3564];
        float m_bxlogzero[3564];
	float m_avgrawlumi;       //rawbxlumi sum over bx
	float m_avglumi;          //bxlumi sum over bx
	float m_avgrawzero;
	float m_avgzero;
        unsigned int fill;
        unsigned int run;
        unsigned int ls;
	unsigned int dummy;
	unsigned int ientry;

	TH2F *m_OccupancyPlots[48];
	//TFile *DQMFile;

	//eventinginput, eventingoutput configuration  
	xdata::Vector<xdata::Properties> m_datasources;
	xdata::Vector<xdata::Properties> m_outputtopics;
	xdata::Vector<xdata::Properties> m_tcdssources;
        xdata::String m_signalTopic;
        xdata::String m_bus;
	std::map<std::string, std::set<std::string> > m_in_busTotopics;
	typedef std::multimap< std::string, std::string > TopicStore;
	typedef std::multimap< std::string, std::string >::iterator TopicStoreIt;
	TopicStore m_out_topicTobuses;
	//count how many outgoing buses not ready
	std::set<std::string> m_unreadybuses;
	
	// outgoing queue
	typedef std::map<std::string, toolbox::squeue< toolbox::mem::Reference* >* > QueueStore;
	typedef std::map<std::string, toolbox::squeue< toolbox::mem::Reference* >* >::iterator QueueStoreIt;
	QueueStore m_topicoutqueues;

	std::map<std::string,std::string> m_outtopicdicts;
	toolbox::task::WorkLoop* m_publishing;

      };
  }
}

#endif
