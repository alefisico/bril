BUILD_HOME=$(shell pwd)/../..

include $(XDAQ_ROOT)/config/mfAutoconf.rules
include $(XDAQ_ROOT)/config/mfDefs.$(XDAQ_OS)

Project=lumi
Package=lumiHFLumiSaver

Sources = common.cc LumiApplication.cc LumiHFLumiSaver.cc

DynamicLibrary = lumiHFLumiSaver

IncludeDirs = \
	$(ROOTSYS)/include\
	$(XDAQ_ROOT)/includes

DependentLibraryDirs = \
	$(ROOTSYS)/lib \
	$(XDAQ_ROOT)/lib

DependentLibraries = \
        dip \
        :liblog4cxx.so.10 	

UserDynamicLinkFlags = \
	-pthread -lboost_system -lCore -lRIO -lMathCore -lCint -lNet -lTree \
	-lMatrix -lHist -lGraf -lGpad

UserCFlags = -O2 -Wall
UserCCFlags = -O2 -Wall

include $(XDAQ_ROOT)/config/Makefile.rules

cleanall:
	rm -fr lib src/linux
