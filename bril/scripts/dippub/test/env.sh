#!/bin/bash
#
# Environment requirements for the HF lumi.
#

export ROOTSYS=/opt/root/hcal-root-5.28.00f-gcc44
export XDAQ_ROOT=/opt/xdaq
export CACTUS_ROOT=/opt/cactus
export HCAL_XDAQ_ROOT=/opt/xdaq
export XDAQ_DOCUMENT_ROOT=/opt/xdaq/htdocs

export PATH="${ROOTSYS}/bin:${XDAQ_ROOT}/bin:${PATH}"
export LD_LIBRARY_PATH="${ROOTSYS}/lib:${XDAQ_ROOT}/lib:${CACTUS_ROOT}/lib:${LD_LIBRARY_PATH}"
