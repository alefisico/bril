#!/bin/bash
#
# Gracefully terminates all xDAQ applications in one shot.
#

# determine this script's directory
pushd "`dirname $0`" >/dev/null
base=`pwd`
popd >/dev/null

# perform running => stopped stansitions
"$base/send_soap.sh" LumiHFReadout    1900 Stop
"$base/send_soap.sh" LumiHFLumiSaver  1903 Stop
"$base/send_soap.sh" LumiHFHistoSaver 1902 Stop

# kill xDAQ processes
for port in 1900 1903 1902 1901; do
    pid="`ps aux | grep $port | grep xdaq | awk '{ print $2; }'`"
    kill "$pid"
    sleep 1
    kill -9 "$pid" >/dev/null 2>&1
done
