BUILD_HOME=$(shell pwd)/../..

include $(XDAQ_ROOT)/config/mfAutoconf.rules
include $(XDAQ_ROOT)/config/mfDefs.$(XDAQ_OS)

Project=lumi
Package=lumiHFLumiDipPublisher

Sources = common.cc LumiApplication.cc LumiHFLumiDipPublisher.cc

DynamicLibrary = lumiHFLumiDipPublisher

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
	-pthread -lboost_system

UserCFlags = -O2 -Wall
UserCCFlags = -O2 -Wall

include $(XDAQ_ROOT)/config/Makefile.rules

cleanall:
	rm -fr lib src/linux
