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
HDFED='./hdfed'
HDFEDCMD="$HDFED -batch"		# use -batch mode for no prompt
SED="sed -e /library/,/String/d"  # filter out the library version

echo ""
echo "=============================="
echo "HDF Utilities tests started"
echo "=============================="

# gif2hdf and hdf2gif 
if [ -f gif2hdf -a -f hdf2gif ]; then
  echo "** Testing gif2hdf and hdf2gif  ***"
  /bin/rm -rf gif_hdf_temp
  /bin/mkdir gif_hdf_temp
  $TESTS_ENVIRONMENT ./hdf2gif $srcdir/testfiles/skull.hdf  gif_hdf_temp/skull.gif
  $TESTS_ENVIRONMENT ./gif2hdf $srcdir/testfiles/SunWheel.gif  gif_hdf_temp/SunWheel.hdf
  $TESTS_ENVIRONMENT ./gif2hdf $srcdir/testfiles/bttrfly.gif  gif_hdf_temp/bttrfly.hdf
else
  echo "** hdf2gif or gif2hdf is not available ***";
fi

echo " *********************** NOTE ****************************"
echo " Use WEB Browser and JHV to compare "
echo " $srcdir/testfiles/SunWheel.gif and $srcdir/testfiles/bttrfly.gif "
echo " with gif_hdf_temp/SunWheel.hdf and gif_hdf_temp/bttrfly.hdf"
echo " "
echo " Use JHV  and WEB Browser to compare "
echo " $srcdir/testfiles/skull.hdf and gif_hdf_temp/skull.gif"
echo " ********************* END NOTE **************************"
echo ""

errors=0

# hdf24to8
# "uncompress" utiltiy became obsolete; replaced with gunzip EIP 2004/12/01
# gunzip not available on Windows; simply untarring files SJW 2007/7/31
if [ -f hdf24to8 -a -f hdftor8 ]; then
  echo "** Testing hdf24to8  ***"
  /bin/rm -f head.r24 head.r8 head8.hdf img001-263.328 pal001
  #cp $srcdir/testfiles/head.r24.Z $srcdir/testfiles/head.r8.Z .
  cp $srcdir/testfiles/head.r24 $srcdir/testfiles/head.r8 .
  $TESTS_ENVIRONMENT ./hdf24to8 head.r24 head8.hdf
  $TESTS_ENVIRONMENT ./hdftor8 head8.hdf
  cmp img001-263.328 head.r8 || errors=1
  /bin/rm -f head.r24 head.r8 head8.hdf img001-263.328 pal001
else
  echo "** hdf24to8 or hdftor8 is not available ***"
fi

if [ $errors -eq 1 ]; then
  haserr=1
  echo " ********* NOTE ***************"
  echo " hdf24to8 might have failed ***"
  echo " The above errors could be formatting "
  echo " problems which can be ignored "
  echo " please run the following by hand to verify "
  echo " "
  echo " /bin/rm -f head.r24 head.r8 head8.hdf img001-263.328 pal001"
  echo " cp $srcdir/testfiles/head.r24 $srcdir/testfiles/head.r8 . "
  echo " ./hdf24to8 head.r24 head8.hdf "
  echo " ./hdftor8 head8.hdf "
  echo " cmp img001-263.328 head.r8 " 
  echo " ******* END NOTE *************"
  echo ""
  errors=0
fi

# hdfed
if [ -f $HDFED ]; then
  echo "** Testing hdfed  ***"
  /bin/rm -f storm110.hdf ntcheck.hdf hdfed.tmp
  cp $srcdir/testfiles/storm110.hdf $srcdir/testfiles/ntcheck.hdf .
  $TESTS_ENVIRONMENT $HDFEDCMD < $srcdir/hdfed.input1 > hdfed.tmp 2>&1
  diff -b hdfed.tmp $srcdir/hdfed.out1 || errors=1
  /bin/rm -f storm110.hdf ntcheck.hdf hdfed.tmp
else
  echo "** hdfed not available ***"
fi

if [ $errors -eq 1 ]; then
  haserr=1
  echo " ********* NOTE ***************"
  echo " hdfed might have failed ***"
  echo " The above errors could be formatting "
  echo " problems which can be ignored "
  echo " please run the following by hand to verify "
  echo " "
  echo "/bin/rm -f storm110.hdf ntcheck.hdf hdfed.tmp "
  echo "cp $srcdir/testfiles/storm110.hdf $srcdir/testfiles/ntcheck.hdf . "
  echo "$HDFEDCMD < $srcdir/hdfed.input1 >& hdfed.tmp "
  echo "diff hdfed.tmp $srcdir/hdfed.out1 "
  echo " ******* END NOTE *************"
  echo ""
  errors=0
