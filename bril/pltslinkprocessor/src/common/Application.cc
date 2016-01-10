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

#include <sstream>
#include <fstream>
#include <iostream>
#include <string>
#include "cgicc/CgiDefs.h"
#include "cgicc/Cgicc.h"
#include "cgicc/HTTPHTMLHeader.h"
#include "cgicc/HTMLClasses.h"
#include "xgi/framework/Method.h"

#include "bril/pltslinkprocessor/Application.h"
#include "bril/pltslinkprocessor/exception/Exception.h"
#include "interface/bril/PLTSlinkTopics.hh"

#include "eventing/api/exception/Exception.h"

#include "xdata/Properties.h"

#include "toolbox/mem/HeapAllocator.h"
#include "toolbox/mem/MemoryPoolFactory.h"
#include "xcept/tools.h"
#include "xdata/InfoSpaceFactory.h"
#include "toolbox/TimeVal.h"
#include "toolbox/task/TimerFactory.h"

#include "root/TCanvas.h"
#include "root/TStyle.h"
#include "root/TLine.h"
#include "root/TROOT.h"

#include "b2in/nub/Method.h"
//#include "toolbox/regex.h"
#include "toolbox/task/WorkLoop.h"
#include "toolbox/task/WorkLoopFactory.h"
#include "bril/pltslinkprocessor/zmq.hpp"

XDAQ_INSTANTIATOR_IMPL (bril::pltslinkprocessor::Application)

using namespace interface::bril;

bril::pltslinkprocessor::Application::Application (xdaq::ApplicationStub* s) throw (xdaq::exception::Exception)
	: xdaq::Application(s),xgi::framework::UIManager(this),eventing::api::Member(this)
{
	xgi::framework::deferredbind(this,this,&bril::pltslinkprocessor::Application::Default, "Default");
	b2in::nub::bind(this, &bril::pltslinkprocessor::Application::onMessage);
	m_poolFactory = toolbox::mem::getMemoryPoolFactory();
	m_appDescriptor = getApplicationDescriptor();
	m_classname = m_appDescriptor->getClassName();
	m_instanceid = m_appDescriptor->getInstance();
	
	std::string memPoolName = m_classname + m_instanceid.toString() + std::string("_memPool");
	toolbox::mem::HeapAllocator* allocator = new toolbox::mem::HeapAllocator();
	toolbox::net::URN urn("toolbox-mem-pool",memPoolName);
	m_memPool = m_poolFactory->createPool(urn, allocator);
	m_timername="lumiPLT_SlinkprocessorTimer";
	//	do_starttimer();
	for(int ihist = 0; ihist<16; ihist++){
	  for(int iroc = 0; iroc<3; iroc++){
	  char histname[80];
	  sprintf(histname, "ChannelOccupancy_CHA%i_ROC%i", ihist, iroc);
	  m_OccupancyPlots[((ihist*3)+iroc)] = new TH2F(histname, histname, 100, 0, 100, 100, 0, 100);
	  }
	}

	try{
	  getApplicationInfoSpace()->fireItemAvailable("eventinginput",&m_datasources);
	  getApplicationInfoSpace()->fireItemAvailable("eventingoutput",&m_outputtopics);
          getApplicationInfoSpace()->fireItemAvailable("signalTopic",&m_signalTopic);
          getApplicationInfoSpace()->fireItemAvailable("bus",&m_bus);
	  getApplicationInfoSpace()->addListener(this, "urn:xdaq-event:setDefaultValues");
	  m_publishing = toolbox::task::getWorkLoopFactory()->getWorkLoop(m_appDescriptor->getURN()+"_publishing","waiting");
	}
	catch(xcept::Exception & e){
	  XCEPT_RETHROW(xdaq::exception::Exception, "Failed to add Listener", e);
	}

 }

bril::pltslinkprocessor::Application::~Application ()
{
  QueueStoreIt it;
  for(it=m_topicoutqueues.begin();it!=m_topicoutqueues.end();++it){
    delete it->second;
  }
  m_topicoutqueues.clear();
}

