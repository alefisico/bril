import sys,os
def list_files(startpath):
    for root, dirs, files in os.walk(startpath):
        if root.find('.svn')!=-1 : continue
        level = root.replace(startpath, '').count(os.sep)
        indent = ' ' * 4 * (level)
        print('{}{}/'.format(indent, os.path.basename(root)))
        subindent = ' ' * 4 * (level + 1)
        if not '.svn' in dirs and not 'x86_64_slc6' in dirs and not 'linux' in dirs:
            for f in files:
                print('{}{}'.format(subindent, f))

if __name__=='__main__':
    startdir='.'
    if len(sys.argv)>1:
      startdir=sys.argv[1]
    print 'scanning %s'%startdir
    list_files(startdir)
