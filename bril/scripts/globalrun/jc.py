#!/usr/bin/env python
"""Usage:
   jc.py startAll <filename> [-z=<zone>] [-u=<user>] [--with-backstore] [--dryrun] [--debug]
   jc.py startXdaqExe <host> [-p=<jcport>] [-e=<execport>] [-z=<zone>] [-u=<user>] [--path=<xdaqpath>] [--envs=<xdaqenvs>] [--dryrun] [--debug]
   jc.py executeCommand <host> --command=<command> [-p=<jcport>] [-u=<user>] [--dryrun] [--debug]
   jc.py configExec <host> [-e=<execport>] [-x=<configxml>] [--dryrun] [--debug]
   jc.py getJobStatus <host> [-p=<jcport>] [-e=<execport>|-j=<jid>] [--dryrun] [--debug]
   jc.py killExec <host> [-p=<jcport>] [-e=<execport>|-j=<jid>] [--dryrun] [--debug]
   jc.py killAll <host> [-p=<jcport>] [--dryrun] [--debug]
   jc.py (-h | --help)

Options:
   -h --help              Show this screen.
   -p=<jcport>            JC daemon port [default: 9999].
   -e=<execport>          Executive port [default: 40000].
   -z=<zone>              Zone.
   -j=<jid>               Job id.
   -u=<user>              Remote run as user. 
   -x=<configxml>         Configure xml string or file [default: '']
   --path=<xdaqpath>      Remote xdaq path [default: /opt/xdaq/bin/xdaq.exe].
   --command=<command>    Remote full command to execute. 
   --envs=<xdaqenvs>      Remote runtime environment.
   --with-backstore       Store generated xml into files. 
   --dryrun               Run without sending soap. 
   --debug                Print debug messages.
    
"""

from docopt import docopt
import sys,yaml
import urllib2,socket
#import xml.etree.ElementTree as ET
from cStringIO import StringIO
import api
from collections import OrderedDict

def ordered_load(stream, Loader=yaml.Loader, object_pairs_hook=OrderedDict):
    class OrderedLoader(Loader):
        pass
    def construct_mapping(loader, node):
        loader.flatten_mapping(node)
        return object_pairs_hook(loader.construct_pairs(node))
    OrderedLoader.add_constructor(
        yaml.resolver.BaseResolver.DEFAULT_MAPPING_TAG, construct_mapping)
    return yaml.load(stream, OrderedLoader)

import logging
log = logging.getLogger('jc')
ch = logging.StreamHandler()
ch.setFormatter(logging.Formatter('%(name)-2s: %(levelname)-4s %(message)s'))
log.addHandler(ch)

def start_fm(fmname,fmconf,zone=None,user=None,withbackstore=False,dryrun=False):
    import subprocess
    if withbackstore:
        bakfilebasename = 'jc_%s'%(fmname)     
    log.debug('Creating executives in %s'%(fmname) )
    for idx,e in enumerate(fmconf):#loop over executives
        hostname = e['hostname']
        port = e['port']
        endpoint = e['endpoint']
        apps = e['apps']
        #user = e['unixUser'] #jc command takes -u argument rather than read unixUser config
        environmentStr = e['environmentString']
        evs = [x.split('=') for x in environmentStr.split(' ')]
        environmentStr = ' '.join(['='.join([e[0],'"'+e[1]+'"']) for e in evs])        
        xmlresult = api.makecontextxml(hostname,port,endpoint,apps,zone=zone,withxmlhead=False)
        
        if withbackstore:    
            xmlfilename = bakfilebasename+'_%d.xml'%(idx)
            log.debug(' write xml config to backstore: %s'%xmlfilename)
            with open(xmlfilename,'w') as xmlf:
                xmlf.write(xmlresult)
                
        cmmd = ['./jc.py','startXdaqExe',hostname,'-e',str(port),'--envs',environmentStr]
        if zone is not None:
            cmmd += ['-z',zone]
        if user is not None:
            cmmd += ['-u',user]
        if dryrun:
            cmmd += ['--dryrun']
        p = subprocess.Popen(cmmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=subprocess.PIPE)
        out, err = p.communicate()
        log.info(out)
        log.info('Executive %d started'%idx)
        cmmd = ['./jc.py','configExec',hostname,'-e',str(port),'-x',xmlresult]
        if dryrun:
            cmmd += ['--dryrun']
        p = subprocess.Popen(cmmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=subprocess.PIPE)
        out, err = p.communicate()
        log.info(out)
        log.info('Executive %d configured'%idx)
        sleep(5)
        
