#! /bin/bash

# Global Yate script that performs 2G/3G authentication
#
# Must be placed in same directory as do_nipc_comp128 and do_nipc_milenage
#
# Install in extmodule.conf
#
# [scripts]
# /path/to/nipc_auth.sh

cd `dirname $0`

# install in Yate and run main loop
echo "%%>setlocal:trackparam:nipc_auth"
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
		    res=`./do_nipc_$proto 0x$ki 0x$rand`
		    if [ ${#res} = 25 ]; then
			resp="sres=${res:0:8}:kc=${res:9}"
		    fi
		    ;;
		Xcomp128-1|Xcomp128-2|Xcomp128-3)
		    proto="${proto:0:7}_${proto:8}"
		    res=`./do_nipc_$proto 0x$ki 0x$rand`
		    if [ ${#res} = 25 ]; then
			resp="sres=${res:0:8}:kc=${res:9}"
		    fi
		    ;;
		Xmilenage)
		    sqn="${params#*:sqn=}"; sqn="${sqn%%:*}"
		    auts="${params#*:auts=}"; auts="${auts%%:*}"
		    amf="${params#*:amf=}"; amf="${amf%%:*}"
		    if [ -n "$amf" ]; then
			amf=" 0x$amf"
		    fi
		    opc="${params#*:opc=}"; opc="${opc%%:*}"
		    case "X$opc" in
			Xtrue|Xyes|Xon|Xenable|Xt|X1)
			    opc=" --opc"
			    ;;
			*)
			    opc=""
			    ;;
		    esac
		    if [ -n "$sqn" ]; then
			if [ -n "$auts" ]; then
			    res=`./do_nipc_milenage$opc --auts 0x$ki 0x$op 0x$sqn 0x$rand$amf`
			    if [ ${#res} = 28 ]; then
				resp="auts=${res}"
			    fi
			else
			    res=`./do_nipc_milenage$opc 0x$ki 0x$op 0x$sqn 0x$rand$amf`
			    if [ ${#res} = 115 ]; then
				resp="xres=${res:0:16}:ck=${res:17:32}:ik=${res:50:32}:autn=${res:83}"
			    fi
			fi
		    else
			if [ -n "$auts" ]; then
			    res=`./do_nipc_milenage$opc 0x$ki 0x$op 0x$auts 0x$rand$amf`
			    if [ ${#res} = 12 ]; then
				resp="sqn=${res}"
			    fi
			else
			    autn="${params#*:autn=}"; autn="${autn%%:*}"
			    res=`./do_nipc_milenage$opc 0x$ki 0x$op 0x$autn 0x$rand$amf`
			    if [ ${#res} = 95 ]; then
				resp="xres=${res:0:16}:ck=${res:17:32}:ik=${res:50:32}:sqn=${res:83}"
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
