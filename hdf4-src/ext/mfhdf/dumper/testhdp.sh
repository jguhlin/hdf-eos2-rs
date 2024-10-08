#! /bin/sh
# Test scripts for hdp (dumper).
# See the USAGE function for command usage.

srcdir=.
currentdir=`pwd`

# Determine whether the szip library is available
USE_COMP_SZIP="no"

# Definitions of commands and variables
HDP='./hdp'               # The tool name
HDP_BIN="${TESTS_ENVIRONMENT} "`pwd`/$HDP        # The path of the tool binary

RM='rm -f'
DIFF=diff
CMP='cmp -s'
nerrors=0		# number of errors (0)
quitonerr=0		# quit on error (not)
noclean=0		# no cleaning temp. files (yes)
only=""			# dumper sub-command to test only
except=""		# dumper sub-command to test not


# Definitions of functions/shorthands
#

# Print Usage of the command
USAGE() {
    echo "Usage: $0 [-help] [-noclean] [-quit] [-except <command>] [-only <command>]"
    echo "    -help: display help information"
    echo "    -noclean: do not clean away temporary files"
    echo "    -quit: quit immediately if any test fails"
    echo "    -except: skip one specific command"
    echo "    -only: test one specific command"
    echo "<command> can be one of {list, dumpsds, dumprig, dumpvd, dumpvg, dumpgr}"
}

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


# Run the test to produce an output file which is then
# compared with the expected ($1) output.
# Note that this can be used to produce the expected
# output files by replace "$output" with "$expected"
# in the run-the-test commands.
TEST()
{
  # parse the arguments
  output=tmp.out
  expected="$srcdir/testfiles/$1"
  shift

  # run the test
  ( 
    cd $srcdir/testfiles
    $HDP_BIN "$@"
  ) > $output
  $CMP $expected $output

  if [ $? -ne 0 ]; then
    echo $DIFF $expected $output
    $DIFF $expected $output
    echo "   <<< FAILED >>>"
    nerrors=`expr $nerrors + 1`

    if [ $quitonerr -gt 0 ]; then
      FINISH
    fi
  fi

  if [ $noclean -eq 0 ]; then
    $RM -f $output
  fi
}


# Report the result and exit
FINISH()
{
    if [ $nerrors -eq 0 ]
    then
	MESG 0 "All hdp tests passed"
    else
	MESG 0 "hdp tests failed: $nerrors"
    fi
    exit $nerrors
}


#===============
# Main Body
#===============

# parse arguments
while [ $# -gt 0 ]
do
    case "$1" in
	"-quit")
	    quitonerr=1
	    ;;
	"-noclean")
	    noclean=1
	    ;;
	"-help")
	    USAGE
	    exit 0
	    ;;
	"-only")
	    shift
	    case "$1" in
    		"list"|"dumpsds"|"dumprig"|"dumpvd"|"dumpvg"|"dumpgr")
		    only="$1"
		    ;;
		*)
		    echo "Unknown command: $1"
		    USAGE
		    exit 1
		    ;;
	    esac
	    ;;
	"-except")
	    shift
	    case "$1" in
    		"list"|"dumpsds"|"dumprig"|"dumpvd"|"dumpvg"|"dumpgr")
		    except="$1"
		    ;;
		*)
		    echo "Unknown command: $1"
		    USAGE
		    exit 1
		    ;;
	    esac
	    ;;
	* )
	    echo "Unknown option: $1"
	    USAGE
	    exit 1
	    ;;
    esac
    shift
done

# Print a beginning banner
MESG 0 "Running hdp tests"

