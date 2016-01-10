BUILD_HOME=$(shell pwd)/../..

include $(XDAQ_ROOT)/config/mfAutoconf.rules
include $(XDAQ_ROOT)/config/mfDefs.$(XDAQ_OS)

Project=lumi
Package=lumiDIPPublisher

Sources = LumiApplication.cc LumiDIPPublisher.cc

DynamicLibrary = lumiDIPPublisher

IncludeDirs = \
	$(ROOTSYS)/include \
	$(XDAQ_ROOT)/include

DependentLibraryDirs = \
	$(ROOTSYS)/lib \
	$(XDAQ_ROOT)/lib

UserDynamicLinkFlags = \
	-pthread -lboost_system

UserCFlags = -O2 -Wall
UserCCFlags = -O2 -Wall

include $(XDAQ_ROOT)/config/Makefile.rules

cleanall:
	rm -fr lib src/linux
