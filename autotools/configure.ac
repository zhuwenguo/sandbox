AC_INIT([test], [1.0])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_FILES([Makefile src/Makefile])
AC_CONFIG_HEADER(config.h)
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_SRCDIR([src/test.cxx])
AM_INIT_AUTOMAKE([foreign -Wall -Werror])
AC_PROG_CC
AC_PROG_CXX

AC_ARG_ENABLE(mpi, [AC_HELP_STRING([--enable-mpi],[compile MPI library])], enable_mpi=$enableval, enable_mpi=no)
if test "$enable_mpi" = "yes"; then
   ACX_MPI([],[AC_MSG_ERROR([could not find mpi library for --enable-mpi])])
   AC_CHECK_PROG(MPIRUN, mpirun, mpirun)
   AC_SUBST(MPIRUN)
   CXX=$MPICXX
fi
AM_CONDITIONAL(MPI, test "$enable_mpi" = "yes")

AX_CXX_MAXOPT
AC_OUTPUT