void bril::pltslinkprocessor::Application::actionPerformed(xdata::Event& e){
  std::stringstream msg;
  if(e.type() == "urn:xdaq-event:setDefaultValues"){

    std::string timername("zmqstarter");
    try{
      toolbox::TimeVal zmqStart(2,0);
      toolbox::task::Timer * timer = toolbox::task::getTimerFactory()->createTimer(timername);
      timer->schedule(this, zmqStart, 0, timername);
    }catch(xdata::exception::Exception& e){
      std::stringstream msg;
      msg << "failed to start timer" << timername;
      LOG4CPLUS_ERROR(getApplicationLogger(),msg.str()+stdformat_exception_history(e));
      XCEPT_RETHROW(bril::pltslinkprocessor::exception::Exception,msg.str(),e);
    }
    
    size_t nsources = m_datasources.elements();
    try{
      for(size_t i=0;i<nsources;++i){
	xdata::Properties* p = dynamic_cast<xdata::Properties*>(m_datasources.elementAt(i));
	xdata::String databus;
	xdata::String topicsStr;
	if(p){
	  databus = p->getProperty("bus");  	 
	  topicsStr = p->getProperty("topics");
	  std::set<std::string> topics = toolbox::parseTokenSet(topicsStr.value_,","); 	  
	  m_in_busTotopics.insert(std::make_pair(databus.value_,topics));
	}
      }
      subscribeall();
      size_t ntopics = m_outputtopics.elements();
      for(size_t i=0;i<ntopics;++i){
	xdata::Properties* p = dynamic_cast<xdata::Properties*>(m_outputtopics.elementAt(i));
	if(p){
	  xdata::String topicname = p->getProperty("topic");
	  xdata::String outputbusStr = p->getProperty("buses");
	  std::set<std::string> outputbuses = toolbox::parseTokenSet(outputbusStr.value_,",");
	  for(std::set<std::string>::iterator it=outputbuses.begin(); it!=outputbuses.end(); ++it){
	    m_out_topicTobuses.insert(std::make_pair(topicname.value_,*it));
	  }
	  m_topicoutqueues.insert(std::make_pair(topicname.value_,new toolbox::squeue<toolbox::mem::Reference*>));
	  m_unreadybuses.insert(outputbuses.begin(),outputbuses.end());
	}
      }
      for(std::set<std::string>::iterator it=m_unreadybuses.begin(); it!=m_unreadybuses.end(); ++it){
	this->getEventingBus(*it).addActionListener(this);
      }
	this->getEventingBus(m_bus.value_).subscribe(m_signalTopic.value_);
    }catch(xdata::exception::Exception& e){
      msg<<"Failed to parse application property";
      LOG4CPLUS_ERROR(getApplicationLogger(), msg);
      XCEPT_RETHROW(bril::pltslinkprocessor::exception::Exception, msg.str(), e);
    }  
  }
}

void bril::pltslinkprocessor::Application::actionPerformed(toolbox::Event& e){
  if(e.type() == "eventing::api::BusReadyToPublish" ){
    std::string busname = (static_cast<eventing::api::Bus*>(e.originator()))->getBusName();
    std::stringstream msg;
    msg<< "event Bus '" << busname << "' is ready to publish";
    m_unreadybuses.erase(busname);
    if(m_unreadybuses.size()!=0) return; //wait until all buses are ready
    try{    
      toolbox::task::ActionSignature* as_publishing = toolbox::task::bind(this,&bril::pltslinkprocessor::Application::publishing,"publishing");
      m_publishing->activate();
      m_publishing->submit(as_publishing);
    }catch(toolbox::task::exception::Exception& e){
      msg<<"Failed to start publishing workloop "<<stdformat_exception_history(e);
      LOG4CPLUS_ERROR(getApplicationLogger(),msg.str());
      XCEPT_RETHROW(bril::pltslinkprocessor::exception::Exception,msg.str(),e);	
    }    
  }
}

void bril::pltslinkprocessor::Application::subscribeall(){
  for(std::map<std::string, std::set<std::string> >::iterator bit=m_in_busTotopics.begin(); bit!=m_in_busTotopics.end(); ++bit ){
    std::string busname = bit->first; 
    for(std::set<std::string>::iterator topicit=bit->second.begin(); topicit!= bit->second.end(); ++topicit){       
      LOG4CPLUS_INFO(getApplicationLogger(),"subscribing "+busname+":"+*topicit); 
      try{
	this->getEventingBus(busname).subscribe(*topicit);
      }catch(eventing::api::exception::Exception& e){
	LOG4CPLUS_ERROR(getApplicationLogger(),"failed to subscribe, remove topic "+stdformat_exception_history(e));
	m_in_busTotopics[busname].erase(*topicit);
      }
    }
  }
}


/*
 * This is the default web page for this XDAQ application.
 */
