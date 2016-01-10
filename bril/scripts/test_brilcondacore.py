print 'testing import'
import coral
import h5py
import matplotlib
import pandas
import numpy as np
print 'OK'

print 'testing matplotlib(graphics)'
import matplotlib.pyplot as plt
import matplotlib.patches as patches
import matplotlib.path as path

fig, ax = plt.subplots()

# histogram our data with numpy
data = np.random.randn(1000)
n, bins = np.histogram(data, 50)

# get the corners of the rectangles for the histogram
left = np.array(bins[:-1])
right = np.array(bins[1:])
bottom = np.zeros(len(left))
top = bottom + n

# we need a (numrects x numsides x 2) numpy array for the path helper
# function to build a compound path
XY = np.array([[left,left,right,right], [bottom,top,top,bottom]]).T

# get the Path object
barpath = path.Path.make_compound_path_from_polys(XY)

# make a patch out of it
patch = patches.PathPatch(barpath, facecolor='blue', edgecolor='gray', alpha=0.8)
ax.add_patch(patch)

# update the view limits
ax.set_xlim(left[0], right[-1])
ax.set_ylim(bottom.min(), top.max())

print 'close window to continue'
plt.show()

print 'OK'


print 'testing h5py(file persistency)'

with h5py.File('testinstall.hdf5','w') as f:
    ds = f.create_dataset('dset', (1000,), dtype='uint32')
    ds[...] = np.arange(1000)
with h5py.File('testinstall.hdf5','r') as f:
    assert(f['dset'][100]==100)
import os
os.remove('testinstall.hdf5')
print 'OK'

print 'testing pandas(analysis tool)'
import pandas as pd
import sys
df = pd.DataFrame([[1, 2], [1, 3], [4, 6]], columns=['A', 'B'])
print df.describe()
assert( df['A'].sum()==6 )
print 'OK'

print 'testing sqlite (db access)'
svc=coral.ConnectionService()
session=svc.connect('sqlite_file:write_test.db',accessMode=coral.access_Update)
session.transaction().start(False)
schema = session.nominalSchema()
schema.dropIfExistsTable( "T" )
description = coral.TableDescription()
description.setName( "T" )
description.insertColumn( "ID","int")
description.setPrimaryKey( "ID" );
description.insertColumn( "x","float")
description.setNotNullConstraint( "x" )
description.insertColumn( "Y","double")
description.insertColumn( "Z","double")
description.insertColumn( "data1","string", 100, False )
description.insertColumn( "data2", "blob")
table = schema.createTable( description )
session.transaction().commit();
print "Done"
del session
print 'testing oracle (db access)'
import os
os.environ['CORAL_AUTH_PATH']='/afs/cern.ch/cms/lumi/DB'
session=svc.connect('oracle://cms_orcoff_prep/cms_lumi_dev_offline',accessMode=coral.access_ReadOnly)
session.transaction().start(True)
session.transaction().commit();
print "Done"
del session
del svc
