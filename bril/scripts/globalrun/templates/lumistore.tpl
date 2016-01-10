<xc:Application class="bril::lumistore::Application" id="%(id)s" instance="%(instance)s" network="local" group="brilcentral" service="store" logpolicy="inherit">
<properties xmlns="urn:xdaq-application:bril::lumistore::Application" xsi:type="soapenc:Struct">
%(datasources)s     
<maxstalesec xsi:type="xsd:unsignedInt">%(maxstalesec)s</maxstalesec>
<checkagesec xsi:type="xsd:unsignedInt">%(checkagesec)s</checkagesec>
<maxsizeMB xsi:type="xsd:unsignedInt">%(maxsizemb)s</maxsizeMB>
<nrowperwbuf xsi:type="xsd:unsignedInt">%(nrowperwbuf)s</nrowperwbuf>
<filepath xsi:type="xsd:string">%(filepath)s</filepath>
<fileformat xsi:type="xsd:string">%(fileformat)s</fileformat>
</properties>
</xc:Application>
<xc:Module>${XDAQ_ROOT}/lib/libbrillumistore.so</xc:Module>
