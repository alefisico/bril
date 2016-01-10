_tojchttpheaders={'Content-type':'text/xml','Content-Description':'SOAP Message','SOAPAction':'urn:xdaq-application:service=jobcontrol'}
_toexechttpheaders={'Content-type':'text/xml','Content-Description':'SOAP Message','SOAPAction':'urn:xdaq-application:lid=0'}
_defaultmsgtpl="""<xdaq:%(cmmd)s xmlns:xdaq="urn:xdaq-soap:3.0"></xdaq:%(cmmd)s>"""
_soapenvelope="""<SOAP-ENV:Envelope SOAP-ENV:encodingStyle="http://schemas.xmlsoap.org/soap/encoding/" xmlns:SOAP-ENV="http://schemas.xmlsoap.org/soap/envelope/" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xmlns:xsd="http://www.w3.org/2001/XMLSchema" xmlns:SOAP-ENC="http://schemas.xmlsoap.org/soap/encoding/"><SOAP-ENV:Header></SOAP-ENV:Header><SOAP-ENV:Body>%s</SOAP-ENV:Body></SOAP-ENV:Envelope>"""
_envtpl="""<EnvironmentVariable %(envs)s />"""
_defaultxdaqenv=' '.join(['XDAQ_PORT="$XDAQport"',
                                     'LD_LIBRARY_PATH="/opt/xdaq/lib:/opt/xdaq/lib64:/usr/lib/debug"',
                                     'XDAQ_DOCUMENT_ROOT="/opt/xdaq/htdocs"',
                                     'XDAQ_OS="linux"',
                                     'XDAQ_PLATFORM="x86"',
                                     'XDAQ_ROOT="/opt/xdaq"',
                                     'XDAQ_SETUP_ROOT="/opt/xdaq/share"'
                                     ]) 

_xmlhead = '<?xml version="1.0" encoding="UTF-8"?>'
_partitionhead = '<xc:Partition xmlns:soapenc="http://schemas.xmlsoap.org/soap/encoding/" xmlns:xc="http://xdaq.web.cern.ch/xdaq/xsd/2004/XMLConfiguration-30" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance">'
_partitionfoot = '</xc:Partition>'


import urllib2,sys,os,socket
import xml.etree.ElementTree as ET
from cStringIO import StringIO

class soapCommand(object):
    def __init__(self,desthost,destport,desttype,**kwds): 
        ## setting reasonable defaults. overwrite getter for specific cases where necessary    
        self._cmmd = self.__class__.__name__
        self._desthost = desthost
        self._destport = destport
        if desttype=='jc':
           self._httpheaders =  _tojchttpheaders
        elif desttype=='exec':
           self._httpheaders =  _toexechttpheaders
        self._responseTag = self.cmmd+'Response'
        self._execpath = '/opt/xdaq/bin/xdaq.exe'
        self._execport = 40000         
        self._user = 'brilpro'
        self._jid = ''

    #mutable members
    @property
    def cmmd(self):
        return self._cmmd
    @cmmd.setter
    def cmmd(self,value):
        self._cmmd = value
    @property
    def responseTag(self):
        return self._responseTag
    @cmmd.setter
    def responseTag(self,value):
        self._responseTag = value
    @property
    def execpath(self):
        return self._execpath
    @execpath.setter
    def execpath(self,value):
        self._execpath = value
    @property
    def execport(self):
        return self._execport
    @execport.setter
    def execport(self,value):
        self._execport = value 
    @property
    def user(self):
        return self._user
    @user.setter
    def user(self,value):
        self._user = value 
    @property
    def jid(self):
        if not self._jid: self._jid = self.execurl
        return self._jid
    @jid.setter
    def jid(self,value):
        self._jid = value

    #readonly members
    @property
    def desturl(self):
        return 'http://%s:%s'%(self._desthost,str(self._destport))
    @property
    def httpheaders(self):
        return self._httpheaders 
    @property
    def msg(self):   
        return _soapenvelope%(_defaultmsgtpl%{'cmmd':self._cmmd})
    
    @property
    def execurl(self):
        return 'http://%s:%s/urn:xdaq-application:lid=0'%(self._desthost,self._execport)

    def printparameters(self,fields=['cmmd','msg']):
        for k in fields: 
            print '%12s : %s'%(k,getattr(self,k))
    
    def _parseResponse(self,responsetag,responsestr):
        tree = ET.parse(responsestr)
        nodeiter = tree.find('.//{urn:rcms-soap:2.0}%s'%responsetag)
        result = {}
        if nodeiter is None: return result
        for node in nodeiter:
            result[node.tag.split('}')[1]]=node.text   
        return result

    def post(self):
        request = urllib2.Request(self.desturl,self.msg,self.httpheaders) 
        response = urllib2.urlopen(request).read()
        print response
        result = self._parseResponse(self.responseTag,StringIO(response))
        return result

