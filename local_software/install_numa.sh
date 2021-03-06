#! /bin/bash

source config.sh


#
# NUMA CTL
#

if [ "`hostname -f`" == "bgq-fn.rcsg.rice.edu" ]; then
	# compile NUMA lib with GNU toolchain
	CC=gcc
	CXX=g++
	LINK=g++
fi


echo "*** NUMA CTL ***"
if [ "`uname -s`" != "Linux" ]; then
	echo "This script only supports NUMACTL on Linux systems"
else

	if [ ! -e "$DST_DIR/lib/libnuma.so" -o "$1" != "" ]; then
		SRC_LINK="http://www.martin-schreiber.info/pub/sweet_local_software/numactl-2.0.11.tar.gz"
		#SRC_LINK="ftp://oss.sgi.com/www/projects/libnuma/download/numactl-2.0.11.tar.gz"
		FILENAME="`basename $SRC_LINK`"
		BASENAME="numactl-2.0.11"

		cd "$SRC_DIR"

		download "$SRC_LINK" "$FILENAME" || exit 1

		tar xzf "$FILENAME"
		cd "$BASENAME"

		./configure --prefix="$DST_DIR" || exit 1
		make -j install

		echo "DONE"

	fi
fi
