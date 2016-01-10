export DIPNS=cmsdimns1.cern.ch,cmsdimns2.cern.ch
export DIM_DNS_NODE=cmsdimns1.cern.ch,cmsdimns2.cern.ch
export DIM_DNS_PORT=2506

/opt/xdaq/bin/xdaq.exe -h srv-s2d16-29-01 -p 50100 -c /brilpro/plt/daq/bril/scripts/PLTAnalyzer.xml

#/opt/xdaq/bin/xdaq.exe -h srv-s2d16-29-01 -p 50100 -c /nfshome0/kthapa/daq/bril/scripts/PLTAnalyzer.xml >& /scratch/dip.log &

#/opt/xdaq/bin/xdaq.exe -h srv-s2d16-29-01 -p 1927 -c /nfshome0/kthapa/diptest/test/PublisherTest.xml -z bril &
