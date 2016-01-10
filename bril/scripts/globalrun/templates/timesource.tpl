<xc:Application class="bril::timesource::Application" id="%(id)s" instance="%(instance)s" network="local" group="brilcentral" service="clock" logpolicy="inherit">
 <properties xmlns="urn:xdaq-application:bril::timesource::Application" xsi:type="soapenc:Struct">
 <simSource xsi:type="xsd:boolean">%(sim)s</simSource>
 <swInitRun xsi:type="xsd:unsignedInt">123450</swInitRun>
 <swLSPerRun xsi:type="xsd:unsignedInt">2</swLSPerRun>
 <swNRuns xsi:type="xsd:unsignedInt">2</swNRuns>
 <swRunPerFill xsi:type="xsd:unsignedInt">1</swRunPerFill>
 %(swNBTimers)s         
 </properties>
 </xc:Application> 
 <xc:Module>${XDAQ_ROOT}/lib/libeventingapi.so</xc:Module>
 <xc:Module>${XDAQ_ROOT}/lib/libbriltimesource.so</xc:Module>
