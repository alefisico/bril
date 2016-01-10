!#/bin/bash

export DIPNS=cmsdimns1.cern.ch,cmsdimns2.cern.ch
export DIM_DNS_NODE=cmsdimns1.cern.ch,cmsdimns2.cern.ch
export DIM_DNS_PORT=2506
/opt/xdaq/bin/xdaq.exe -h vmepc-s2d16-07-01 -p 1903 -c /cmsnfsbrilpro/brilpro/plt/daq/bril/scripts/pltprocessor.xml  &
