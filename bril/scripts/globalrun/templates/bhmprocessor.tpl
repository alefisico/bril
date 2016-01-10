<xc:Application class="bril::bhmprocessor::Application" id="%(id)s" instance="%(instance)s" network="local" group="bhmprocessor" service="processor" logpolicy="inherit">
<properties xmlns="urn:xdaq-application:bril::bhmprocessor::Application" xsi:type="soapenc:Struct">
 %(eventinginput)s
 %(eventingoutput)s	
</properties> 
</xc:Application>
<xc:Module>${XDAQ_ROOT}/lib/libeventingapi.so</xc:Module>
<xc:Module>${XDAQ_ROOT}/lib/libbrilbhmprocessor.so</xc:Module>
