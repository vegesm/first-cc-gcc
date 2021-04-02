#!/bin/sh

UNDERSCORE=_
if [ "$(expr substr $(uname -s) 1 5)" = "Linux" ]; then
	UNDERSCORE=
fi

# convert original asm listing and remove comments
cat $1  | ./cvopt  | sed 's|/.*||g'  | \
	# convert the lookup table at the start to use .int directives
	sed -r 's/([0-9]+)\.;[[:blank:]]+([A-Za-z0-9]+)/.int \1, \2/g' | \
	# replace .even directive with .balign, replace first row of the lookup table
	sed 's/\.even/.balign 4/g' | sed -r 's/_(eff|reg|sp|cc)tab=\.;[[:blank:]]*\.\+2/_\1tab:.int .,.+4/g'  | \
	# replace single `0` characters with .int directive
	sed -r 's/^([[:blank:]]+)0$/\1.int 0/g' | \
	# remove leading underscore on linux
	sed -r 's/_(eff|reg|sp|cc)tab/'$UNDERSCORE'\1tab/'
