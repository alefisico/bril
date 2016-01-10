#!/usr/bin/env python
"""Usage:
   fm.py <filename> [-o=<outputdir>] [-z=<zone>] [--plainxml] [--debug]
   fm.py (-h | --help)

Options:
   -h --help              Show this screen.
   -o=<outputdir>         Output directory [default: .].
   -z=<zone>              Zone [default: None].
   --plainxml             Show embedded config file as plain XML. [default: False]
   --debug                Print debug messages.


"""

import sys,yaml,os
from docopt import docopt
import api
import logging
log = logging.getLogger('fm')
ch = logging.StreamHandler()
ch.setFormatter(logging.Formatter('%(name)-2s: %(levelname)-4s %(message)s'))
log.addHandler(ch)


def makercmsconfigurations(allfms,rcmsparams,outputdir='.',zone=None,plainxml=False):
    '''
    make fmname.xml for each fm
    '''
    xmlhead = [api._xmlhead]
    foot = ['</Configuration>']
    for fmname,executives in allfms.iteritems():
        content = ['<Configuration xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" user="%(rcmsuser)s" path="bril/%(runtag)s/%(fmname)s">'%({'rcmsuser':rcmsparams['rcmsuser'],'runtag':rcmsparams['runtag'],'fmname':fmname})]
        listofexecutive = makelistofexecutive(executives,zone=zone,plainxml=plainxml)
        execs = [ makefm(fmname,rcmsparams['host'],listofexecutive) ]       
        f = open(os.path.join(outputdir,fmname+'.xml'),'w') 
        f.write( '\n'.join(xmlhead+content+execs+foot) )
        f.close() 

def makefm(fmname,rcmshost,listofexecutive):
    '''
    Make FM with xdaqexecutive and jobcontrol
    '''
    fm_head = ['<FunctionManager name="%(fmname)s" hostname="%(rcmshost)s" port="46000" qualifiedResourceType="rcms.fm.resource.qualifiedresource.FunctionManager" sourceURL="http://%(rcmshost)s:46000/functionmanagers/zhen/BrilDAQFunctionManager.jar" className="rcms.fm.app.brildaqfm.BrilDAQFunctionManager">'%({'fmname':fmname,'rcmshost':rcmshost})]
    fm_foot = ['</FunctionManager>']    
    return '\n'.join(fm_head+listofexecutive+fm_foot)

def makelistofexecutive(listofexecutives,zone=None,plainxml=False):
    result = []
    exetmpl = '''<XdaqExecutive hostname="%(hostname)s" port="%(port)s" urn="/urn:xdaq-application:lid=0" qualifiedResourceType="rcms.fm.resource.qualifiedresource.XdaqExecutive" role="%(role)s" instance="0" logURL="%(logURL)s" logLevel="%(logLevel)s" pathToExecutive="/opt/xdaq/bin/xdaq.exe" unixUser="%(unixUser)s" environmentString="%(environmentString)s">
      <configFile>%(contextconfig)s</configFile>
     </XdaqExecutive>     
     '''
    hostlist=[]    
    for e in listofexecutives:        
        contextconfig_xml = api.makecontextxml(e['hostname'],e['port'],e['endpoint'],e['apps'],zone=zone,withxmlhead=True) 
        contextconfig = contextconfig_xml 
        if not plainxml:
            contextconfig = _wrapinxml(contextconfig_xml)
        #print contextconfig
        result.append(exetmpl%({'hostname':e['hostname'],'port':str(e['port']),'role':e['role'],'logURL':e['logURL'],'logLevel':e['logLevel'],'unixUser':e['unixUser'],'environmentString':e['environmentString'],'contextconfig':contextconfig}))
        hostlist.append(e['hostname'])
    hosts = set(hostlist)
    jctmpl = '''<Service name="JobControl" hostname="%(hostname)s" port="9999" urn="/urn:xdaq-application:lid=10" qualifiedResourceType="rcms.fm.resource.qualifiedresource.JobControl" />'''
    for h in hosts:
        result.append(jctmpl%({'hostname':h}))
    return result

def _wrapinxml(contextconfig_xml):
    a = contextconfig_xml.replace('<','&lt;')
    a = a.replace('>','&gt;')
    return a

if __name__=='__main__':   
    args = docopt(__doc__)
    log.setLevel(logging.INFO)
    if args['--debug'] : log.setLevel(logging.DEBUG)
    log.debug(str(args))
    
    with open(args['<filename>']) as f:
        conf = yaml.safe_load(f)
        rcmsparams = conf.pop('rcmsparams',None)
        allfms = dict( (k,v) for k,v in conf.iteritems() if k!='host' )
        makercmsconfigurations(allfms,rcmsparams,outputdir=args['-o'],zone=args['-z'],plainxml=args['--plainxml'])
        
            
