templates
-- applicaiton configuration template files

jc.py
-- to run multiple contexts on multiple hosts without rcms
    ./jc.py startAll <configfile> [args]
-- to send single command to jc
    ./jc.py startXdaqExe <host> [args]
    ./jc.py configExec <host> [args]
    ./jc.py getJobStatus <host> [args]
    ./jc.py killExec <host> [args]

-- examples:
    ./jc.py startAll bhmlocalsim_v0.yaml --with-backstore
    ./jc.py killAll pczhen.cern.ch
    ./jc.py getJobStatus pczhen.cern.ch -p 9999 -e 30010
    ./jc.py killExec pczhen.cern.ch -p 9999 -e 30010
       
fm.py
-- export JAVA_HOME=blah 
-- export PATH=${JAVA_HOME}/bin:$PATH
-- ln -s /afs/cern.ch/cms/lumi/DB/DUCK.properties DUCK.properties
-- mkdir -p out
-- ./fm.py <configfile> -z bril -o out

-- populate duck for each xml
   ls out/*.xml
   java -jar /afs/cern.ch/cms/lumi/DB/duck.jar out/bcm1fFM.xml -c "blah";java -jar /afs/cern.ch/cms/lumi/DB/duck.jar out/bhmFM.xml -c "blah";java -jar /afs/cern.ch/cms/lumi/DB/duck.jar out/brilclockFM.xml -c "blah";java -jar /afs/cern.ch/cms/lumi/DB/duck.jar out/brildaqFM.xml -c "blah";java -jar /afs/cern.ch/cms/lumi/DB/duck.jar out/pltFM.xml -c "blah";

-- javaws http://cmsrc-srv:9000/rs_manager/1_13_1/manager.jnlp

ssh me@cmsrc-lumidev
