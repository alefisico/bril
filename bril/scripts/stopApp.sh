#!/bin/bash
usage="Usage: ./stopApp.sh port \n Stop application by port number. \n"
p=$1
if [ -z $1 ];then
    printf "$usage"
    exit
fi
/sbin/fuser -v -k -n tcp $@