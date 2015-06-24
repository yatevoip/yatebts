# Process this file with autoconf to produce a configure script.
AC_INIT(Yate-BTS, 5.0.1)
AC_CONFIG_SRCDIR([README])
AC_PREREQ(2.52)
AC_CONFIG_HEADERS([config.h])

if echo "$ac_confdir/$ac_pwd" | grep -q '[[[:space:]\\\$\"'\'']]'; then
    AC_ERROR([Unsupported character(s) in source or current directory name])
fi

PACKAGE_RELEASE="1"
PACKAGE_STATUS="devel"

PACKAGE_REVISION=`svn info 2>/dev/null | sed -n 's,^Revision: *,,p'`
test -z "$PACKAGE_REVISION" && PACKAGE_REVISION=`cat "$srcdir/revision.txt" 2>/dev/null`

PACKAGE_VERSION_MAJOR="${PACKAGE_VERSION%%.*}"
PACKAGE_VERSION_MINOR="${PACKAGE_VERSION#*.}"
PACKAGE_VERSION_MINOR="${PACKAGE_VERSION_MINOR%.*}"
PACKAGE_VERSION_BUILD="${PACKAGE_VERSION##*.}"
PACKAGE_VERSION_RELEASE="$((0+${PACKAGE_RELEASE}))"
AC_SUBST(PACKAGE_VERSION_MAJOR)
AC_SUBST(PACKAGE_VERSION_MINOR)
AC_SUBST(PACKAGE_VERSION_BUILD)
AC_SUBST(PACKAGE_VERSION_RELEASE)
AC_SUBST(PACKAGE_RELEASE)
AC_SUBST(PACKAGE_STATUS)
AC_SUBST(PACKAGE_REVISION)

# We may need the host OS type but avoid the overhead of AC_CANONICAL_SYSTEM
AC_MSG_CHECKING([for local operating system type])
uname_os=`uname -s`
test -x "$uname_os" && uname_os=Unknown
AC_MSG_RESULT([$uname_os])

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_AWK

# Delete -g as we handle debug info at build time
CFLAGS=`echo "$CFLAGS" | sed 's/\(^\| \+\)-g[[0-9]]*//' 2>/dev/null`

# Helper to check by pkgconfig including local paths
function pkgconfig_check()
{
    res=`(pkg-config "$@") 2>/dev/null`
    [[ -z "$res" ]] && res=`(PKG_CONFIG_PATH=/usr/local/lib64/pkgconfig pkg-config "$@") 2>/dev/null`
    [[ -z "$res" ]] && res=`(PKG_CONFIG_PATH=/usr/local/lib/pkgconfig pkg-config "$@") 2>/dev/null`
    printf '%s\n' "$res" 2>/dev/null
}

AC_MSG_CHECKING([for sed command to use])
# sed expression to convert version into something comparable lexicographically
csed='sed'
vsed='s/\(^\|\.\)\([[0-9]]\)\($\|\.\)/\10\2\3/g'
if [[ -n "`echo a | sed 's/a\|b//'`" ]]; then
    csed='sed -E'
    vsed='s/(^|\.)([[0-9]])($|\.)/\10\2\3/g'
fi
AC_MSG_RESULT([$csed])

# Check compiler and platform specifics
AC_C_CONST
AC_C_INLINE
AC_TYPE_SIZE_T
AC_HEADER_TIME
AC_C_BIGENDIAN(,[CFLAGS="$CFLAGS -DLITTLE_ENDIAN"])

# Check for mandatory features
AC_CHECK_HEADERS([sys/socket.h sys/time.h pthread.h syslog.h], , [AC_MSG_ERROR([This header file is required.])])
AC_CHECK_FUNCS([openlog syslog], , [AC_MSG_ERROR([This function is required.])])

# Check for less critical stuff
AC_CHECK_HEADERS([byteswap.h sys/prctl.h])

