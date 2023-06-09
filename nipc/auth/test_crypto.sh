#! /bin/sh

all0="000000000000"

test=`./do_nipc_milenage 0xA82573E41613C37D8F8F1EBC829FBF5C 0x467D46FB75EEC4DB1548EBE3D2004EFF 0x753fd2894f2f2455c152b23a7c40 0x6374409263c9b4f62bd7cf2665b1c054`
if [ "x$test" = "x$all0" ]; then
    echo "MILENAGE test 1 OK"
else
    echo "MILENAGE test 1 failed: got '$test', expecting '$all0'" >&2
fi

test=`./do_nipc_milenage 0xA82573E41613C37D8F8F1EBC829FBF5C 0x467D46FB75EEC4DB1548EBE3D2004EFF 0xBAC45D181DB00FCB3A76339051E7 0x337F180A6F0FA6921E21040E7A09D9A2`
if [ "x$test" = "x$all0" ]; then
    echo "MILENAGE test 2 OK"
else
    echo "MILENAGE test 2 failed: got '$test', expecting '$all0'" >&2
fi

exp="F7A6A2CA54261C0A 9E0AFBCF1144DA7692746FF52C9A35EC 67CC2BD2BD96FF16C6A19E5C10753D1B 343A26789DC70000EF4765A1B6DD016D"
test=`./do_nipc_milenage 0x5B238655E11E37717D7DD8E7B2A6378C 0x467D46FB75EEC4DB1548EBE3D2004EFF 0x00000000011C 0x88F2AC8A88F0F7CEC9D72CB29DB58DB0`
if [ "x$test" = "x$exp" ]; then
    echo "MILENAGE test 3 OK"
else
    echo "MILENAGE test 3 failed: got '$test', expecting '$exp'" >&2
fi

exp="F7A6A2CA54261C0A 9E0AFBCF1144DA7692746FF52C9A35EC 67CC2BD2BD96FF16C6A19E5C10753D1B 00000000011C"
test=`./do_nipc_milenage 0x5B238655E11E37717D7DD8E7B2A6378C 0x467D46FB75EEC4DB1548EBE3D2004EFF 0x343A26789DC70000EF4765A1B6DD016D 0x88F2AC8A88F0F7CEC9D72CB29DB58DB0`
if [ "x$test" = "x$exp" ]; then
    echo "MILENAGE test 4 OK"
else
    echo "MILENAGE test 4 failed: got '$test', expecting '$exp'" >&2
fi

exp="BAC45D181DB00FCB3A76339051E7"
test=`./do_nipc_milenage --auts 0xA82573E41613C37D8F8F1EBC829FBF5C 0x467D46FB75EEC4DB1548EBE3D2004EFF 0x000000000000 0x337F180A6F0FA6921E21040E7A09D9A2`
if [ "x$test" = "x$exp" ]; then
    echo "MILENAGE test 5 OK"
else
    echo "MILENAGE test 5 failed: got '$test', expecting '$exp'" >&2
fi

exp="F7A6A2CA54261C0A 9E0AFBCF1144DA7692746FF52C9A35EC 67CC2BD2BD96FF16C6A19E5C10753D1B 343A26789DC790014D8EE02734ACC628"
test=`./do_nipc_milenage --opc 0x5B238655E11E37717D7DD8E7B2A6378C 0xBEC199A5AFB432DF7E7A3E457A992668 0x00000000011C 0x88F2AC8A88F0F7CEC9D72CB29DB58DB0 0x9001`
if [ "x$test" = "x$exp" ]; then
    echo "MILENAGE test 6 OK"
else
    echo "MILENAGE test 6 failed: got '$test', expecting '$exp'" >&2
fi

exp="BAC45D181DB00FCB3A76339051E7"
test=`./do_nipc_milenage --opc --auts 0xA82573E41613C37D8F8F1EBC829FBF5C 0xC69B87AF18CEFEB6FB0083BA51E8D200 0x000000000000 0x337F180A6F0FA6921E21040E7A09D9A2`
if [ "x$test" = "x$exp" ]; then
    echo "MILENAGE test 7 OK"
else
    echo "MILENAGE test 7 failed: got '$test', expecting '$exp'" >&2
fi


exp="F38D6AE6 251D97EBBECE7000"
test=`./do_nipc_comp128 0xFE328519ACE736DCE836AD893BCE8721 0xEFEDB342A03A4D38E7AB8CEE4E133D1A`
if [ "x$test" = "x$exp" ]; then
    echo "COMP128 test 1 OK"
else
    echo "COMP128 test 1 failed: got '$test', expecting '$exp'" >&2
fi

exp="0DF072B6 5DE5E352FD076000"
test=`./do_nipc_comp128 0xFE328519ACE736DCE836AD893BCE8721 0x8CBF6C5893F0F9489BF9C8064B2DA276`
if [ "x$test" = "x$exp" ]; then
    echo "COMP128 test 2 OK"
else
    echo "COMP128 test 2 failed: got '$test', expecting '$exp'" >&2
fi
