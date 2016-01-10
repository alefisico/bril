<xc:Application class="pt::utcp::Application" id="%(id)s" instance="0" network="local">
<properties xmlns="urn:xdaq-application:pt::utcp::Application" xsi:type="soapenc:Struct">
  <maxClients xsi:type="xsd:unsignedInt">50</maxClients>
  <autoConnect xsi:type="xsd:boolean">false</autoConnect>
  <ioQueueSize xsi:type="xsd:unsignedInt">65536</ioQueueSize>
  <eventQueueSize xsi:type="xsd:unsignedInt">65536</eventQueueSize>
  <maxReceiveBuffers xsi:type="xsd:unsignedInt">266</maxReceiveBuffers>
  <maxBlockSize xsi:type="xsd:unsignedInt">%(maxblocksize)s</maxBlockSize>  
  <committedPoolSize xsi:type="xsd:double">%(committedpoolsize)s</committedPoolSize>
</properties>
</xc:Application>
<xc:Module>${XDAQ_ROOT}/lib/libtcpla.so</xc:Module>
<xc:Module>${XDAQ_ROOT}/lib/libptutcp.so</xc:Module>