void bril::pltslinkprocessor::Application::Default (xgi::Input * in, xgi::Output * out) throw (xgi::exception::Exception)
{
  
  *out << busesToHTML();
  std::string appurl = getApplicationContext()->getContextDescriptor()->getURL()+"/"+m_appDescriptor->getURN();
  *out << appurl;
  *out << "We are publishing simulated data on frequency : "<< m_signalTopic.toString();   
  *out << cgicc::br();
  
  *out << cgicc::table().set("class","xdaq-table-vertical");
  *out << cgicc::caption("input/output");
  *out << cgicc::tbody();
  
  *out << cgicc::tr();
  *out << cgicc::th("in_topics");
  *out << cgicc::td(pltslinkhistT::topicname());
  *out << cgicc::tr();
  
  *out << cgicc::tr();
  *out << cgicc::th("out_topics");
  *out << cgicc::td( pltslinkhistT::topicname());
  *out << cgicc::tr();
  
  *out << cgicc::tr();
  *out << cgicc::th("signalTopic");
  *out << cgicc::td( m_signalTopic.toString());
  *out << cgicc::tr();

  *out << cgicc::tbody(); 
  *out << cgicc::table();
  *out << cgicc::br() << std::endl;
  
  *out << cgicc::img().set("src","/OccupancyPlot_CHA15_ROC2.pdf").set("alt", "Channel 15 ROC 2").set("height", "100").set("width","100") << std::endl;

}

/*
 * This is the place where we receive messages.
 */
void bril::pltslinkprocessor::Application::onMessage (toolbox::mem::Reference * ref, xdata::Properties & plist) throw (b2in::nub::exception::Exception)
{
  // The eventing can send 'control' messages, so we need to check we are receiving a user message.
  std::stringstream msg;
  std::string action = plist.getProperty("urn:b2in-eventing:action");
  if (action == "notify"){
    std::string topic = plist.getProperty("urn:b2in-eventing:topic");
    LOG4CPLUS_DEBUG(getApplicationLogger(), "Received data from "+topic);
    std::string v = plist.getProperty("DATA_VERSION");
    if(v.empty()){
      msg<<"Received data message without version header, do not process.";
      LOG4CPLUS_ERROR(getApplicationLogger(),msg.str());
      if(ref!=0){
	ref->release();
	ref=0;
      }
      return;
    }
    if(v!=DATA_VERSION){
      msg.str("");
      msg<<"Mismatched data version, exiting";
      LOG4CPLUS_ERROR(getApplicationLogger(),msg.str());
      if(ref!=0){
	ref->release();
	ref=0;
      }
      return;
    }

    //First handle the topic sent if the number of active channels has changed.
    /*
    std::cout<<"received topic "<<topic<<std::endl;
    if(topic == m_signalTopic.toString()){
      interface::bril::DatumHead* inheader = (interface::bril::DatumHead*)(ref->getDataLocation()); 
      fill = inheader->fillnum;
      run = inheader->runnum;
      ls = inheader->lsnum;
      //unsigned int channelId = inheader->getChannelID();//count from 1
      unsigned int nb = inheader->nbnum;
      msg<<"process on fill "<<fill<<" run "<<run<<" ls " << ls << " nibble "<< nb;
      std::cout <<"process on fill "<<fill<<" run "<<run<<" ls " << ls << " nibble "<< nb << std::endl;
    }*/

    std::vector<std::string> substrings;
    if (topic == pltslinkhistT::topicname()){    
      DatumHead* inheader = (DatumHead*)(ref->getDataLocation());
      LOG4CPLUS_DEBUG(getApplicationLogger(),msg.str());
      pltslinkhistT* dataptr=(pltslinkhistT*)ref->getDataLocation();      
      //unsigned int tot = 0;
      //      for(int iiii = 0; iiii<3654; iiii++){
      //	tot+=dataptr->payload()[iiii];}
      LOG4CPLUS_DEBUG(getApplicationLogger(),msg.str());
      uint32_t NHits = dataptr->payload()[0];
      std::cout << "Total number of pixel hits is " << NHits << std::endl;

      for(int ihits = 0; ihits<NHits; ihits++){
	//std::cout << "ihits+1501 " << dataptr->payload()[ihits+1501] << std::endl;
	//std::cout << "ihits+1001 " << (dataptr->payload()[ihits+1001]) << std::endl;
	//std::cout << "dataptr " << ((dataptr->payload()[ihits+1001])+dataptr->payload()[ihits+1501]) << std::endl;
        m_OccupancyPlots[((dataptr->payload()[ihits+1001])+dataptr->payload()[ihits+1501])]->Fill(dataptr->payload()[ihits+501], dataptr->payload()[ihits+1]);
      }
    }
  }
  ientry++;
  if(ref != 0){
    ref->release();
  }  
}

