#!/bin/bash
#
# Example of running the LumiHFReadout.
#
# NOTE: this script makes assumptions about the structure of directories and
# thus will not work with a different layout.
#

# stop on first error
set -e

# determine project's top-level directory
pushd "`dirname $0`/.." >/dev/null
base=`pwd`
popd >/dev/null

# determine absolute path to liblumiHFReadout.so
modpath=`find "${base}/src/lumi/lumiHFReadout" -name liblumiHFReadout.so`
[ -z "$modpath" ] && \
   { echo "FATAL: liblumiHFReadout.so not found (did you forgot to compile it first?)" 1>&2; exit 1; }

# temporary path to xml with xdaq's configuration
# NOTE: this file will not be automatically removed after termination of this script
xmlpath="/tmp/test_liblumiHFReadout_${UID}.xml"

source "${base}/test/env.sh"

# make xml
cat >$xmlpath <<EOF
<?xml version='1.0'?>
<xc:Partition xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
              xmlns:soapenc="http://schemas.xmlsoap.org/soap/encoding/"
              xmlns:xc="http://xdaq.web.cern.ch/xdaq/xsd/2004/XMLConfiguration-30">
   <xc:Context url="http://localhost:1900">
      <xc:Endpoint protocol="utcp" service="b2in" hostname="localhost" port="2000"
                   network="utcp1" sndTimeout="2000" rcvTimeout="2000" rmode="select"
                   affinity="RCV:P,SND:W,DSR:W,DSS:W" singleThread="true" publish="true"/>

      <xc:Application class="pt::utcp::Application" id="101" instance="0" network="local">
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

      <xc:Application class="eventing::core::Publisher" id="102" instance="0" network="utcp1"
                      group="b2in" service="eventing-publisher" bus="hf">
         <properties xmlns="urn:xdaq-application:eventing::core::Publisher" xsi:type="soapenc:Struct">
            <eventings xsi:type="soapenc:Array" soapenc:arrayType="xsd:ur-type[1]">
               <item xsi:type="xsd:string" soapenc:position="[0]">utcp://localhost:1999</item>
            </eventings>
         </properties>
      </xc:Application>

      <xc:Module>${XDAQ_ROOT}/lib/libb2inutils.so</xc:Module>
      <xc:Module>${XDAQ_ROOT}/lib/libeventingcore.so</xc:Module>

      <xc:Application class="lumi::LumiHFReadout" id="103" instance="0" network="local"
                      bus="hf" service="LumiHFReadout"/>
      <xc:Module>${XDAQ_ROOT}/lib/libeventingapi.so</xc:Module>
      <xc:Module>${modpath}</xc:Module>
   </xc:Context>
</xc:Partition>
EOF

# start LumiHFReadout
export LUMI_CONFIG_HFREADOUT="${base}/etc/lumihfreadout.conf"
exec xdaq.exe -h localhost -p 1900 -c $xmlpath
