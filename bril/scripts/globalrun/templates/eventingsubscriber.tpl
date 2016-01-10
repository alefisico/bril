 <xc:Application class="eventing::core::Subscriber" id="%(id)s" instance="%(instance)s" network="utcp1" group="b2in" service="eventing-subscriber" bus="%(bus)s" logpolicy="inherit">
 <properties xmlns="urn:xdaq-application:eventing::core::Subscriber" xsi:type="soapenc:Struct"> %(eventings)s </properties>
</xc:Application>
 <xc:Module>${XDAQ_ROOT}/lib/libb2inutils.so</xc:Module>
 <xc:Module>${XDAQ_ROOT}/lib/libeventingcore.so</xc:Module>