void bril::pltslinkprocessor::Application::do_publish(const std::string& busname,const std::string& topicname,toolbox::mem::Reference* bufRef){
  std::stringstream msg;
  try{
    xdata::Properties plist;
    plist.setProperty("DATA_VERSION",DATA_VERSION);
    std::map<std::string,std::string>::iterator it=m_outtopicdicts.find(topicname);
    if( it!=m_outtopicdicts.end() ){
      plist.setProperty("PAYLOAD_DICT",it->second);
    }
    msg << "publish to "<<busname<<" , "<<topicname; 
    this->getEventingBus(busname).publish(topicname,bufRef,plist);
  }catch(xcept::Exception& e){
    msg<<"Failed to publish "<<topicname<<" to "<<busname;    
    LOG4CPLUS_ERROR(getApplicationLogger(),stdformat_exception_history(e));
    if(bufRef){
      bufRef->release();
      bufRef = 0;
    }
    XCEPT_DECLARE_NESTED(bril::pltslinkprocessor::exception::Exception,myerrorobj,msg.str(),e);
    this->notifyQualified("fatal",myerrorobj);
  }  
}
 

bool bril::pltslinkprocessor::Application::publishing(toolbox::task::WorkLoop* wl){
  QueueStoreIt it;
  for(it=m_topicoutqueues.begin();it!=m_topicoutqueues.end();++it){
    if(it->second->empty()) continue;
    std::string topicname = it->first;
    	    LOG4CPLUS_INFO(getApplicationLogger(),"Publishing "+topicname);
    toolbox::mem::Reference* data = it->second->pop();
    std::pair<TopicStoreIt,TopicStoreIt > ret = m_out_topicTobuses.equal_range(topicname);
    for(TopicStoreIt topicit = ret.first; topicit!=ret.second;++topicit){
      if(data) do_publish(topicit->second,topicname,data->duplicate());    
    }
    if(data) data->release();
  }
  return true;
}


void bril::pltslinkprocessor::Application::do_stoptimer(){
  toolbox::task::Timer* timer = toolbox::task::TimerFactory::getInstance()->getTimer(m_timername);
  if( timer->isActive() ){
    timer->stop();
  }
}

void bril::pltslinkprocessor::Application::do_starttimer(){
  if(!toolbox::task::TimerFactory::getInstance()->hasTimer(m_timername)){
    (void) toolbox::task::TimerFactory::getInstance()->createTimer(m_timername);
  }
  toolbox::task::Timer* timer = toolbox::task::TimerFactory::getInstance()->getTimer(m_timername);
  toolbox::TimeInterval interval(20,0);
  try{
    do_stoptimer();
    toolbox::TimeVal start = toolbox::TimeVal::gettimeofday();
    timer->start();
    timer->scheduleAtFixedRate( start, this, interval, 0, m_timername);
  }catch (toolbox::task::exception::InvalidListener& e){
    LOG4CPLUS_FATAL (this->getApplicationLogger(), xcept::stdformat_exception_history(e));
  }catch (toolbox::task::exception::InvalidSubmission& e){
    LOG4CPLUS_FATAL (this->getApplicationLogger(), xcept::stdformat_exception_history(e));
  }catch (toolbox::task::exception::NotActive& e){
    LOG4CPLUS_FATAL (this->getApplicationLogger(), xcept::stdformat_exception_history(e));
  }catch (toolbox::exception::Exception& e){
    LOG4CPLUS_FATAL (this->getApplicationLogger(), xcept::stdformat_exception_history(e));
  }
}


void bril::pltslinkprocessor::Application::timeExpired(toolbox::task::TimerEvent& e){
  
  std::cout << "doing zmq" << std::endl;
  do_zmqclient();

}

void bril::pltslinkprocessor::Application::do_zmqclient(){

  zmq::context_t zmq_context(1);
  zmq::socket_t zmq_socket(zmq_context, ZMQ_SUB);

  int zmq_port = 5556;
  
  char zmq_address[1000];
  
  sprintf(zmq_address, "tcp://vmepc-s2d16-07-01.cms:%i", zmq_port);

  zmq_socket.connect(zmq_address);
  zmq_socket.setsockopt(ZMQ_SUBSCRIBE, 0, 0);

  uint32_t EventHisto[3573];
  uint32_t tcds_info[4];
  uint32_t channel;	
  int n_messages = 0;
  while(1){
    zmq::message_t zmq_EventHisto(3573*sizeof(uint32_t));
    zmq_socket.recv(&zmq_EventHisto);
   
    memcpy(EventHisto, zmq_EventHisto.data(), 3573*sizeof(uint32_t));
    memcpy(&channel, (uint32_t*)EventHisto+2, sizeof(uint32_t));
    memcpy(&tcds_info, (uint32_t*)EventHisto+3, 4*sizeof(uint32_t));

    if(channel == 0){
      fill = tcds_info[3];
      run = tcds_info[2]; 
      ls = tcds_info[1]; 
    std::cout << "zmq message received with fill " << tcds_info[3] << " run " << tcds_info[2] << " LS " << tcds_info[1] << "NB" << tcds_info[0] << std::endl;
    if(tcds_info[0]==64) make_plots();}
  }


}