# Test command list
TestCmd=list
TestName="Test command $TestCmd"
if [ "$except" != $TestCmd -a \( -z "$only" -o "$only" = $TestCmd \) ]
then
MESG 3 "$TestName"
TEST list-1.out list tdata.hdf
TEST list-2.out list -l tdata.hdf
TEST list-3.out list -d tdata.hdf
TEST list-4.out list -e tdata.hdf
TEST list-5.out list -t 720 tdata.hdf
TEST list-6.out list -d -t "Numeric Data Group" tdata.hdf
TEST list-7.out list -g tdata.hdf
TEST list-8.out list -a tdata.hdf
TEST list-9.out list -a Example6.hdf
TEST list-10.out list -n Example6.hdf
else
MESG 3 "$TestName <<<SKIPPED>>>"
fi

# Test command dumpsds
TestCmd=dumpsds
TestName="Test command $TestCmd"
if [ "$except" != $TestCmd -a \( -z "$only" -o "$only" = $TestCmd \) ]
then
MESG 3 "$TestName"
# Test 1 prints all datasets
TEST dumpsds-1.out dumpsds swf32.hdf

# Tests 2 and 3 print datasets given their indices
TEST dumpsds-2.out dumpsds -i 2 swf32.hdf
TEST dumpsds-3.out dumpsds -i 1,3 swf32.hdf

# Test 4 should fail with error message: "SD with name Time: not found"
TEST dumpsds-4.out dumpsds -n Time swf32.hdf

# Test 5 prints datasets given their names 
TEST dumpsds-5.out dumpsds -n fakeDim0,Data-Set-2 swf32.hdf

# Test 6 prints datasets given their ref numbers
TEST dumpsds-6.out dumpsds -r 3,2 swf32.hdf

# Test 7 prints only data of the datasets selected by their ref numbers
TEST dumpsds-7.out dumpsds -r 3,2 -d swf32.hdf

# Test 8 prints only header information
TEST dumpsds-8.out dumpsds -h swf32_fileattr.hdf

# Test 9 prints data in clean format, no \digit's
TEST dumpsds-9.out dumpsds -c swf32_fileattr.hdf

# Test 10 prints contents of file without file attribute's data
TEST dumpsds-10.out dumpsds -g swf32_fileattr.hdf

# Test 11 prints contents of file without local attribute's data
TEST dumpsds-11.out dumpsds -l swf32_fileattr.hdf

# Test 12 prints a dataset by name and the name is very long
TEST dumpsds-12.out dumpsds -h -n "The name of this dataset is long and it is used to test the new variable length name feature." SDSlongname.hdf

# Test 13 prints contents of file when a dimension has the same name as its SDS
TEST dumpsds-13.out dumpsds sds1_dim1_samename.hdf

# Test 14 prints contents of file when a dimension has the same name as 
# that of another SDS
TEST dumpsds-14.out dumpsds sds2_dim1_samename.hdf

# Test 15 prints headers of all data sets with various compression method to
# test displaying compression information

if test $USE_COMP_SZIP != "yes"; then
TEST dumpsds-15.out dumpsds sds_compressed.hdf
else
TEST dumpsds-15szip.out dumpsds sds_compressed.hdf
fi

# Test 16 prints SDSs in index order, by default
TEST dumpsds-16.out dumpsds -h -i 39,36 -n data34,data27 -r 36,37 -i 0,1 -n data6,data9,data4,data3 -r 16,17,15 -i 23,22,21 sds_empty_many.hdf

# Test 17 prints SDSs in the order they were specified, when flag -k is present
# as a request to 'k'eep the specified order
TEST dumpsds-17.out dumpsds -k -h -i 39,36 -n data34,data27 -r 36,37 -i 0,1 -n data6,data9,data4,data3 -r 16,17,15 -i 23,22,21 sds_empty_many.hdf

# Test 18 reads a few small datasets in a netCDF file
TEST dumpsds-18.out dumpsds -i 0,1,2 Roy.nc

# Test 19 displays a message when the file is a netCDF 64-bit file
TEST dumpsds-19.out dumpsds Roy-64.nc

else
MESG 3 "$TestName <<<SKIPPED>>>"
fi

