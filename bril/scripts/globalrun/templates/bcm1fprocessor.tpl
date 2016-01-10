<xc:Application class="bril::bcm1fprocessor::Application" id="%(id)s" instance="%(instance)s" network="local" group="bcm1fprocessor" service="processor" logpolicy="inherit">
 <properties xmlns="urn:xdaq-application:bril::bcm1fprocessor::Application" xsi:type="soapenc:Struct">
 %(eventinginput)s
 %(eventingoutput)s
 </properties>
</xc:Application>
<xc:Module>${XDAQ_ROOT}/lib/libeventingapi.so</xc:Module>
<xc:Module>${XDAQ_ROOT}/lib/libbrilbcm1fprocessor.so</xc:Module>
