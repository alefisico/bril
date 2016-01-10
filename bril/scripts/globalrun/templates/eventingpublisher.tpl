 <xc:Application class="eventing::core::Publisher" id="%(id)s" instance="%(instance)s" network="utcp1" group="b2in" service="eventing-publisher" bus="%(bus)s" logpolicy="inherit">
 <properties xmlns="urn:xdaq-application:eventing::core::Publisher" xsi:type="soapenc:Struct">
 %(eventings)s
</properties>
</xc:Application>
 <xc:Module>${XDAQ_ROOT}/lib/libb2inutils.so</xc:Module>
 <xc:Module>${XDAQ_ROOT}/lib/libeventingcore.so</xc:Module>
