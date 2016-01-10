#!/bin/bash
#
# Example of running the LumiDIPPublisher.
#

# stop on first error
set -e

# determine absolute path to liblumiDIPPublisher.so
modpath="`pwd`/lib/linux/x86_64_slc6/liblumiDIPPublisher.so"

# temporary path to xml with xdaq's configuration
# NOTE: this file will not be automatically removed after termination of this script
xmlpath="/tmp/test_liblumiDIPPublisher_${UID}.xml"

# make xml
cat >$xmlpath <<EOF
<?xml version='1.0'?>
<xc:Partition xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
              xmlns:soapenc="http://schemas.xmlsoap.org/soap/encoding/"
              xmlns:xc="http://xdaq.web.cern.ch/xdaq/xsd/2004/XMLConfiguration-30">
   <xc:Context url="http://localhost:1905">
      <xc:Endpoint protocol="utcp" service="b2in" hostname="localhost" port="2005"
                   network="utcp1" sndTimeout="2000" rcvTimeout="2000" rmode="select"
                   affinity="RCV:P,SND:W,DSR:W,DSS:W" singleThread="true" publish="true"/>

      <xc:Application class="pt::utcp::Application" id="401" instance="0" network="local">
         <properties xmlns="urn:xdaq-application:pt::utcp::Application" xsi:type="soapenc:Struct">
            <maxClients xsi:type="xsd:unsignedInt">10</maxClients>
            <autoConnect xsi:type="xsd:boolean">false</autoConnect>
            <ioQueueSize xsi:type="xsd:unsignedInt">65536</ioQueueSize>
            <eventQueueSize xsi:type="xsd:unsignedInt">65536</eventQueueSize>
            <maxReceiveBuffers xsi:type="xsd:unsignedInt">2</maxReceiveBuffers>
            <maxBlockSize xsi:type="xsd:unsignedInt">65536</maxBlockSize>
            <committedPoolSize xsi:type="xsd:double">0xFFFFFFF</committedPoolSize>
         </properties>
      </xc:Application>
      <xc:Module>${XDAQ_ROOT}/lib/libtcpla.so</xc:Module>
      <xc:Module>${XDAQ_ROOT}/lib/libptutcp.so</xc:Module>

      <xc:Application class="eventing::core::Subscriber" id="402" instance="0" network="utcp1"
                      group="b2in" service="eventing-subscriber" bus="hf">
         <properties xmlns="urn:xdaq-application:eventing::core::Subscriber" xsi:type="soapenc:Struct">
            <eventings xsi:type="soapenc:Array" soapenc:arrayType="xsd:ur-type[1]">
               <item xsi:type="xsd:string" soapenc:position="[0]">utcp://localhost:1999</item>
            </eventings>
         </properties>
      </xc:Application>

      <xc:Module>${XDAQ_ROOT}/lib/libb2inutils.so</xc:Module>
      <xc:Module>${XDAQ_ROOT}/lib/libeventingcore.so</xc:Module>

      <xc:Application class="lumi::LumiDIPPublisher" id="254" instance="0" network="local"
                      bus="hf" service="LumiDIPPublisher"/>
      <xc:Module>${XDAQ_ROOT}/lib/libeventingapi.so</xc:Module>
      <xc:Module>${modpath}</xc:Module>
   </xc:Context>
</xc:Partition>
EOF

# start
exec xdaq.exe -h localhost -p 1905 -c $xmlpath