# Test command dumprig
TestCmd=dumprig
TestName="Test command $TestCmd"
if [ "$except" != $TestCmd -a \( -z "$only" -o "$only" = $TestCmd \) ]
then
MESG 3 "$TestName"
TEST dumprig-1.out dumprig tdf24.hdf
TEST dumprig-2.out dumprig -i 1,2 tdf24.hdf
TEST dumprig-3.out dumprig -i 1,3 tdf24.hdf		# '-i 3' is invalid
TEST dumprig-4.out dumprig -m 24 tdf24.hdf
TEST dumprig-5.out dumprig -r 3,4 tdf24.hdf
TEST dumprig-6.out dumprig -r 3,4 -d tdf24.hdf
else
MESG 3 "$TestName <<<SKIPPED>>>"
fi

# Test command dumpvd
TestCmd=dumpvd
TestName="Test command $TestCmd"
if [ "$except" != $TestCmd -a \( -z "$only" -o "$only" = $TestCmd \) ]
then
MESG 3 "$TestName"
TEST dumpvd-1.out dumpvd tvset.hdf
TEST dumpvd-2.out dumpvd -i 1,3,5 tvset.hdf
TEST dumpvd-3.out dumpvd -r 1238,1239,1251,1252 tvset.hdf
TEST dumpvd-4.out dumpvd -n "Multi-Order Vdata" tvset.hdf
TEST dumpvd-5.out dumpvd -n "Mixed Vdata","Integer Vdata" tvset.hdf
TEST dumpvd-6.out dumpvd -c "Test object","No class specified" tvset.hdf
TEST dumpvd-7.out dumpvd -f B tvset.hdf
TEST dumpvd-8.out dumpvd -f "STATION_NAME","FLOATS" tvset.hdf
TEST dumpvd-9.out dumpvd -f "STATION_NAME","FLOATS" -d tvset.hdf
TEST dumpvd-10.out dumpvd tvattr.hdf

# Tests 11 and 12 print out the vdatas of classes "SDSVar" and "CoordVar"
# to test the fix of bugzilla 624 (these are new classes used to distinguish
# between SDS and coordinate variables)
TEST dumpvd-11.out dumpvd -c "SDSVar" sds1_dim1_samename.hdf
TEST dumpvd-12.out dumpvd -c "CoordVar" sds1_dim1_samename.hdf

# Added test #13 to test long vdata's name and class, HDFFR-1267 - BMR 5/15/11
TEST dumpvd-13.out dumpvd vslongname.hdf

# Added test #14 to test the detection of external file, HDFFR-1266.  The
# external file is Tables_External_File and when the file is removed from the
# current directory, the test will fail with the name of the file displayed in
# the error message. - BMR 6/10/11
TEST dumpvd-14.out dumpvd Tables.hdf

else
MESG 3 "$TestName <<<SKIPPED>>>"
fi

# Test command dumpvg
TestCmd=dumpvg
TestName="Test command $TestCmd"
if [ "$except" != $TestCmd -a \( -z "$only" -o "$only" = $TestCmd \) ]
then
MESG 3 "$TestName"
TEST dumpvg-1.out dumpvg tvset.hdf
TEST dumpvg-2.out dumpvg -i 0,1 tvset.hdf
TEST dumpvg-3.out dumpvg -r 3 tvset.hdf
TEST dumpvg-4.out dumpvg -n "Simple Vgroup" tvset.hdf
TEST dumpvg-5.out dumpvg -c "Test object" tvset.hdf
TEST dumpvg-6.out dumpvg -i 1,3,5 tdata.hdf
TEST dumpvg-7.out dumpvg -r 32,39 tdata.hdf
TEST dumpvg-8.out dumpvg -n nsamp,tdata.hdf tdata.hdf
TEST dumpvg-9.out dumpvg -c CDF0.0 tdata.hdf

# Added option -h to the following test; this option has always 
# failed; just fixed it - BMR 8/1/00
TEST dumpvg-10.out dumpvg -h -c Dim0.0,Var0.0 tdata.hdf