fi

# ristosds
if [ -f ristosds -a -f $HDFED ]; then
  echo "** Testing ristosds  ***"
  /bin/rm -f storm*.hdf hdfed.tmp1
  cp $srcdir/testfiles/storm110.hdf $srcdir/testfiles/storm120.hdf $srcdir/testfiles/storm130.hdf .
  $TESTS_ENVIRONMENT ./ristosds storm*.hdf -o storm.hdf > /dev/null 2>&1
  $TESTS_ENVIRONMENT $HDFEDCMD < $srcdir/ristosds.input1 > hdfed.tmp1 2>&1
  diff -b hdfed.tmp1 $srcdir/ristosds.out1 || errors=1
  /bin/rm -f storm*.hdf hdfed.tmp1
else
  echo "** ristosds or hdfed not available ***"
fi

if [ $errors -eq 1 ]; then
  haserr=1
  echo " ********* NOTE ***************"
  echo " ristosds might have failed ***"
  echo " The above errors could be formatting "
  echo " problems which can be ignored "
  echo " please run the following by hand to verify "
  echo " "
  echo " /bin/rm -f storm*.hdf hdfed.tmp1 "
  echo " cp $srcdir/testfiles/storm110.hdf $srcdir/testfiles/storm120.hdf $srcdir/testfiles/storm130.hdf . "
  echo "./ristosds storm*.hdf -o storm.hdf "
  echo "$HDFEDCMD < $srcdir/ristosds.input1 >& hdfed.tmp1 "
  echo " diff  hdfed.tmp1 $srcdir/ristosds.out1"
  echo " ******* END NOTE *************"
  echo ""
  errors=0
fi


#hdfpack
if [ -f hdfpack -a -f hdfls ]; then
  echo "** Testing hdfpack  ***"
  /bin/rm -f test.hdf test.blk test.pck hdfls.tmp1
  cp $srcdir/testfiles/test.hdf .
  $TESTS_ENVIRONMENT ./hdfpack test.hdf test.pck
  $TESTS_ENVIRONMENT ./hdfpack -b test.hdf test.blk
  ($TESTS_ENVIRONMENT ./hdfls test.hdf | $SED ) > hdfls.tmp1 2>&1
  ($TESTS_ENVIRONMENT ./hdfls test.pck | $SED ) >> hdfls.tmp1 2>&1
  diff -b hdfls.tmp1 $srcdir/hdfpack.out1 || errors=1
  /bin/rm -f test.hdf test.blk test.pck hdfls.tmp1
else
  echo "** hdfpack or hdfls not available ***"
fi

if [ $errors -eq 1 ]; then
  haserr=1
  echo " ********* NOTE ***************"
  echo " hdfpack might have failed ***"
  echo " The above errors could be formatting "
  echo " problems which can be ignored "
  echo " please run the following by hand to verify "
  echo " "
  echo "/bin/rm -f test.hdf test.blk test.pck hdfls.tmp1 "
  echo " cp $srcdir/testfiles/test.hdf . "
  echo "./hdfpack test.hdf test.pck "
  echo "./hdfpack -b test.hdf test.blk "
  echo "(./hdfls test.hdf | $SED ) >& hdfls.tmp1"
  echo "(./hdfls test.pck | $SED ) >>& hdfls.tmp1"
  echo " diff hdfls.tmp1 $srcdir/hdfpack.out1 "
  echo " ******* END NOTE *************"
  echo ""
  errors=0
fi

#hdftopal/paltohdf
if [ -f  hdftopal -a -f paltohdf ]; then
  echo "** Testing hdftopal/paltohdf  ***"
  /bin/rm -f palette.* 
  cp $srcdir/testfiles/palette.raw .
  $TESTS_ENVIRONMENT ./paltohdf palette.raw palette.hdf
  $TESTS_ENVIRONMENT ./hdftopal palette.hdf palette.raw.new
  cmp palette.raw palette.raw.new || errors=1
  /bin/rm -f palette.* 
else
  echo "** hdftopal or paltohdf not available ***"
fi