void bril::pltslinkprocessor::Application::make_plots(){
  std::cout << "Making plots" << std::endl;
  TCanvas *c = new TCanvas("Occupancy", "Occupancy");

  char outFolderROOT[80] = "/nfshome0/algomez/PLTPlots/";
  char filenameROOT[80];
  sprintf(filenameROOT, "run%i_ls%i_streamDQMPLT_eventing-bus.root", run, ls );
  TFile DQMFile(strcat(outFolderROOT,filenameROOT), "RECREATE");
  char filedirROOT[80];
  sprintf(filedirROOT, "Run summary/%i/BRIL", run );
  TDirectory *cdtof = DQMFile.mkdir( filedirROOT );
  cdtof->cd(); 

  char outFolderDAT[80] = "/nfshome0/algomez/PLTPlots/";
  char filenameDAT[80];
  sprintf(filenameDAT, "run%i_ls%d_streamDQMPLT_eventing-bus.dat", run, ls );
  FILE *fileDAT;
  fileDAT = fopen( strcat(outFolderDAT,filenameDAT), "w");
  for(int ihist = 0; ihist < 16; ihist++){
    for(int iroc = 0; iroc < 3; iroc++){
      m_OccupancyPlots[((ihist*3)+iroc)]->Draw("colz");
      m_OccupancyPlots[((ihist*3)+iroc)]->Write();

	  fprintf( fileDAT , "{ \"name\"   : \"OccupancyPlots\",\n");
	  fprintf( fileDAT , "  \"type\"   : \"2D\",\n");
	  fprintf( fileDAT , "  \"titles\" : \"[OccupancyPlot_CHA%i_ROC%i, Row (ROC %i), Column (ROC %i)]\",\n", ihist, iroc, iroc, iroc );  // should be run
	  fprintf( fileDAT , "  \"nbins\"  : \"[100, 100]\",\n");
	  fprintf( fileDAT , "  \"data\"   : \"[");
	double w;
	for (int biny=0; biny<= m_OccupancyPlots[((ihist*3)+iroc)]->GetNbinsY()+1; biny++) {
		for (int binx=0; binx<= m_OccupancyPlots[((ihist*3)+iroc)]->GetNbinsX()+1; binx++) {
		        w = m_OccupancyPlots[((ihist*3)+iroc)]->GetBinContent(binx,biny);
            		fprintf( fileDAT , "%d %d %g, ", binx,biny,w );
         	}
		dummy++; 
      	}
	  fprintf( fileDAT , "],\n");
	  fprintf( fileDAT , "  \"xrange\"   : \"[0,100]\",\n");
	  fprintf( fileDAT , "  \"yrange\"   : \"[0,100]\",\n");
	  fprintf( fileDAT , "  \"integral\" : \"[0 ]\"\n}\n");


      c->SaveAs(TString::Format("/nfshome0/algomez/PLTPlots/OccupancyPlot_CHA%i_ROC%i.pdf", ihist, iroc));
      //m_OccupancyPlots[((ihist*3)+iroc)]->Reset("ICES");
    }
  }
  char outFolder[80] = "/nfshome0/algomez/PLTPlots/";
  char filenameJSN[80];
  sprintf(filenameJSN, "run%i_ls%i_streamDQMPLT_eventing-bus.jsn", run, ls );
  FILE *fileJSN;
  fileJSN = fopen( strcat(outFolder, filenameJSN), "w");
  fprintf( fileJSN, "{\"data\": [1 %i, 1 %i, 0, \"%s\", 0, 0, 0, 0, 0, ""]}", ientry, ientry, filenameROOT);
  fclose(fileJSN);

  DQMFile.Write();
  DQMFile.Close();
  fclose(fileDAT);
  std::cout << "Output complete. DQM files " << filenameJSN << ", " << filenameDAT << " and " << filenameROOT << " created." << std::endl;

}