# this following test is removed since option -d is removed
#TEST dumpvg-11.out dumpvg -c Dim0.0,Var0.0 -d tdata.hdf

# moved test #12 up to #11, consequently - BMR 7/25/00
TEST dumpvg-11.out dumpvg tvattr.hdf

# Added these two tests for the new feature: vgroup has variable length 
# name - BMR 10/27/06
# Note that the dumpvg-13 test searches for an SDS also
TEST dumpvg-12.out dumpvg VGlongname.hdf
TEST dumpvg-13.out dumpvg -n "SD Vgroup - this vgroup has an sds as a member and it is actually meant to test long vgroup name" VGlongname.hdf

# Prints contents of file when a dimension has the same name as its SDS 
TEST dumpvg-14.out dumpvg sds1_dim1_samename.hdf

# Prints contents of file when a dimension has the same name as that 
# of another SDS
TEST dumpvg-15.out dumpvg sds2_dim1_samename.hdf

# Verify the fix for bug HDFFR-197 and a vgroup with ref=0 (some old RI stuff)
TEST dumpvg-16.out dumpvg -h grtdfi322.hdf
TEST dumpvg-17.out dumpvg grtdfi322.hdf

else
MESG 3 "$TestName <<<SKIPPED>>>"
fi

# Test command dumpgr
TestCmd=dumpgr
TestName="Test command $TestCmd"
if [ "$except" != $TestCmd -a \( -z "$only" -o "$only" = $TestCmd \) ]
then
MESG 3 "$TestName"
TEST dumpgr-1.out dumpgr grtdfui82.hdf
TEST dumpgr-2.out dumpgr -i 0,1,3 grtdfui82.hdf
TEST dumpgr-3.out dumpgr -i 0 grtdfui82.hdf
TEST dumpgr-4.out dumpgr -n Image_array_5 grtdfui82.hdf
TEST dumpgr-5.out dumpgr -r 6,2,3 grtdfui82.hdf
TEST dumpgr-6.out dumpgr -r 6 -d  grtdfui82.hdf
TEST dumpgr-7.out dumpgr -o $currentdir/my.dat grtdfui82.hdf
$RM my.dat
TEST dumpgr-8.out dumpgr -o $currentdir/mybin.dat  -b grtdfui82.hdf
$RM mybin.dat
TEST dumpgr-9.out dumpgr grtdfui83.hdf
TEST dumpgr-10.out dumpgr grtdfui84.hdf
TEST dumpgr-11.out dumpgr grtdfui162.hdf

# Test 12 shows that file attribute is printed
TEST dumpgr-12.out dumpgr grtdfi322.hdf

# Tests 13, 14, and 15 test option -h, which was not included in any
# of the previous tests, and the new options -p and -pd, printing palette
# with or without palette information 
TEST dumpgr-13.out dumpgr -p Image_with_Palette.hdf
TEST dumpgr-14.out dumpgr -h Image_with_Palette.hdf
TEST dumpgr-15.out dumpgr -r 2,4 -pd Image_with_Palette.hdf

# Test 16: to test new option -s, printing data as stream
TEST dumpgr-16.out dumpgr -r 6 -d -s grtdfui82.hdf

# Test 17: to test new option -m (interlace mode = LINE)
TEST dumpgr-17.out dumpgr -r 6 -m 1 grtdfui82.hdf

# Test 18: to test new option -c (printing attribute data in clean format)
TEST dumpgr-18.out dumpgr -c grtdfi322.hdf

# Test 19: to test new options -g and -l (suppress all attribute data)
TEST dumpgr-19.out dumpgr -g -l grtdfi322.hdf

# Test 20: to test dumpgr successfully detect IMCOMP compression type
TEST dumpgr-20.out dumpgr IMCOMP.hdf

else
MESG 3 "$TestName <<<SKIPPED>>>"
fi

# End of test
FINISH