if [ $errors -eq 1 ]; then
  haserr=1
  echo " ********* NOTE ***************"
  echo " hdftopal or paltohdf might have failed ***"
  echo " The above errors could be formatting "
  echo " problems which can be ignored "
  echo " please run the following by hand to verify "
  echo " "
  echo " /bin/rm -f palette.* "
  echo " cp $srcdir/testfiles/palette.raw . "
  echo "./paltohdf palette.raw palette.hdf "
  echo "./hdftopal palette.hdf palette.raw.new "
  echo "cmp palette.raw palette.raw.new "
  echo " ******* END NOTE *************"
  echo ""
  errors=0
fi

# r8tohdf/hdftor8
if [ -f r8tohdf -a -f hdftor8 -a -f hdfls ]; then
  echo "** Testing r8tohdf/hdftor8  ***"
  /bin/rm -f storm* img* palette.raw hdfls.tmp2 pal005
  cp $srcdir/testfiles/storm*.raw $srcdir/testfiles/palette.raw .
  $TESTS_ENVIRONMENT ./r8tohdf 57 57 storm.hdf storm*.raw
  $TESTS_ENVIRONMENT ./r8tohdf 57 57 storm.hdf -p palette.raw -i storm110.raw
  $TESTS_ENVIRONMENT ./hdftor8 storm.hdf
  ($TESTS_ENVIRONMENT ./hdfls -l storm.hdf | $SED) > hdfls.tmp2 2>&1
  diff -b hdfls.tmp2 $srcdir/hdftor8.out1 || errors=1
  cmp img001-057.057  storm110.raw || errors=1
  cmp img002-057.057  storm120.raw || errors=1
  cmp img003-057.057  storm130.raw || errors=1
  cmp img004-057.057  storm140.raw || errors=1
  /bin/rm -f storm* img* palette.raw hdfls.tmp2 pal005
else
  echo "** r8tohdf, hdftor8 or hdfls not available ***"
fi

if [ $errors -eq 1 ]; then
  haserr=1
  echo " ********* NOTE ***************"
  echo " r8tohdf or hdftor8 might have failed ***"
  echo " The above errors could be formatting "
  echo " problems which can be ignored "
  echo " please run the following by hand to verify "
  echo " "
  echo "/bin/rm -f storm* img* palette.raw hdfls.tmp2 pal005 "
  echo "cp $srcdir/testfiles/storm*.raw $srcdir/testfiles/palette.raw . "
  echo "./r8tohdf 57 57 storm.hdf storm*.raw "
  echo "./r8tohdf 57 57 storm.hdf -p palette.raw -i storm110.raw "
  echo "./hdftor8 storm.hdf "
  echo "(./hdfls -l storm.hdf | $SED) >& hdfls.tmp2 "
  echo " diff hdfls.tmp2 $srcdir/hdftor8.out1 "
  echo "cmp img001-057.057  storm110.raw "
  echo "cmp img002-057.057  storm120.raw "
  echo "cmp img003-057.057  storm130.raw "
  echo "cmp img004-057.057  storm140.raw "
  echo " ******* END NOTE *************"
  echo ""
 errors=0
fi

# hdfcomp
if [ -f  hdfcomp -a -f hdfls ]; then
  echo "** Testing hdfcomp  ***"
  /bin/rm -f storm*.hdf all*.hdf hdfls.tmp3
  cp $srcdir/testfiles/storm*.hdf .
  $TESTS_ENVIRONMENT ./hdfcomp allstorms.hdf storm*.hdf
  $TESTS_ENVIRONMENT ./hdfcomp allcomp.hdf -c storm*.hdf
  ($TESTS_ENVIRONMENT ./hdfls -l allstorms.hdf | $SED) > hdfls.tmp3 2>&1
  ($TESTS_ENVIRONMENT ./hdfls -l allcomp.hdf | $SED) >> hdfls.tmp3 2>&1
  diff -b   hdfls.tmp3 $srcdir/hdfcomp.out1 || errors=1
  /bin/rm -f storm*.hdf all*.hdf hdfls.tmp3
else
  echo "** hdfcomp or hdfls not available ***"
fi

if [ $errors -eq 1 ]; then
  haserr=1
  echo " ********* NOTE ***************"
  echo " hdfcomp might have failed ***"
  echo " The above errors could be formatting "
  echo " problems which can be ignored "
  echo " please run the following by hand to verify "
  echo " "
  echo "/bin/rm -f storm*.hdf all*.hdf hdfls.tmp3 "
  echo "cp $srcdir/testfiles/storm*.hdf . "
  echo "./hdfcomp allstorms.hdf storm*.hdf "
  echo "./hdfcomp allcomp.hdf -c storm*.hdf "
  echo "(./hdfls -l allstorms.hdf | $SED) >& hdfls.tmp3 "
  echo "(./hdfls -l allcomp.hdf | $SED) >>& hdfls.tmp3 "
  echo "diff hdfls.tmp3 $srcdir/hdfcomp.out1 "
  echo " ******* END NOTE *************"
  echo ""
  errors=0
