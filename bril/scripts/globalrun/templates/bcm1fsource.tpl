<xc:Application class="bril::bcm1fsource::Application" id="%(id)s" instance="%(instance)s" network="local" group="brilbcm1f" service="source" logpolicy="inherit">
 <properties xmlns="urn:xdaq-application:bril::bcm1fsource::Application" xsi:type="soapenc:Struct">
   <simSource xsi:type="xsd:boolean">%(sim)s</simSource>
   <signalTopic xsi:type="xsd:string">NB1</signalTopic>
   <bus xsi:type="xsd:string">%(bus)s</bus>  
   <topics xsi:type="xsd:string">%(topics)s</topics>
 </properties>
</xc:Application>
<xc:Module>${XDAQ_ROOT}/lib/libeventingapi.so</xc:Module>
<xc:Module>${XDAQ_ROOT}/lib/libbrilbcm1fsource.so</xc:Module>
