#!/bin/bash
#
# Starts all xDAQ applications in one shot.
# Log files will be created in current directory.
#

# determine this script's directory
pushd "`dirname $0`" >/dev/null
base=`pwd`
popd >/dev/null

$base/start_eventingBus.sh >eventing.log 2>&1 &

$base/start_lumiHFHistoSaver.sh >saver_histo.log 2>&1 &

#$base/start_lumiHFLumiSaver.sh >saver_lumi.log 2>&1 &

$base/start_lumiHFReadout.sh >readout.log 2>&1 &