fi

#jpeg2hdf/hdf2jpeg
if [ -f jpeg2hdf -a -f hdf2jpeg -a -f hdfls ]; then
  echo "** Testing jpeg2hdf/hdf2jpeg  ***"
  /bin/rm -f jpeg.hdf jpeg_img.jpg jpeg2.jpg hdfls.tmp4
  cp $srcdir/testfiles/jpeg_img.jpg .
  $TESTS_ENVIRONMENT ./jpeg2hdf jpeg_img.jpg jpeg.hdf
  $TESTS_ENVIRONMENT ./hdf2jpeg jpeg.hdf jpeg2.jpg
  ($TESTS_ENVIRONMENT ./hdfls -l jpeg.hdf | $SED) > hdfls.tmp4 2>&1
  diff -b   hdfls.tmp4 $srcdir/jpeg2hdf.out1 || errors=1
  cmp jpeg_img.jpg jpeg2.jpg || errors=1
  /bin/rm -f jpeg.hdf jpeg_img.jpg jpeg2.jpg hdfls.tmp4
else
  echo "** jpeg2hdf, hdf2jpeg or hdfls  not available ***"
fi

if [ $errors -eq 1 ]; then
  haserr=1
  echo " ********* NOTE ***************"
  echo " jpeg2hdf or hdf2jpeg might have failed ***"
  echo " The above errors could be formatting "
  echo " problems which can be ignored "
  echo " please run the following by hand to verify "
  echo " "
  echo "/bin/rm -f jpeg.hdf jpeg_img.jpg jpeg2.jpg hdfls.tmp4 "
  echo "cp $srcdir/testfiles/jpeg_img.jpg . "
  echo "./jpeg2hdf jpeg_img.jpg jpeg.hdf "
  echo "./hdf2jpeg jpeg.hdf jpeg2.jpg "
  echo "(./hdfls -l jpeg.hdf | $SED) >& hdfls.tmp4 "
  echo "diff hdfls.tmp4 $srcdir/jpeg2hdf.out1 "
  echo "cmp jpeg_img.jpg jpeg2.jpg "
  echo " ******* END NOTE *************"
  echo ""
  errors=0
fi

