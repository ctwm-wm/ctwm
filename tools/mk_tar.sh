#!/bin/sh
#
# Setup and generate a release tarball

# Make sure we're in the expected root of the tree
cd `dirname $0`/..

# Figure out version
version=`head -n1 VERSION`

# If it's a non-release, append date
if echo -n $version | grep -q '[^0-9\.]'; then
    version="$version.`date '+%Y%m%d'`"
fi

# Setup the dir
dir="ctwm-$version"
if [ -d $dir ] ; then
	echo "Dir '$dir' already exists!"
	exit;
fi
if [ -r $dir.tar ] ; then
	echo "Tarball '$dir.tar' already exists!"
	exit;
fi
mkdir -m755 $dir

# Create a totally fresh branch in it
bzr branch --use-existing-dir . $dir

# Generate the appropriate files for it
( cd $dir && make release_files allclean adoc_clean )

# Blow away the bzr metastuff, we don't need to package that
( cd $dir && rm -rf .bzr )

# Tar it up
tar \
	--uid 0 --uname ctwm --gid 0 --gname ctwm \
	-cvf $dir.tar $dir

# Cleanup
rm -rf $dir
ls -l $dir.tar
