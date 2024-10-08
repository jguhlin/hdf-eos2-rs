#!/bin/sh 
# HDF Utilities Test script
# Usage: testutil.sh [machine-type]

srcdir=.

machinetype="$1"

# Check if target machine supports 32 bits datatype.
# "true" is TRUE; "" means FALSE.
case $machinetype in
    *unicos*) has32="";;
    *) has32="true";;
esac

# initialize errors variable
errors=0
haserr=0

# setup hdfed command which is used often
HDFED='../../hdf/util/hdfed'
HDFLS="${TESTS_ENVIRONMENT} ../../hdf/util/hdfls"
HDFEDCMD="${TESTS_ENVIRONMENT} $HDFED -batch"		# use -batch mode for no prompt
SED="sed -e /library/,/String/d"  # filter out the library version
TESTCMD="${TESTS_ENVIRONMENT} ./hdfimport"    # The test command

echo ""
echo "=============================="
echo "HDFIMPORT tests started"
echo "=============================="

if [ -f hdfimport -a -f hdfimporttest ]; then
echo "** Testing hdfimport  ***"

# use [!S] to prevent SDSfloat2.hdf and SDSfloat3.hdf from being deleted;
# please replace with better solution, one is if available. - BMR (2006/9/23)
/bin/rm -f ctxt* cb* [!S]*.hdf *.out hdfls.tmp5 hdfed.tmp6 hdffiles.tmp
${TESTS_ENVIRONMENT} ./hdfimporttest
echo "Testing for 32-bit floating point ASCII (2D data)" 
$TESTCMD ctxtr2 -o ctxtr2.hdf
echo "Testing for 32-bit floating point ASCII (3D data)" 
$TESTCMD ctxtr3 -o ctxtr3.hdf
echo "Testing for 32-bit integer binary (2D data)" 
$TESTCMD cb32i2 -o cb32i2.hdf
echo "Testing for 32-bit integer  binary (3D data)" 
$TESTCMD cb32i3 -o cb32i3.hdf
echo "Testing for 16-bit integer binary (2D data)" 
$TESTCMD cb16i2 -o cb16i2.hdf
echo "Testing for 16-bit integer (3D data)" 
$TESTCMD cb16i3 -o cb16i3.hdf
echo "Testing for 32-bit floating point binary (2D data)" 
$TESTCMD cb32r2 -o cb32r2.hdf
echo "Testing for 32-bit floating point binary (3D data)" 
$TESTCMD cb32r3 -o cb32r3.hdf
echo "Testing for 64-bit floating point binary (2D data) - Default Behaviour (Conversion to 32 bit FP SDS)" 
$TESTCMD cb64r2 -o cb64r2.hdf
echo "Testing for 64-bit floating point binary (3D data) - Default Behaviour (Conversion to 32-bit FP SDS)" 
$TESTCMD cb64r3 -o cb64r3.hdf
echo "Testing for 64-bit floating point binary (2D data) - Conversion to 64-bit FP SDS" 
$TESTCMD cb64r2 -n -o cb64r2-n.hdf
echo "Testing for 64-bit floating point binary (3D data) - Conversion to 64-bit FP SDS" 
$TESTCMD cb64r3 -n -o cb64r3-n.hdf
echo "Testing for raster options" 
$TESTCMD ctxtr2 -o ctxtr2_ris.hdf -raster -e 50 50
$TESTCMD cb64r2 -o cb64r2_ris.hdf -raster -i 50 50 -f

# test with hdf files
echo "Testing for reading from hdf files" 
$TESTCMD $srcdir/SDSfloat2.hdf -o SDSfloat2.out
$TESTCMD $srcdir/SDSfloat3.hdf -o SDSfloat3.out

($HDFLS -l ctxtr2.hdf | $SED) > hdfls.tmp5 2>&1
($HDFLS -l ctxtr3.hdf | $SED) >> hdfls.tmp5 2>&1
($HDFLS -l cb32i2.hdf | $SED) >> hdfls.tmp5 2>&1
($HDFLS -l cb32i3.hdf | $SED) >> hdfls.tmp5 2>&1
($HDFLS -l cb16i2.hdf | $SED) >> hdfls.tmp5 2>&1
($HDFLS -l cb16i3.hdf | $SED) >> hdfls.tmp5 2>&1
($HDFLS -l cb32r2.hdf | $SED) >> hdfls.tmp5 2>&1
($HDFLS -l cb32r3.hdf | $SED) >> hdfls.tmp5 2>&1
($HDFLS -l cb64r2.hdf | $SED) >> hdfls.tmp5 2>&1
($HDFLS -l cb64r3.hdf | $SED) >> hdfls.tmp5 2>&1
($HDFLS -l cb64r2-n.hdf | $SED) >> hdfls.tmp5 2>&1
($HDFLS -l cb64r3-n.hdf | $SED) >> hdfls.tmp5 2>&1
($HDFLS -l ctxtr2_ris.hdf | $SED) >> hdfls.tmp5 2>&1
($HDFLS -l cb64r2_ris.hdf | $SED) >> hdfls.tmp5 2>&1

