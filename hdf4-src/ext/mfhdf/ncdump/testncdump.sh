#! /bin/sh
# Tests for the ncdump tool

srcdir=.
currentdir=`pwd`

# Definitions of commands and variables
NCDUMP='ncdump'                  # The tool name
NCDUMP_BIN="${TESTS_ENVIRONMENT} "`pwd`/$NCDUMP       # The path of the tool binary

RM='rm -f'
CMP='cmp -s'
DIFF='diff -c'
CP='cp'

nerrors=0
verbose=yes

$CP $srcdir/test0.cdl ./nctest0.cdl

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
        MESG 0 "All ncdump tests passed"
    else
        MESG 0 "ncdump tests failed: $nerrors"
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

# Run a test and print PASS or *FAIL*.  If a test fails then increment
# the `nerrors' global variable
#
# XXX: This test is almost worthless since ncdump always returns EXIT_SUCCESS.
#
RUN() {
   # Run test.

   ${TESTS_ENVIRONMENT} ../ncgen/ncgen -o test0.nc -n nctest0.cdl
   RET=$?
   if [ $RET -eq 1 ]; then
      echo "*FAILED*"
      nerrors="`expr $nerrors + 1`"
   fi

   ${TESTS_ENVIRONMENT} $NCDUMP_BIN test0.nc > test1.cdl
   RET=$?
   if [ $RET -eq 1 ]; then
      echo "*FAILED*"
      nerrors="`expr $nerrors + 1`"
   fi

   ${TESTS_ENVIRONMENT} ../ncgen/ncgen -o test1.nc -n test1.cdl
   RET=$?
   if [ $RET -eq 1 ]; then
      echo "*FAILED*"
      nerrors="`expr $nerrors + 1`"
   fi

   ${TESTS_ENVIRONMENT} $NCDUMP_BIN -n test0 test1.nc > test2.cdl
   RET=$?
   if [ $RET -eq 1 ]; then
      echo "*FAILED*"
      nerrors="`expr $nerrors + 1`"
   fi

   $CMP test1.cdl test2.cdl
   cmpval=$?
   if [ "$cmpval" = 0 ] ; then
       echo " PASSED"
   else
       echo "*FAILED*"
       nerrors="`expr $nerrors + 1`"
   fi
   $RM -f test1.nc test1.cdl test2.cdl
}

##############################################################################
###                       T H E   T E S T S                                ###
##############################################################################

# Print a beginning banner
MESG 0 "Running ncdump tests"

# compare output
RUN

# End of test, return exit code
FINISH
    