##
##  fp2hdf is now called hdfimport, and is in mfhdf/hdfimport
##
 # # fp2hdf
 # if [ -f fp2hdf -a -f hdfls -a -f $HDFED ]; then
 # echo "** Testing fp2hdf  ***"
 # if [ ! "$has32" ]; then
 #     echo "    $machinetype does not support float32."
 #     echo "    float32 tests skipped."
 # fi
 # 
 # /bin/rm -f ctxtr* cb* *.hdf hdfls.tmp5 hdfed.tmp6
 # ./fptest
 # ./fp2hdf ctxtr2 -o ctxtr2.hdf
 # ./fp2hdf ctxtr3 -o ctxtr3.hdf
 # if [ "$has32" ]; then
 # ./fp2hdf cb32r2 -o cb32r2.hdf
 # ./fp2hdf cb32r3 -o cb32r3.hdf
 # fi
 # ./fp2hdf cb64r2 -o cb64r2.hdf
 # ./fp2hdf cb64r3 -o cb64r3.hdf
 # ./fp2hdf ctxtr2 -o ctxtr2_ris.hdf -raster -e 50 50
 # ./fp2hdf cb64r2 -o cb64r2_ris.hdf -raster -i 50 50 -f
 # (./hdfls -l ctxtr2.hdf | $SED) > hdfls.tmp5 2>&1
 # (./hdfls -l ctxtr3.hdf | $SED) >> hdfls.tmp5 2>&1
 # if [ "$has32" ]; then
 # (./hdfls -l cb32r2.hdf | $SED) >> hdfls.tmp5 2>&1
 # (./hdfls -l cb32r3.hdf | $SED) >> hdfls.tmp5 2>&1
 # fi
 # (./hdfls -l cb64r2.hdf | $SED) >> hdfls.tmp5 2>&1
 # (./hdfls -l cb64r3.hdf | $SED) >> hdfls.tmp5 2>&1
 # (./hdfls -l ctxtr2_ris.hdf | $SED) >> hdfls.tmp5 2>&1
 # (./hdfls -l cb64r2_ris.hdf | $SED) >> hdfls.tmp5 2>&1
 # if [ "$has32" ]; then
 #     diff  hdfls.tmp5 $srcdir/fp2hdf.out1 || errors=1
 #     $HDFEDCMD < $srcdir/fp2hdf.input1 > hdfed.tmp6 2>&1
 #     diff  hdfed.tmp6 $srcdir/fp2hdf.out2 || errors=1
 # else
 #     diff  hdfls.tmp5 $srcdir/fp2hdf.out1-32 || errors=1
 #     $HDFEDCMD < $srcdir/fp2hdf.input1-32 > hdfed.tmp6 2>&1
 #     diff  hdfed.tmp6 $srcdir/fp2hdf.out2-32 || errors=1
 # fi
 # /bin/rm -f ctxtr* cb* *.hdf hdfls.tmp5 hdfed.tmp6
 # else
 # echo "** fp2hdf, hdfed or hdfls not available ***"
 # fi
 # 
 # if [ $errors -eq 1 ]; then
 #   haserr=1
 #   echo " ********* NOTE ***************"
 #   echo " fp2hdf might have failed ***"
 #   echo " The above errors could be formatting "
 #   echo " problems which can be ignored "
 #   echo " please run the following by hand to verify "
 #   echo " "
 #   echo "  /bin/rm -f ctxtr* cb* *.hdf hdfls.tmp5 hdfed.tmp6"
 #   echo " ./fptest "
 #   echo " ./fp2hdf ctxtr2 -o ctxtr2.hdf "
 #   echo " ./fp2hdf ctxtr3 -o ctxtr3.hdf "
 #   if [ "$has32" ]; then
 #   echo " ./fp2hdf cb32r2 -o cb32r2.hdf "
 #   echo " ./fp2hdf cb32r3 -o cb32r3.hdf "
 #   fi
 #   echo " ./fp2hdf cb64r2 -o cb64r2.hdf "
 #   echo " ./fp2hdf cb64r3 -o cb64r3.hdf "
 #   echo " ./fp2hdf ctxtr2 -o ctxtr2_ris.hdf -raster -e 50 50 "
 #   echo " ./fp2hdf cb64r2 -o cb64r2_ris.hdf -raster -i 50 50 -f "
 #   echo "( ./hdfls -l ctxtr2.hdf | $SED) >&  hdfls.tmp5 "
 #   echo "( ./hdfls -l ctxtr3.hdf | $SED) >>& hdfls.tmp5 "
 #   if [ "$has32" ]; then
 #   echo "( ./hdfls -l cb32r2.hdf | $SED) >>& hdfls.tmp5 "
 #   echo "( ./hdfls -l cb32r3.hdf | $SED) >>& hdfls.tmp5 "
 #   fi
 #   echo "( ./hdfls -l cb64r2.hdf | $SED) >>& hdfls.tmp5 "
 #   echo "( ./hdfls -l cb64r3.hdf | $SED) >>& hdfls.tmp5 "
 #   echo "( ./hdfls -l ctxtr2_ris.hdf | $SED) >>& hdfls.tmp5 "
 #   echo "( ./hdfls -l cb64r2_ris.hdf | $SED) >>& hdfls.tmp5 "
 #   if [ "$has32" ]; then
 #   echo " diff hdfls.tmp5 $srcdir/fp2hdf.out1 "
 #   echo " $HDFEDCMD < $srcdir/fp2hdf.input1 >& hdfed.tmp6 "
 #   echo " diff hdfed.tmp6 $srcdir/fp2hdf.out2 "
 #   else
 #   echo " diff hdfls.tmp5 $srcdir/fp2hdf.out1-32 "
 #   echo " $HDFEDCMD < $srcdir/fp2hdf.input1-32 >& hdfed.tmp6 "
 #   echo " diff hdfed.tmp6 $srcdir/fp2hdf.out2-32 "
 #   fi
 #   echo " ******* END NOTE *************"
 #   echo ""
 #   errors=0
 # fi

#
# Check errors result
if [ $haserr -eq 0 ]; then
  echo "=============================="
  echo "HDF Utilities tests passed."
  echo "=============================="
else
  echo "**************************************"
  echo "HDF Utilities tests encountered errors"
  echo "**************************************"
fi

echo ""
exit $haserr
