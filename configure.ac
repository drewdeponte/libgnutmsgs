# Must contain a call to AC_INIT before the checks, and call to
# AC_OUTPUT at the end.
#
# The layout of standard autoconf.ac
#
# AC_INIT(package, version, bug-report-address)
# information on the package
# checks for programs
# checks for libraries
# checks for header files
# checks for types
# checks for structures
# checks for compiler characteristics
# checks for library functions
# checks for system services
# AC_CONFIG_FILES([file...])
# AC_OUTPUT
#

AC_PREREQ(2.59)
AC_INIT(Gnutella Messages Library, 0.1.0, cyphactor@socal.rr.com, lib_gnut_msgs)
AM_INIT_AUTOMAKE
AC_CONFIG_SRCDIR([src/gnut_msgs.c])
AC_CONFIG_HEADER([src/config.h])

# checks for programs
AC_PROG_LIBTOOL
AC_PROG_CC

# checks for libraries

# checks for header files
AC_HEADER_STDC
AC_CHECK_HEADERS([arpa/inet.h netinet/in.h string.h sys/socket.h stdint.h])

# checks for types

# checks for structures

# checks for compiler characteristics
AC_C_BIGENDIAN

# checks for library functions
AC_CHECK_FUNCS([memset socket])

# checks for system services

AC_CONFIG_FILES([Makefile src/Makefile testing/Makefile])
AC_OUTPUT