# run hdfls on SDSfloat*.out, and remove the library version from the
# output for later checking against original output - BMR (2006/9/7)
($HDFLS SDSfloat2.out | $SED) >> hdffiles.tmp 2>&1
($HDFLS SDSfloat3.out | $SED) >> hdffiles.tmp 2>&1

    diff -b   hdfls.tmp5 $srcdir/hdfimport.out1 || errors=1
    $HDFEDCMD < $srcdir/hdfimport.input1 > hdfed.tmp6 2>&1
    diff -b   hdfed.tmp6 $srcdir/hdfimport.out2 || errors=1

    # hdfimport.out3 contains output that hdfls previously generated
    # from SDSfloat*.out with the library version removed - BMR (2006/9/7)
    diff -b   hdffiles.tmp $srcdir/hdfimport.out3 || errors=1

# use [!S] to prevent SDSfloat2.hdf and SDSfloat3.hdf from being deleted;
# please replace with a better solution, if one is available. - BMR (2006/9/23)
/bin/rm -f ctxt* cb* [!S]*.hdf *.out hdfls.tmp5 hdfed.tmp6 hdffiles.tmp
#/bin/rm -f ctxt* cb* *.hdf 
else
haserr=1
echo "** hdfimport or hdfimporttest not available ***"
fi

if [ $errors -eq 1 ]; then
  haserr=1
  echo " ********* NOTE ***************"
  echo " hdfimport might have failed ***"
  echo " The above errors could be formatting "
  echo " problems which can be ignored "
  echo " please run the following by hand to verify "
  echo " "
  echo "  /bin/rm -f ctxtr* cb* *.hdf hdfls.tmp5 hdfed.tmp6"
  echo " ${TESTS_ENVIRONMENT} ./hdfimporttest "
  echo " $TESTCMD ctxtr2 -o ctxtr2.hdf "
  echo " $TESTCMD ctxtr3 -o ctxtr3.hdf "
  echo " $TESTCMD cb32i2 -o cb32i2.hdf "
  echo " $TESTCMD cb32i3 -o cb32i3.hdf "
  echo " $TESTCMD cb16i2 -o cb16i2.hdf "
  echo " $TESTCMD cb16i3 -o cb16i3.hdf "
  echo " $TESTCMD cb32r2 -o cb32r2.hdf "
  echo " $TESTCMD cb32r3 -o cb32r3.hdf "
  echo " $TESTCMD cb64r2 -o cb64r2.hdf "
  echo " $TESTCMD cb64r3 -o cb64r3.hdf "
  echo " $TESTCMD cb64r2 -n -o cb64r2-n.hdf "
  echo " $TESTCMD cb64r3 -n -o cb64r3-n.hdf "
  echo " $TESTCMD ctxtr2 -o ctxtr2_ris.hdf -raster -e 50 50 "
  echo " $TESTCMD cb64r2 -o cb64r2_ris.hdf -raster -i 50 50 -f "
  echo " $TESTCMD SDSfloat2.hdf -o SDSfloat2.out "
  echo " $TESTCMD SDSfloat3.hdf -o SDSfloat3.out "
  echo "($HDFLS -l ctxtr2.hdf | $SED) >&  hdfls.tmp5 "
  echo "($HDFLS -l ctxtr3.hdf | $SED) >>& hdfls.tmp5 "
  echo "($HDFLS -l cb32i2.hdf | $SED) >> hdfls.tmp5 2>&1 "
  echo "($HDFLS -l cb32i3.hdf | $SED) >> hdfls.tmp5 2>&1 "
  echo "($HDFLS -l cb16i2.hdf | $SED) >> hdfls.tmp5 2>&1 "
  echo "($HDFLS -l cb16i3.hdf | $SED) >> hdfls.tmp5 2>&1 "
  echo "($HDFLS -l cb32r2.hdf | $SED) >>& hdfls.tmp5 "
  echo "($HDFLS -l cb32r3.hdf | $SED) >>& hdfls.tmp5 "
  echo "($HDFLS -l cb64r2.hdf | $SED) >>& hdfls.tmp5 "
  echo "($HDFLS -l cb64r3.hdf | $SED) >>& hdfls.tmp5 "
  echo "($HDFLS -l cb64r2-n.hdf | $SED) >> hdfls.tmp5 2>&1 "
  echo "($HDFLS -l cb64r3-n.hdf | $SED) >> hdfls.tmp5 2>&1 "
  echo "($HDFLS -l ctxtr2_ris.hdf | $SED) >>& hdfls.tmp5 "
  echo "($HDFLS -l cb64r2_ris.hdf | $SED) >>& hdfls.tmp5 "
  echo "($HDFLS SDSfloat2.out | $SED) >> hdffiles.tmp 2>&1 "
  echo "($HDFLS SDSfloat3.out | $SED) >> hdffiles.tmp 2>&1 "
  echo " diff hdfls.tmp5 hdfimport.out1 "
  echo " $HDFEDCMD < hdfimport.input1 >& hdfed.tmp6 "
  echo " diff hdfed.tmp6 hdfimport.out2 "
  echo " diff  hdffiles.tmp hdfimport.out3 || errors=1 "
  echo " ******* END NOTE *************"
  echo ""
  errors=0
fi

#
# Check errors result
if [ $haserr -eq 0 ]; then
    echo "================================="
    echo "HDFIMPORT Utilities tests passed."
    echo "================================="
else
    echo "*********************************************"
    echo "HDFIMPORT Utilities tests encountered errors"
    echo "*********************************************"
fi
echo ""
exit $haserr
