prepend() {
  if [ ".${LD_LIBRARY_PATH}" == "." ] ; then
     export LD_LIBRARY_PATH=$1
  else
     export LD_LIBRARY_PATH=$1:${LD_LIBRARY_PATH}
  fi
}

SOURCE="${BASH_SOURCE[0]}"
MYDIR="$(cd "$( dirname "$SOURCE" )" && pwd )"
ARCH="x86_64_slc6"

export XDAQ_ROOT=/opt/xdaq
export CACTUS_ROOT=/opt/cactus
export XDAQ_DOCUMENT_ROOT=/opt/xdaq/htdocs
#export DIM_DNS_NODE=cmsdimns1.cern.ch,cmsdimns2.cern.ch
export DIPNS=cmsdimns1.cern.ch,cmsdimns2.cern.ch
export DIM_DNS_PORT=2506
export LOG4CXX_CONFIGURATION=/opt/xdaq/etc/log4cxx.properties
export BRILDAQ_DOCUMENT_ROOT=/opt/xdaq/htdocs/bril
export LOCALDAQ_ROOT=${MYDIR}/../../../${ARCH}
export XDAQ_SETUP_ROOT=/opt/xdaq/share
export XDAQ_ZONE=bril

#order matters here
prepend "${XDAQ_ROOT}/lib:${XDAQ_ROOT}/lib64:${CACTUS_ROOT}/lib:/usr/local/lib64"
prepend "${LOCALDAQ_ROOT}/lib"

echo "LD_LIBRARY_PATH is " ${LD_LIBRARY_PATH}
echo "PATH is " ${PATH}

