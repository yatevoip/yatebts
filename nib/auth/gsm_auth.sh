#! /bin/bash

# Global Yate script that performs 2G/3G authentication
#
# Must be placed in same directory as do_comp128 and do_milenage
#
# Install in extmodule.conf
#
# [scripts]
# /path/to/gsm_auth.sh

cd `dirname $0`

# install in Yate and run main loop
echo "%%>setlocal:trackparam:gsm_auth"
echo "%%>install:95:gsm.auth"
echo "%%>setlocal:restart:true"
while read -r REPLY; do
    case "$REPLY" in
	%%\>message:*)
	    # gsm.auth handling
	    id="${REPLY#*:*}"; id="${id%%:*}"
	    params=":${REPLY#*:*:*:*:*:}:"
	    # extract parameters, assume we don't need unescaping
	    rand="${params#*:rand=}"; rand="${rand%%:*}"
	    ki="${params#*:ki=}"; ki="${ki%%:*}"
	    op="${params#*:op=}"; op="${op%%:*}"
	    proto="${params#*:protocol=}"; proto="${proto%%:*}"
	    resp=""
	    case "X$proto" in
		Xcomp128)
		    case "X$op" in
			X1|X2|X3)
			    proto="${proto}_${op}"
			    ;;
		    esac
		    res=`./do_$proto 0x$ki 0x$rand`
		    if [ ${#res} = 25 ]; then
			resp="sres=${res:0:8}:kc=${res:9}"
		    fi
		    ;;
		Xmilenage)
		    sqn="${params#*:sqn=}"; sqn="${sqn%%:*}"
		    if [ -n "$sqn" ]; then
			res=`./do_milenage 0x$ki 0x$op 0x$sqn 0x$rand`
			if [ ${#res} = 115 ]; then
			    resp="xres=${res:0:16}:ck=${res:17:32}:ik=${res:50:32}:autn=${res:83}"
			fi
		    else
			auts="${params#*:auts=}"; auts="${auts%%:*}"
			if [ -n "$auts" ]; then
			    res=`./do_milenage 0x$ki 0x$op 0x$auts 0x$rand`
			    if [ ${#res} = 12 ]; then
				resp="sqn=${res}"
			    fi
			else
			    autn="${params#*:autn=}"; autn="${autn%%:*}"
			    res=`./do_milenage 0x$ki 0x$op 0x$autn 0x$rand`
			    if [ ${#res} = 82 ]; then
				resp="xres=${res:0:16}:ck=${res:17:32}:ik=${res:50:32}"
			    fi
			fi
		    fi
		    ;;
	    esac
	    if [ -n "$resp" ]; then
		echo "%%<message:$id:true:::$resp"
	    else
		echo "%%<message:$id:false::"
	    fi
	    ;;
    esac
done

# vi: set ts=8 sw=4 sts=4 noet:
