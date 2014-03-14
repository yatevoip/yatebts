#!/bin/sh

# autogen.sh
# This file is part of the Yate-BTS Project http://www.yatebts.com
#
# Yet Another Telephony Engine - Base Transceiver Station
# Copyright (C) 2013-2014 Null Team Impex SRL
#
# This software is distributed under multiple licenses;
# see the COPYING file in the main directory for licensing
# information for this specific distribution.
#
# This use of this software may be subject to additional restrictions.
# See the LEGAL file in the main directory for details.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.


# Run this to generate a new configure script

if [ -z `which which 2>/dev/null` ]; then
    echo "Please install the required 'which' utility." >&2
    exit 1
fi

ac=`which autoconf 2>/dev/null`
test -z "$ac" && ac=/usr/local/gnu-autotools/bin/autoconf
if [ -x "$ac" ]; then
    "$ac" || exit $?
    case "x$1" in
	x--silent)
	    ;;
	x--configure)
	    shift
	    ./configure "$@" || exit $?
	    echo "Finished! Now run make."
	    ;;
	*)
	    echo "Finished! Now run configure. If in doubt run ./configure --help"
	    ;;
    esac
else
    echo "Please install Gnu autoconf to build from CVS." >&2
    exit 1
fi