class jcCommand(soapCommand):
    def __init__(self,desthost,destport,**kwds):
         super(jcCommand,self).__init__(desthost,destport,'jc',**kwds)
         
class execCommand(soapCommand):
    def __init__(self,desthost,destport,**kwds):
         super(execCommand,self).__init__(desthost,destport,'exec',**kwds)
    
class killAll(jcCommand):
    pass

class killExec(jcCommand):
    def __init__(self,desthost,destport):
        super(killExec,self).__init__(desthost,destport)
    @property
    def msg(self):     
        cmmdtpl = '<xdaq:%(cmmd)s xmlns:xdaq="urn:xdaq-soap:3.0" jid="%(jid)s"></xdaq:%(cmmd)s>'
        return _soapenvelope%(cmmdtpl%({'cmmd':self._cmmd,'jid':self.execurl}))
 
class getJobStatus(jcCommand):
    def __init__(self,desthost,destport):
        super(getJobStatus,self).__init__(desthost,destport)
    @property
    def msg(self):
        cmmdtpl = '<xdaq:%(cmmd)s xmlns:xdaq="urn:xdaq-soap:3.0" jid="%(jid)s"></xdaq:%(cmmd)s>'
        return _soapenvelope%(cmmdtpl%({'cmmd':self._cmmd,'jid':self.jid}))

class executeCommand(jcCommand):    
    def  __init__(self,desthost,destport,argv=''):
        super(executeCommand,self).__init__(desthost,destport)
        self._argv = argv 
    @property
    def jid(self):
        if not self._jid:         
            self._jid = os.path.basename(self.execpath) # implicitly use command name as jid
        return self._jid
    @property
    def msg(self): 
        cmmdtpl = '<xdaq:%(cmmd)s xmlns:xdaq="urn:xdaq-soap:3.0" execPath="%(execpath)s" user="%(user)s" argv="%(argv)s" jid="%(jid)s"></xdaq:%(cmmd)s>'
        return _soapenvelope%(cmmdtpl%{'cmmd':self.cmmd,'execpath':self.execpath,'user':self.user,'argv':self._argv,'jid':self.jid})

class startXdaqExe(jcCommand):
    def __init__(self,desthost,destport):
        super(startXdaqExe,self).__init__(desthost,destport)
        self._zone = ''
        self._envs = _defaultxdaqenv  
    @property
    def msg(self):
        argv = '-h %(host)s -p %(execport)s --stdout /tmp/jc_%(cmmd)s.stdout --stderr /tmp/jc_%(cmmd)s.stderr -u file.append:/tmp/jc_%(cmmd)s.log'%({'host':self._desthost,'execport':self._execport,'cmmd':self._cmmd})
        if self._zone:
            argv += ' -z %s '%self._zone
        envstr = _envtpl%{'envs':self._envs}
        cmmdtpl = '<xdaq:%(cmmd)s xmlns:xdaq="urn:xdaq-soap:3.0" execPath="%(execpath)s" user="%(user)s" argv="%(argv)s" execURL="%(execurl)s" jid="%(jid)s" > %(envstr)s </xdaq:%(cmmd)s>' 
        return _soapenvelope%(cmmdtpl%({'cmmd':self._cmmd,'execpath':self._execpath,'user':self._user,'argv':argv,'execurl':self.execurl,'jid':self.jid,'envstr':envstr}))
    @property
    def zone(self):
        return self._zone
    @zone.setter
    def zone(self,value):
        self._zone = value
    @property
    def envs(self):
        return self._envs
    @envs.setter
    def envs(self,value):
        self._envs = value

