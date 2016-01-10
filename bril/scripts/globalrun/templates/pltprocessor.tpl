<xc:Application class="bril::pltprocessor::Application" id="%(id)s" instance="%(instance)s" network="local" group="pltprocessor" service="processor" logpolicy="inherit">
 <properties xmlns="urn:xdaq-application:bril::pltprocessor::Application" xsi:type="soapenc:Struct">
 %(eventinginput)s
 %(eventingoutput)s
</properties>
</xc:Application>
<xc:Module>${XDAQ_ROOT}/lib/libeventingapi.so</xc:Module>
<xc:Module>${XDAQ_ROOT}/lib/libbrilpltprocessor.so</xc:Module>
