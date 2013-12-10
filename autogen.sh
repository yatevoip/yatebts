#!/bin/sh

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
