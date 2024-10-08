#! /bin/sh
# Tests for ncgen

srcdir=.

NCDUMP='../ncdump/ncdump'

RM='rm -f'
CMP='cmp -s'
DIFF='diff -c'
CP='cp'

nerrors=0
verbose=yes

# Print message with formats according to message level ($1)
MESG() {
  level=$1
  shift
  case $level in
    0)
      echo '============================='
      echo $*
      echo '============================='
      ;;
    3)
      echo '-----------------------------'
      echo $*
      echo '-----------------------------'
      ;;
    6)
      echo "*** $* ***"
      ;;
    *)
      echo "MESG(): Unknown level ($level)"
      exit 1
      ;;
  esac
}


# Report the result and exit
FINISH()
{
    if [ $nerrors -eq 0 ]
    then
        MESG 0 "All ncgen tests passed"
    else
        MESG 0 "ncgen tests failed: $nerrors"
    fi
    exit $nerrors
}

# Print a line-line message left justified in a field of 70 characters
# beginning with the word "Testing".
#
TESTING() {
   SPACES="                                                               "
   echo "Testing $* $SPACES" | cut -c1-70 | tr -d '\012'
}

bcheck() {
   ## Test the "-b" option of ncgen
   ${TESTS_ENVIRONMENT} ./ncgen -b test1.cdl
   ${TESTS_ENVIRONMENT} $NCDUMP test1.nc > test2.cdl
   $DIFF test1.cdl test2.cdl
   RET=$?
   if [ $RET -eq 0 ]; then
      echo "*** ncgen -b test successful ***"
   else 
      echo "*** ncgen -b test failed ***"
      nerrors="`expr $nerrors + 1`"
   fi
}

ccheck() {
   ## Test the "-c" option of ncgen
   ${TESTS_ENVIRONMENT} ./ctest0        # tests `-c' option, creates ctest0.nc
   ${TESTS_ENVIRONMENT} $NCDUMP -n test1 ctest0.nc > ctest1.cdl
   $DIFF test1.cdl ctest1.cdl
   RET=$?
   if [ $RET -eq 0 ]; then
      echo "*** ncgen -c test successful ***"
   else
      echo "*** ncgen -c test failed  ***"
      nerrors="`expr $nerrors + 1`"
   fi
}

fcheck() {
   ## Test the "-f" option of ncgen
   ${TESTS_ENVIRONMENT} ./ftest0
   ${TESTS_ENVIRONMENT} $NCDUMP -n test1 ftest0.nc > ftest1.cdl
   $DIFF test1.cdl ftest1.cdl
   RET=$?
   if [ $RET -eq 0 ]; then
      echo "*** ncgen -f test successful ***"
   else
      echo "*** ncgen -f test failed (but roundoff differences are OK) ***"
      nerrors="`expr $nerrors + 1`"
   fi
}

# Run a test and print PASS or *FAIL*.  If a test fails then increment
# the `nerrors' global variable
#
RUN() {
   # create test files
   ${TESTS_ENVIRONMENT} ./ncgen -b -o test0.nc $srcdir/test0.cdl
   ${TESTS_ENVIRONMENT} $NCDUMP -n test1 test0.nc > test1.cdl

   # Run test.
   if [ "$HDF_BUILD_NETCDF" = "0" ]; then
      if [ "$HDF_BUILD_FORTRAN" = "0" ]; then
         bcheck
         ccheck
         fcheck
      else
         bcheck
         ccheck
      fi
   else
      bcheck
      ccheck
   fi
}

##############################################################################
###                       T H E   T E S T S                                ###
##############################################################################

# Print a beginning banner
MESG 0 "Running ncgen tests"

# compare output
RUN

# End of test, return exit code
FINISH