# Check for Yate (required)
YATE_REQ="5.5.1"
YATE_VER=""
YATE_DEF=""
YATE_INC=""
YATE_LIB=""
YATE_LNK=""
YATE_STR=""
YATE_MOD=""
YATE_SCR=""
YATE_SHR=""
YATE_CFG=""
THREAD_LIB=""
INSTALL_D=""
INSTALL_L=""
yc="yate-config"
AC_ARG_WITH(yate,AC_HELP_STRING([--with-yate=DIR],[use Yate from DIR]),[ac_cv_use_yate=$withval],[ac_cv_use_yate=yes])
if [[ "x$ac_cv_use_yate" = "xno" ]]; then
    YATE_VER="no"
else
if [[ "x$ac_cv_use_yate" != "xyes" ]]; then
    dir=`cd "$ac_cv_use_yate"; pwd`
    yc="$dir/$yc"
    YATE_DEF="-I$dir -I$dir/libs/yradio"
    YATE_LIB="-L$dir"
fi
AC_MSG_CHECKING([for Yate using $yc])
YATE_VER=`"$yc" --version 2>/dev/null`
YATE_DEF="$YATE_DEF "`"$yc" --c-all 2>/dev/null`
YATE_LNK="$YATE_LIB "`"$yc" --ld-nostrip 2>/dev/null`
YATE_LIB="-l${YATE_LNK##* -l}"
YATE_LNK="${YATE_LNK%% -l*}"
YATE_STR=`"$yc" --ld-strip 2>/dev/null`
YATE_MOD=`"$yc" --modules 2>/dev/null`
YATE_SCR=`"$yc" --scripts 2>/dev/null`
YATE_SHR=`"$yc" --share 2>/dev/null`
YATE_CFG=`"$yc" --config 2>/dev/null`
THREAD_LIB=`"$yc" --param=THREAD_LIB 2>/dev/null`
INSTALL_D=`"$yc" --param=INSTALL_D 2>/dev/null`
INSTALL_L=`"$yc" --param=INSTALL_L 2>/dev/null`
YATE_INC=`echo "$CFLAGS $YATE_DEF -DHAVE_CONFIG_H" | sed 's/-fno-exceptions *//; s/  / /g' 2>/dev/null`
if [[ "x$YATE_VER" = "x" ]]; then
    YATE_VER="no"
fi
AC_MSG_RESULT([$YATE_VER])
if [[ "x$YATE_VER" = "xno" ]]; then
    AC_ERROR([Could not find Yate])
fi
yver=`echo "$YATE_VER" | $csed "$vsed" | $csed "$vsed" | sed 's/\(..\)\.\(..\)\.\(..\)/\1\2\3/'`
yreq=`echo "$YATE_REQ" | $csed "$vsed" | $csed "$vsed" | sed 's/\(..\)\.\(..\)\.\(..\)/\1\2\3/'`
if [[ "1$yver" -lt "1$yreq" ]]; then
    AC_ERROR([Requires at least Yate version $YATE_REQ])
fi
fi
AC_SUBST(YATE_REQ)
AC_SUBST(YATE_VER)
AC_SUBST(YATE_DEF)
AC_SUBST(YATE_INC)
AC_SUBST(YATE_LIB)
AC_SUBST(YATE_LNK)
AC_SUBST(YATE_STR)
AC_SUBST(YATE_MOD)
AC_SUBST(YATE_SCR)
AC_SUBST(YATE_SHR)
AC_SUBST(YATE_CFG)
AC_SUBST(THREAD_LIB)
AC_SUBST(INSTALL_D)
AC_SUBST(INSTALL_L)


# Check for sse3 operations
HAVE_SSE3=no
AC_ARG_ENABLE(sse3,AC_HELP_STRING([--enable-sse3],[Enable SSE3 operations (default: no)]),want_sse3=$enableval,want_sse3=no)
if [[ "x$want_sse3" != "xno" ]]; then
AC_MSG_CHECKING([whether to use SSE3 operations])
AC_LANG_SAVE
AC_LANG_C
SAVE_CFLAGS="$CFLAGS"
CFLAGS="$CFLAGS -Wall -Werror -msse3"
AC_TRY_LINK([#include <stdint.h>
#include <math.h>
#include <xmmintrin.h>
#include <emmintrin.h>
#include <pmmintrin.h>
],[
__m128 a = _mm_set1_ps(1.0f);
__m128 b = _mm_set1_ps(2.0f);
a = _mm_hadd_ps(a,b);
return 0;
],
HAVE_SSE3="yes",
want_sse3="no (missing -march ?)"
)
CFLAGS="$SAVE_CFLAGS"
AC_LANG_RESTORE
AC_MSG_RESULT([$want_sse3])
fi
AC_SUBST(HAVE_SSE3)


