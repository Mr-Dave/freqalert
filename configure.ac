# Process this file with autoconf to produce a configure script

AC_INIT(freqalert, esyscmd(['./version.sh']))
AC_GNU_SOURCE
AC_CONFIG_SRCDIR([freqalert.c])
AC_CONFIG_HEADERS(config.h)
AC_PROG_CC
AC_HEADER_STDC
AC_C_CONST

###############################################################################
###  Host system
###############################################################################
DISTRO="Linux"
AC_SUBST(DISTRO)

###############################################################################
###  Host specific paths
###############################################################################
TEMP_LIBS="-lm -lasound -lfftw3 ${TEMP_LIBS}"
TEMP_CFLAGS="${TEMP_CFLAGS} ${CFLAGS}"
TEMP_CPPFLAGS="${TEMP_CPPFLAGS} ${CPPFLAGS}"
TEMP_LDFLAGS="${TEMP_LDFLAGS} ${LDFLAGS}"

AC_SUBST(CFLAGS, "${TEMP_CFLAGS}")
AC_SUBST(CPPFLAGS, "${TEMP_CPPFLAGS}")
AC_SUBST(LDFLAGS, "${TEMP_LDFLAGS}")

##############################################################################
###  Check for threading
##############################################################################
  THREADS="yes"
  TEMP_LIBS="$TEMP_LIBS -pthread"
  TEMP_CFLAGS="${TEMP_CFLAGS} -D_THREAD_SAFE"

##############################################################################
###  Developer Flags
##############################################################################
  TEMP_CFLAGS="${TEMP_CFLAGS} -W -Werror -Wall -Wextra -Wformat -Wshadow -Wpointer-arith -Wwrite-strings -Waggregate-return -Wstrict-prototypes -Wmissing-prototypes -Wnested-externs -Winline -Wredundant-decls -Wno-long-long -ggdb -g3"

CFLAGS="${TEMP_CFLAGS} $CPU_OPTIONS"
LIBS="${TEMP_LIBS}"
LDFLAGS="${TEMP_LDFLAGS}"

##############################################################################
###  exec paths
##############################################################################
if test $prefix = "NONE";then
    BIN_PATH="$ac_default_prefix"
    if test $exec_prefix = "NONE"; then
        BIN_PATH="$BIN_PATH/bin"
    else
        BIN_PATH="$BIN_PATH/$bindir"
    fi
else
    if test $exec_prefix = "NONE";then
        BIN_PATH="$prefix/bin"
    else
        BIN_PATH="$prefix/$bindir"
    fi
fi
AC_SUBST(BIN_PATH)

AC_CHECK_HEADERS(stdio.h unistd.h stdint.h fcntl.h time.h signal.h sys/types.h alsa/asoundlib.h fftw3.h)

AC_CONFIG_FILES([
freqalert.conf
Makefile
])
AC_OUTPUT

##############################################################################
###  Report results to user
##############################################################################
echo ""
echo "   **************************"
echo "   Configure status       "
echo "   ${PACKAGE_NAME} ${PACKAGE_VERSION}"
echo "   **************************"
echo

if test "${THREADS}" = "yes"; then
    echo "pthread support:     Yes"
else
    echo "pthread support:     No"
    echo "**********************************************"
    echo "** Fatal Error YOU MUST HAVE pthread Support *"
    echo "**********************************************"
fi

echo
echo "CFLAGS: $CFLAGS"
echo "LIBS: $LIBS"
echo "LDFLAGS: $LDFLAGS"
echo
echo  "Install prefix:       $prefix"
echo
