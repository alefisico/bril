<xc:Application heartbeat="true" class="b2in::eventing::Application" id="%(id)s" network="utcp1" group="b2in" service="b2in-eventing" publish="true" logpolicy="inherit">
   <properties xmlns="urn:xdaq-application:b2in::eventing::Application" xsi:type="soapenc:Struct"> </properties>
</xc:Application>
<xc:Module>${XDAQ_ROOT}/lib/libb2inutils.so</xc:Module>
<xc:Module>${XDAQ_ROOT}/lib/libb2ineventing.so</xc:Module>
