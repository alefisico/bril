<xc:Application class="bril::bhmsource::Application" id="%(id)s" instance="%(instance)s" network="local" group="bhmsource" service="source" logpolicy="inherit">
 <properties xmlns="urn:xdaq-application:bril::bhmsource::Application" xsi:type="soapenc:Struct">
   <simSource xsi:type="xsd:boolean">%(sim)s</simSource>
   <signalTopic xsi:type="xsd:string">NB1</signalTopic>
   <lutFile xsi:type="xsd:string">lut.txt</lutFile>
   <bus xsi:type="xsd:string">%(bus)s</bus>  
   <topics xsi:type="xsd:string">%(topics)s</topics>
 </properties>
</xc:Application>
<xc:Module>${XDAQ_ROOT}/lib/libeventingapi.so</xc:Module>
<xc:Module>${XDAQ_ROOT}/lib/libbrilbhmsource.so</xc:Module>
