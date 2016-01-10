BUILD_HOME=$(shell pwd)/../..

include $(XDAQ_ROOT)/config/mfAutoconf.rules
include $(XDAQ_ROOT)/config/mfDefs.$(XDAQ_OS)

Project=lumi
Package=lumiHFReadout

Sources = \
	common.cc \
	LumiApplication.cc LumiConfiguration.cc LumiQueue.cc \
	LumiHFReadout.cc \
	LumiHFReadout_ThreadReadout.cc \
	LumiHFReadout_ThreadCalculator.cc

DynamicLibrary = lumiHFReadout

IncludeDirs = \
	$(XDAQ_ROOT)/include \
	$(CACTUS_ROOT)/include

DependentLibraryDirs = \
	$(XDAQ_ROOT)/lib \
	$(CACTUS_ROOT)/lib 

UserDynamicLinkFlags = -pthread -lboost_system -lcactus_uhal_uhal -lhcalUHTR

UserCFlags = -O2 -Wall
UserCCFlags = -O2 -Wall

include $(XDAQ_ROOT)/config/Makefile.rules

cleanall:
	rm -fr lib src/linux