if __name__=='__main__':   
    args = docopt(__doc__)
    log.setLevel(logging.INFO)
    if args['--debug'] : log.setLevel(logging.DEBUG)
    log.debug(str(args))

    if args['startAll']:
        from time import sleep
        filename = args['<filename>']
        with open(filename) as f:
           conf = ordered_load(f, yaml.SafeLoader)
        allfms = [k for k in conf.keys() if k not in ['host','rcmsparams']]
        for fm in allfms:
            fmconf = conf[fm]
            log.info('Starting %s'%(fm))            
            start_fm(fm,fmconf,zone=args['-z'],user=args['-u'],withbackstore=args['--with-backstore'],dryrun=args['--dryrun'])
            log.info('Done')
            sleep(0.5)
        sys.exit(0)

    if args['startXdaqExe']:
       l = api.startXdaqExe(args['<host>'],args['-p'])
       l.execport = args['-e']
       if args['--envs']:
           l.envs = args['--envs']
       if args['-z']:
           l.zone = args['-z']
       if not args['-u']:
           import getpass
           l.user=getpass.getuser()
       else:
           l.user=args['-u'] 
       if args['--dryrun']:
           l.printparameters(['cmmd','httpheaders','desturl','user','jid','msg'])
       else:
           print l.post()
       sys.exit(0)
    if args['configExec']:
       l = api.Configure(args['<host>'],args['-e'])
       l.execport = args['-e']       
       xmlcontent = args['-x']
       try: 
           f = open(xmlcontent)
           l.xmlcontent = f.read().rstrip('\n')
           f.close()
       except IOError as e:
           l.xmlcontent = xmlcontent
       if args['--dryrun']:
           l.printparameters(['httpheaders','responseTag','desturl','msg'])
       else:
           print l.post()
       sys.exit(0)
    if args['getJobStatus']:
       l = api.getJobStatus(args['<host>'],args['-p'])
       if args['-j']:
          l.jid = args['-j']
       elif args['-e']:  
          l.execport = args['-e']
       if args['--dryrun']:
           l.printparameters(['httpheaders','responseTag','desturl','execport','jid','msg'])
       else:
           print l.post()
       sys.exit(0)
    if args['killExec']:
       l = api.killExec(args['<host>'],args['-p'])
       if args['-j']:
           l.jid = args['-j']
       elif args['-e']:  
          l.execport = args['-e']
       if args['--dryrun']:
           l.printparameters(['httpheaders','responseTag','desturl','execport','jid','msg'])
       else:
           print l.post()
       sys.exit(0)
    if args['killAll']:
       l = api.killAll(args['<host>'],args['-p'])
       if args['--dryrun']:
           l.printparameters(['httpheaders','responseTag','desturl','msg'])
       else:
           print l.post()
       sys.exit(0)        
    if args['executeCommand']:
       command = args['--command']
       cmmd,argv = command.split(' ',1)
       l = api.executeCommand(args['<host>'],args['-p'],argv=argv)
       l.execpath = cmmd
       if args['-u']: l.user = args['-u']
       if args['--dryrun']:
           l.printparameters(['httpheaders','responseTag','user','msg'])
       else:
           print l.post()
       sys.exit(0)        
   
#
#
#
'''
single command examples:
./jc.py startXdaqExe pczhen.cern.ch  -p 9999 -e 40010 -u zhen
./jc.py configExec pczhen.cern.ch  -e 40010 -x memprobe.xml --debug
./jc.py getJobStatus pczhen.cern.ch -p 9999 -e 40010 
./jc.py killExec  pczhen.cern.ch  -e 40010
'''    

'''
process control command examples:
./jc.py startAll globalsim_v2.yaml 
'''    