class Configure(execCommand):
    def __init__(self,desthost,destport):
        super(Configure,self).__init__(desthost,destport)
        self._execport = destport # no jc , so execport and destport is the same
        self._xmlcontent = ''
    @property
    def desturl(self):        
        return 'http://%s:%s/urn:xdaq-application:lid=0'%(self._desthost,str(self._destport))

    @property
    def xmlcontent(self):
        return self._xmlcontent
    @xmlcontent.setter
    def xmlcontent(self,value):
        self._xmlcontent = value    
    @property
    def msg(self):
        cmmdtpl = '<xdaq:%(cmmd)s xmlns:xdaq="urn:xdaq-soap:3.0">%(xmlcontent)s </xdaq:%(cmmd)s>'
        return _soapenvelope%(cmmdtpl%{'cmmd':self.cmmd,'xmlcontent':self.xmlcontent})

def makeproperty(idx,propertydict):
    head = ['<item xsi:type="soapenc:Struct" soapenc:position="[%d]">'%(idx)]
    foot = ['</item>']
    contents = []
    contents.append('<properties xsi:type="soapenc:Struct">')
    for name,value in propertydict.items():
        contents.append('<%(name)s xsi:type="xsd:string">%(value)s</%(name)s>'%({'name':name,'value':value}) )
    contents.append('</properties>')
    return ''.join(head+contents+foot)

def makelist(idx,itemfields):
    head = ['<item xsi:type="xsd:string" soapenc:position="[%d]">'%(idx)]
    foot = ['</item>']
    fieldtpl = itemfields[0]
    contents = [ fieldtpl%(tuple(itemfields[1:])) ]
    return ''.join(head+contents+foot)

def makearrayconf(tagname,listofitems):
    head = ['<%s xsi:type="soapenc:Array" soapenc:arrayType="xsd:ur-type[%d]">'%(tagname,len(listofitems))]
    foot = ['</%s>'%(tagname)]
    contents = []
    for idx,p in enumerate(listofitems):
        if isinstance(p,list) : #list of list
            contents.append(makelist(idx,p))
        elif isinstance(p,dict) :
            contents.append(makeproperty(idx,p))
    return '\n'.join(head+contents+foot)
    
def makecontextxml(hostname,port,endpoint,listofapps,zone=None,withxmlhead=True):
    xmlhead=[_xmlhead]
    if not withxmlhead:
        xmlhead=[]
    partitionhead=[_partitionhead]
    partitionfoot=[_partitionfoot]
    contexthead = ['<xc:Context url="http://%s:%s">'%(hostname,str(port))]
    contextfoot = ['</xc:Context>']
    endpoint = ['<xc:Endpoint protocol="utcp" service="b2in" rmode="select" hostname="%(hostname)s" port="%(endpoint)s" network="utcp1" sndTimeout="2000" rcvTimeout="2000" affinity="RCV:P,SND:W,DSR:W,DSS:W" singleThread="true" publish="true"/>'%({'hostname':hostname,'endpoint':str(endpoint)})]
    listofappsconf = []
    for a in listofapps:
        tpl=''
        if zone is not None and len(zone)!=0 and os.path.basename(a['config'])=='ptutcp.tpl':
            continue #skip ptutcp if in zone
        f = open(a['config'])
        tpl = f.read().rstrip('\n')
        f.close()                   
        tplvalues = {}
        for k,v in a.items():
            if k=='config': continue
            if isinstance(v,list):
                tplvalues[k] = makearrayconf(k,v)
            else:
                tplvalues[k] = v
        listofappsconf.append(tpl%tplvalues)
    return '\n'.join(xmlhead+partitionhead+contexthead+endpoint+listofappsconf+contextfoot+partitionfoot)

if __name__=='__main__':
  defaultjcport=9999
  l=killAll('pczhen.cern.ch',defaultjcport)
  l.printparameters() 

  l = killExec('pczhen.cern.ch',defaultjcport)
  l.execport=40010
  l.printparameters() 

  l.destport=40010
  l = Configure('pczhen.cern.ch',defaultjcport)
  l.printparameters()

  l=startXdaqExe('pczhen.cern.ch',defaultjcport)
  print l.msg

  l=startXdaqExe('pczhen.cern.ch',defaultjcport)
  l.zone = 'bril'
  l.execport=5678
  l.printparameters(['cmmd','msg','jid'])

  l=Configure('pczhen.cern.ch',49990)
  l.xmlcontent='blah'
  l.printparameters(['cmmd','desturl','execurl','msg'])