# Check for sse4.1 operations
HAVE_SSE41=no
AC_ARG_ENABLE(sse41,AC_HELP_STRING([--enable-sse41],[Enable SSE4.1 operations (default: no)]),want_sse41=$enableval,want_sse41=no)
if [[ "x$want_sse41" != "xno" ]]; then
AC_MSG_CHECKING([whether to use SSE4.1 operations])
AC_LANG_SAVE
AC_LANG_C
SAVE_CFLAGS="$CFLAGS"
CFLAGS="$CFLAGS -Wall -Werror -msse4.1"
AC_TRY_LINK([#include <stdint.h>
#include <math.h>
#include <xmmintrin.h>
#include <emmintrin.h>
#include <smmintrin.h>
],[
__m128 a = _mm_set1_ps(1.0f);
__m128 b = _mm_set1_ps(2.0f);
a = _mm_blend_ps(a,b,0x0c);
return 0;
],
HAVE_SSE41="yes",
want_sse41="no (missing -march ?)"
)
CFLAGS="$SAVE_CFLAGS"
AC_LANG_RESTORE
AC_MSG_RESULT([$want_sse41])
fi
AC_SUBST(HAVE_SSE41)


AC_CONFIG_FILES([yate-bts.spec
                 Makefile
                 mbts/A53/Makefile:mbts/Makefile.head.in:mbts/A53/Makefile.in:mbts/Makefile.tail.in
                 mbts/CLI/Makefile:mbts/Makefile.head.in:mbts/CLI/Makefile.in:mbts/Makefile.tail.in
                 mbts/CommonLibs/Makefile:mbts/Makefile.head.in:mbts/CommonLibs/Makefile.in:mbts/Makefile.tail.in
                 mbts/Connection/Makefile:mbts/Makefile.head.in:mbts/Connection/Makefile.in:mbts/Makefile.tail.in
                 mbts/Control/Makefile:mbts/Makefile.head.in:mbts/Control/Makefile.in:mbts/Makefile.tail.in
                 mbts/GPRS/Makefile:mbts/Makefile.head.in:mbts/GPRS/Makefile.in:mbts/Makefile.tail.in
                 mbts/GSM/Makefile:mbts/Makefile.head.in:mbts/GSM/Makefile.in:mbts/Makefile.tail.in
                 mbts/Globals/Makefile:mbts/Makefile.head.in:mbts/Globals/Makefile.in:mbts/Makefile.tail.in
                 mbts/Peering/Makefile:mbts/Makefile.head.in:mbts/Peering/Makefile.in:mbts/Makefile.tail.in
                 mbts/SGSNGGSN/Makefile:mbts/Makefile.head.in:mbts/SGSNGGSN/Makefile.in:mbts/Makefile.tail.in
                 mbts/TRXManager/Makefile:mbts/Makefile.head.in:mbts/TRXManager/Makefile.in:mbts/Makefile.tail.in
                 mbts/sqlite3/Makefile:mbts/Makefile.head.in:mbts/sqlite3/Makefile.in:mbts/Makefile.tail.in
                 mbts/apps/Makefile:mbts/Makefile.head.in:mbts/apps/Makefile.in:mbts/Makefile.tail.in
                 transceiver/Makefile
                 roaming/Makefile
                 nib/Makefile
                 nib/auth/Makefile])
CONFIGURE_FILES=`echo "$ac_config_files config.h config.status config.log" | sed 's/yate-[[^ ]]*\.spec *//; s/:[[^ ]]\+//g'`
AC_SUBST(CONFIGURE_FILES)
AC_OUTPUT
