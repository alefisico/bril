#!/bin/bash
#
# Sends Stop/Start commands to an xDAQ application.
#

app="$1"
port="$2"
cmd="$3"

# check input / print usage info
[ -z "$app" -o -z "$port" -o -z "$cmd" ] && \
    { echo "Usage: $0 LumiHFReadout|LumiHFHistoSaver|LumiHFLumiSaver <port> Start|Stop"; exit 1; }

curl -H "SOAPAction: urn:xdaq-application:class=lumi::${app},instance=0" \
    "http://localhost:${port}" -d \
"<SOAP-ENV:Envelope
   SOAP-ENV:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\"
   xmlns:SOAP-ENV=\"http://schemas.xmlsoap.org/soap/envelope/\"
   xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"
   xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\"
   xmlns:SOAP-ENC=\"http://schemas.xmlsoap.org/soap/encoding/\">
    <SOAP-ENV:Header/>
    <SOAP-ENV:Body>
        <xdaq:${cmd} xmlns:xdaq=\"urn:xdaq-soap:3.0\"/>
    </SOAP-ENV:Body>
</SOAP-ENV:Envelope>"

# add a newline symbol
echo
