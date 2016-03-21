#!/bin/sh
# $0 event_names.list event_names.c.in > event_names.c

enames=$1
in=$2

# Generate the string of the designated initializer for the array
ENSTR=""
for i in `cat $enames`; do
	ENSTR="$ENSTR [$i] = \"$i\","
done

# Now translate
sed -e "s/%%EVENT_NAMES%%/$ENSTR/" ${in}
