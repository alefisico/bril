# $Id$

#########################################################################
# XDAQ Components for Distributed Data Acquisition                      #
# Copyright (C) 2000-2011, CERN.			                			#
# All rights reserved.                                                  #
# Authors: L.Orsini, A. Forrest, A.Petrucci								#
#                                                                       #
# For the licensing terms see LICENSE.		                       		#
# For the list of contributors see CREDITS.   			        		#
#########################################################################

##
#
# This is the TriDAS/daq/lumi/PLT_datprocessor Package Makefile
#
##
BUILD_HOME:=$(shell pwd)/../../..

include $(XDAQ_ROOT)/config/mfAutoconf.rules
include $(XDAQ_ROOT)/config/mfDefs.$(XDAQ_OS)

include $(XDAQ_ROOT)/config/mfDefs.extern_coretools
include $(XDAQ_ROOT)/config/mfDefs.coretools
include $(XDAQ_ROOT)/config/mfDefs.extern_powerpack
include $(XDAQ_ROOT)/config/mfDefs.powerpack
include $(XDAQ_ROOT)/config/mfDefs.bril_worksuite

#
# Package to be built
#
Project=daq
Package=bril/pltslinksource

#
# Source files
#
Sources=$(wildcard src/common/*.cc)

#
# Include directories
#
IncludeDirs = \
	$(XI2O_UTILS_INCLUDE_PREFIX) \
	$(XERCES_INCLUDE_PREFIX) \
	$(LOG4CPLUS_INCLUDE_PREFIX) \
	$(CGICC_INCLUDE_PREFIX) \
	$(XCEPT_INCLUDE_PREFIX) \
	$(CONFIG_INCLUDE_PREFIX) \
	$(TOOLBOX_INCLUDE_PREFIX) \
	$(PT_INCLUDE_PREFIX) \
	$(XDAQ_INCLUDE_PREFIX) \
	$(XDATA_INCLUDE_PREFIX) \
	$(XOAP_INCLUDE_PREFIX) \
	$(XGI_INCLUDE_PREFIX) \
	$(I2O_INCLUDE_PREFIX) \
	$(XI2O_INCLUDE_PREFIX) \
	$(B2IN_NUB_INCLUDE_PREFIX) \
	$(INTERFACE_BRIL_INCLUDE_PREFIX) \
	/usr/include

LibraryDirs = 

UserSourcePath =

UserCFlags =
UserCCFlags =
UserDynamicLinkFlags =
UserStaticLinkFlags =
UserExecutableLinkFlags =

# These libraries can be platform specific and
# potentially need conditional processing
#
Libraries =
ExternalObjects = 

#
# Compile the source files and create a shared library
#

DependentLibraryDirs = /usr/lib64
DependentLibraries = zmq

DynamicLibrary=brilpltslinksource
StaticLibrary=

TestLibraries=
TestExecutables=

include $(XDAQ_ROOT)/config/Makefile.rules
include $(XDAQ_ROOT)/config/mfRPM.rules
