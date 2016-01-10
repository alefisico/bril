#usage: readpytable.py filepath tablename
#example python readpytable.py /home/zhen/bcm1f/4859cbb3-925a-4e05-8fcb-2786959556c3_1411280925_0.hd5 bcm1fhist 
import sys,tables
filename = sys.argv[1]
tablename = sys.argv[2]
f=tables.open_file(filename)
datatable=None
print '### walk nodes in / ###'
for leaf in f.walk_nodes(where='/',classname="Leaf"):
   print leaf.name
   if leaf.name==tablename:
       print ' found '
       datatable = leaf

if datatable is None:
   print('%s not found'%tablename)  
   sys.exit(0)

print '### get node directly ###'
datatable = f.get_node(where='/',name=tablename,classname="Leaf")
print datatable.attrs.TITLE
#print datatable[1::3]

print '### query rows ###'
nrows = datatable.nrows
print 'nrows ',nrows
print 'last run ',datatable[nrows-1]['runnum']
print 'ls ',datatable[nrows-1]['lsnum']
print 'nb ',datatable[nrows-1]['nbnum']
print 'publishnnb ',datatable[nrows-1]['publishnnb']
print 'timestampsec ',datatable[nrows-1]['timestampmsec']
print 'data length ',len(datatable[nrows-1]['data'])
print 'data ',datatable[nrows-1]['data']
print datatable.description
print("Variable names on datatable with their type:")
for name in datatable.colnames:
    print(name,datatable.coldtypes[name])

selectedbyruid = [ (p['runnum'],p['lsnum'],p['nbnum'],p['data']) for p in datatable if p['ruid']==1]
print selectedbyruid

for row in datatable.where('(ruid==1) & (runnum==123450) & (lsnum==1) & (nbnum==1)'):
    print len(row['data'])

selectedbytime = [ (p['ruid']) for p in datatable if p['runnum']==123450 and p['lsnum']==1 and p['nbnum']==1 ]
print selectedbytime

f.close()
