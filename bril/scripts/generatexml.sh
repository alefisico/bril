#!/bin/bash
usage="Usage: ./generatexml.sh packagedir"
pkgname=$1
if [ -z $1 ];then
    echo "$usage"
    exit
fi
conffile="${pkgname}/template.param"
templatelist=(`find ${pkgname} -name "*.template"`)
tmpfile="tmp.xml"
ARCH="x86_64_slc6"

SOURCE="${BASH_SOURCE[0]}"
MYDIR="$(cd "$( dirname "$SOURCE" )" && pwd )"
localdaqroot=$(echo "${MYDIR}/../../../${ARCH}" | sed 's/\//\\\//g')

for t in ${templatelist[@]}; do
  contexts=()
  echo "sources:"
  echo "    $conffile,$t"
  bn=`basename $t`
  filename=`echo "$bn" | sed 's/\.template/\.xml/g'`
  cp $t $tmpfile
  echo "generated:"
  echo "    `pwd`/$filename"
  while IFS="=" read -r name value 
  do 
    if [[ ! $name =~ [^[:space:]] ]]; then
      continue
    fi
    echo ${name} | grep % 1>/dev/null
    if [ ! $? -eq 0 ]; then
      continue
    fi
    v="${value//\"/}"
    ev=`eval echo $v`

    if [ $name == "%localdaqroot%" ]; then
       if [[ -z "$ev" ]]; then  
           sed "s/%localdaqroot%/$localdaqroot/g" <$tmpfile >$filename
       else
           sed "s/%localdaqroot%/$ev/g" <$tmpfile >$filename
       fi
    cp $filename $tmpfile
    fi
   
    sed "s/$name/$ev/g" <$tmpfile >$filename 
    result=$(diff $tmpfile $filename )

    if [[ ! -z $result ]]; then
       if [ -n "$(echo "$name"|grep -E "^%(.*)contextport%$")" ]; then
          contexts+=("$ev") 
       fi
    fi
    cp $filename $tmpfile
  done < "$conffile"
  rm $tmpfile
  #echo "contexts ${contexts[@]}"
  echo "command per context:"
  for i in "${!contexts[@]}";do
     contextport="${contexts[i]}"
     if [ -n "$(echo "$filename"|grep -E "inzone")" ];then
       echo "   ${XDAQ_ROOT}/bin/xdaq.exe -p $contextport -c `pwd`/$filename -z bril"
     else
       echo "   ${XDAQ_ROOT}/bin/xdaq.exe -p $contextport -c `pwd`/$filename "
     fi
     unset contexts[i]  
  done
  #echo "${contexts[@]}"
  echo
done
