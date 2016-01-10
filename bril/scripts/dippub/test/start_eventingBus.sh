#!/bin/bash
#
# Example of running the eventing application.
#

# stop on first error
set -e

# temporary path to xml with xdaq's configuration
# NOTE: this file will not be automatically removed after termination of this script
xmlpath="/tmp/test_eventing_${UID}.xml"

source "`dirname $0`/env.sh"

# make xml
cat >$xmlpath <<EOF
<?xml version='1.0'?>
<xc:Partition xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
              xmlns:soapenc="http://schemas.xmlsoap.org/soap/encoding/"
              xmlns:xc="http://xdaq.web.cern.ch/xdaq/xsd/2004/XMLConfiguration-30">
   <xc:Context url="http://localhost:1901">
      <xc:Endpoint protocol="utcp" service="b2in" hostname="localhost" port="1999"
                   network="utcp1" sndTimeout="2000" rcvTimeout="2000" rmode="select"
                   affinity="RCV:P,SND:W,DSR:W,DSS:W" singleThread="true" publish="true"/>

      <xc:Application class="pt::utcp::Application" id="201" instance="0" network="local">
         <properties xmlns="urn:xdaq-application:pt::utcp::Application" xsi:type="soapenc:Struct">
            <maxClients xsi:type="xsd:unsignedInt">10</maxClients>
            <autoConnect xsi:type="xsd:boolean">false</autoConnect>
            <ioQueueSize xsi:type="xsd:unsignedInt">65536</ioQueueSize>
            <eventQueueSize xsi:type="xsd:unsignedInt">65536</eventQueueSize>
            <maxReceiveBuffers xsi:type="xsd:unsignedInt">4</maxReceiveBuffers>
            <maxBlockSize xsi:type="xsd:unsignedInt">65536</maxBlockSize>
            <committedPoolSize xsi:type="xsd:double">0xFFFFFFF</committedPoolSize>
         </properties>
      </xc:Application>
      <xc:Module>${XDAQ_ROOT}/lib/libtcpla.so</xc:Module>
      <xc:Module>${XDAQ_ROOT}/lib/libptutcp.so</xc:Module>

      <xc:Application heartbeat="true" class="b2in::eventing::Application" id="202" network="utcp1"
                      group="eventingtest" service="b2in-eventing" publish="true"/>
      <xc:Module>${XDAQ_ROOT}/lib/libb2inutils.so</xc:Module>
      <xc:Module>${XDAQ_ROOT}/lib/libb2ineventing.so</xc:Module>

      <xc:Application class="xmem::probe::Application" id="203" instance="0" network="local"/>
      <xc:Module>${XDAQ_ROOT}/lib/libxmemprobe.so</xc:Module>

   </xc:Context>
</xc:Partition>
EOF

# start eventing & auxiliary stuff
exec xdaq.exe -h localhost -p 1901 -c $xmlpath
