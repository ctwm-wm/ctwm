#!/bin/sh
#
# Setup and generate a release tarball

# Make sure we're in the expected root of the tree
cd `dirname $0`
rtdir=`git rev-parse --show-toplevel`
cd $rtdir

# Figure out version
version=`head -n1 VERSION`
if [ ! -z "$1" ]; then
	# Completely override from the command line
	version=$1
elif echo -n $version | grep -q '[^0-9\.]'; then
	# If it's a non-release, append date
	version="$version.`date '+%Y%m%d'`"
fi

# Setup the dir
tmpdir=`mktemp -d "${rtdir}/ctwm-mktar.XXXXXX"`
dirname="ctwm-$version"
dir="${tmpdir}/ctwm-$version"
if [ -d $dir ] ; then
	echo "Dir '$dir' already exists!"
	exit;
fi
if [ -r $rtdir/$dirname.tar ] ; then
	echo "Tarball '$dirname.tar' already exists!"
	exit;
fi
mkdir -pm755 $dir

# Create a totally fresh repo
git clone $rtdir $dir

# Do various setup in the branch to prepare
(
	cd $dir

	# Generate the appropriate files for it, and clean up intermediate
	# products.
	make release_files allclean adoc_clean

	# Blow away the git metastuff, we don't need to package that
	rm -rf .git
)

# Tar it up
( cd $tmpdir && tar \
		--uid 0 --uname ctwm --gid 0 --gname ctwm \
		-cvf $rtdir/$dirname.tar $dirname
)

# Cleanup
rm -rf $tmpdir
ls -l $dirname.tar
