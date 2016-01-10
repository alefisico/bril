#!/usr/bin/env python
import os,sys,socket
#scan local host for free port in given range
def scanfreeport(candidates):
    localIP = socket.gethostbyname(socket.gethostname())
    result=[]
    for port in candidates:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        r = sock.connect_ex((localIP, port))
        if r != 0: #nobody is listening, so free
            result.append(port)
        sock.close()
    return result

if __name__ == '__main__':
  pmin=30000
  pmax=30010
  if len(sys.argv)>1:
    pmin=int(sys.argv[1])
    pmax=pmin+10
  if len(sys.argv)>2:
    pmax=int(sys.argv[2])
  print 'scan range: [%d,%d)'%(pmin,pmax)
  candidates = xrange(pmin,pmax)
  freeports = scanfreeport(candidates)
  print 'free ports:',freeports
