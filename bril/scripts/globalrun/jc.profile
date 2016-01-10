<?xml version='1.0'?>
<!-- Order of specification will determine the sequence of installation. all modules are loaded prior instantiation of plugins -->
<xp:Profile xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xmlns:soapenc="http://schemas.xmlsoap.org/soap/encoding/" xmlns:xp="http://xdaq.web.cern.ch/xdaq/xsd/2005/XMLProfile-11">
 <xp:Application heartbeat="false" class="executive::Application" id="0" group="profile" service="executive" network="local" logpolicy="inherit">
  <properties xmlns="urn:xdaq-application:Executive" xsi:type="soapenc:Struct">
   <logUrl xsi:type="xsd:string">console</logUrl>
   <logLevel xsi:type="xsd:string">INFO</logLevel>
  </properties>
 </xp:Application>
 <xp:Module>${XDAQ_ROOT}/lib/libb2innub.so</xp:Module>
 <xp:Module>${XDAQ_ROOT}/lib/libexecutive.so</xp:Module>
 <xp:Application class="pt::utcp::Application" id="20" instance="0" network="local" heartbeat="false" logpolicy="inherit">
  <properties xmlns="urn:xdaq-application:pt::utcp::Application" xsi:type="soapenc:Struct">
   <maxBlockSize xsi:type="xsd:unsignedInt">65536</maxBlockSize>
   <committedPoolSize xsi:type="xsd:double">0xA00000</committedPoolSize>
   <ioQueueSize xsi:type="xsd:unsignedInt">1024</ioQueueSize>
   <autoConnect xsi:type="xsd:boolean">false</autoConnect>
   <protocol xsi:type="xsd:string">utcp</protocol>
   <maxReceiveBuffers xsi:type="xsd:unsignedInt">4</maxReceiveBuffers>
  </properties>
 </xp:Application>
 <xp:Module>/opt/xdaq/lib/libtcpla.so</xp:Module>
 <xp:Module>/opt/xdaq/lib/libptutcp.so</xp:Module>
 <xp:Application heartbeat="false" class="pt::http::PeerTransportHTTP" id="1" group="profile" network="local" logpolicy="inherit">
   <properties xmlns="urn:xdaq-application:pt::http::PeerTransportHTTP" xsi:type="soapenc:Struct">
   <documentRoot xsi:type="xsd:string">${XDAQ_DOCUMENT_ROOT}</documentRoot>
   <aliasName xsi:type="xsd:string">/directory</aliasName>
   <aliasPath xsi:type="xsd:string">${XDAQ_SETUP_ROOT}/${XDAQ_ZONE}</aliasPath>
   <httpHeaderFields xsi:type="soapenc:Array" soapenc:arrayType="xsd:ur-type[3]">
    <item xsi:type="soapenc:Struct" soapenc:position="[0]">
        <name xsi:type="xsd:string">Access-Control-Allow-Origin</name>
        <value xsi:type="xsd:string">*</value>
    </item>
    <item xsi:type="soapenc:Struct" soapenc:position="[1]">
        <name xsi:type="xsd:string">Access-Control-Allow-Methods</name>
        <value xsi:type="xsd:string">POST, GET, OPTIONS</value>
    </item>
    <item xsi:type="soapenc:Struct" soapenc:position="[2]">
        <name xsi:type="xsd:string">Access-Control-Allow-Headers</name>
        <value xsi:type="xsd:string">x-requested-with</value>
    </item>
   </httpHeaderFields>
   <expiresByType xsi:type="soapenc:Array" soapenc:arrayType="xsd:ur-type[5]">
   <item xsi:type="soapenc:Struct" soapenc:position="[0]">
    <name xsi:type="xsd:string">image/png</name>
    <value xsi:type="xsd:string">PT4300H</value>
   </item>
   <item xsi:type="soapenc:Struct" soapenc:position="[1]">
    <name xsi:type="xsd:string">image/jpg</name>
    <value xsi:type="xsd:string">PT4300H</value>
   </item>
   <item xsi:type="soapenc:Struct" soapenc:position="[2]">
    <name xsi:type="xsd:string">image/gif</name>
    <value xsi:type="xsd:string">PT4300H</value>
   </item>
   <item xsi:type="soapenc:Struct" soapenc:position="[3]">
    <name xsi:type="xsd:string">application/x-shockwave-flash</name>
    <value xsi:type="xsd:string">PT120H</value>
   </item>
   <item xsi:type="soapenc:Struct" soapenc:position="[4]">
    <name xsi:type="xsd:string">application/font-woff</name>
    <value xsi:type="xsd:string">PT8600H</value>
   </item>
   </expiresByType>
   </properties>
 </xp:Application>
 <xp:Module>${XDAQ_ROOT}/lib/libpthttp.so</xp:Module>
 <xp:Application heartbeat="false" class="pt::fifo::PeerTransportFifo" id="8" group="profile" network="local" logpolicy="inherit" />
 <xp:Module>${XDAQ_ROOT}/lib/libptfifo.so</xp:Module>
 <!-- HyperDAQ -->
 <xp:Application heartbeat="false" class="hyperdaq::Application" id="3" group="profile" service="hyperdaq" network="local" logpolicy="inherit"/>
 <xp:Module>${XDAQ_ROOT}/lib/libhyperdaq.so</xp:Module>
 <!-- XMem probe-->
 <xp:Application class="xmem::probe::Application" id="7" network="local" logpolicy="inherit"/>
 <xp:Module>${XDAQ_ROOT}/lib/libxmemprobe.so</xp:Module>
 <xp:Endpoint protocol="utcp" service="b2in" hostname="localhost" port="1911" maxport="1970" autoscan="true" network="localnet" smode="select" rmode="select" nonblock="true" sndTimeout="2000" rcvTimeout="2000"/>
 <!-- jobcontrol requires the installation of jobcontrol -->
 <xp:Application heartbeat="false" class="jobcontrol::Application" id="10" service="jobcontrol" group="init2" network="local" logpolicy="inherit"/>
 <xp:Module>${XDAQ_ROOT}/lib/libxdaq2rc.so</xp:Module>
 <xp:Module>${XDAQ_ROOT}/lib/libjobcontrol.so</xp:Module>
</xp:Profile>
