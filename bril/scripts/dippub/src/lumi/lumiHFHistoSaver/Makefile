BUILD_HOME=$(shell pwd)/../..

include $(XDAQ_ROOT)/config/mfAutoconf.rules
include $(XDAQ_ROOT)/config/mfDefs.$(XDAQ_OS)

Project=lumi
Package=lumiHFHistoSaver

Sources = common.cc LumiApplication.cc LumiHFHistoSaver.cc

DynamicLibrary = lumiHFHistoSaver

IncludeDirs = \
	$(ROOTSYS)/include \
	$(XDAQ_ROOT)/include

DependentLibraryDirs = \
	$(ROOTSYS)/lib \
	$(XDAQ_ROOT)/lib

UserDynamicLinkFlags = \
	-pthread -lboost_system -lCore -lRIO -lMathCore -lCint -lNet -lTree \
	-lMatrix -lHist -lGraf -lGpad

UserCFlags = -O2 -Wall
UserCCFlags = -O2 -Wall

include $(XDAQ_ROOT)/config/Makefile.rules

cleanall:
	rm -fr lib src/linux
