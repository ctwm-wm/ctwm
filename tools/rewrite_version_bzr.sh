#!/bin/sh
#
# Rewrite version.c.in (well, technically, stdin) with bzr revision info.
# We assume if we're getting called, bzr is available.


# "revno revid" of the working tree
REVID=`bzr revision-info --tree | cut -wf2-`
if [ $? -ne 0 ]; then
	# Failed somehow
	REVID="???"
fi

# That's all we need; just pass stdin through and sub
while read line; do
	echo "${line}" | sed -e "s/%%REVISION%%/\"${REVID}\"/"
done
