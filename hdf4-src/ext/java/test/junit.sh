#! /bin/sh
#
# Copyright by The HDF Group.
# Copyright by the Board of Trustees of the University of Illinois.
# All rights reserved.
#
# This file is part of HDF Java Products. The full HDF Java copyright
# notice, including terms governing use, modification, and redistribution,
# is contained in the file, COPYING.  COPYING can be found at the root of
# the source code distribution tree. You can also access it online at
# http://www.hdfgroup.org/licenses.  If you do not have access to the file,
# you may request a copy from help@hdfgroup.org.
#

top_builddir=../..
top_srcdir=../..
srcdir=.

TESTNAME=JUnitInterface
EXIT_SUCCESS=0
EXIT_FAILURE=1

# Set up default variable values if not supplied by the user.
RM='rm -rf'
CMP='cmp'
DIFF='diff -c'
CP='cp'
DIRNAME='dirname'
LS='ls'
AWK='awk'

nerrors=0
verbose=yes

# setup my machine information.
myos=`uname -s`

# where the libs exist
HDFLIB_HOME="$top_srcdir/java/lib"
BLDLIBDIR="$top_builddir/hdf4/lib"
BLDDIR="."
HDFTEST_HOME="$top_srcdir/java/test"
JARFILE=jarhdf-4.2.17-1.jar
TESTJARFILE=jarhdftest.jar
test -d $BLDLIBDIR || mkdir -p $BLDLIBDIR

######################################################################
# library files
# --------------------------------------------------------------------
# All the library files copy from source directory to test directory
# NOTE: Keep this framework to add/remove test files.
#       This list are also used for checking exist.
#       Comment '#' without space can be used.
# --------------------------------------------------------------------
LIST_LIBRARY_FILES="
$top_builddir/hdf/src/.libs/libdf.*
$top_builddir/mfhdf/libsrc/.libs/libmfhdf.*
$top_builddir/java/src/jni/.libs/libhdf_java.*
"
LIST_JAR_TESTFILES="
$HDFLIB_HOME/hamcrest-core.jar
$HDFLIB_HOME/junit.jar
$HDFLIB_HOME/slf4j-api-1.7.33.jar
$HDFLIB_HOME/ext/slf4j-simple-1.7.33.jar
"
LIST_JAR_FILES="
$top_builddir/java/src/$JARFILE
"
LIST_DATA_FILES="
$HDFTEST_HOME/testfiles/JUnit-TestH4.txt
$HDFTEST_HOME/testfiles/JUnit-TestH4ANparams.txt
$HDFTEST_HOME/testfiles/JUnit-TestH4DFparams.txt
$HDFTEST_HOME/testfiles/JUnit-TestH4DFPparams.txt
$HDFTEST_HOME/testfiles/JUnit-TestH4DFRparams.txt
$HDFTEST_HOME/testfiles/JUnit-TestH4GRparams.txt
$HDFTEST_HOME/testfiles/JUnit-TestH4HCparams.txt
$HDFTEST_HOME/testfiles/JUnit-TestH4SDparams.txt
$HDFTEST_HOME/testfiles/JUnit-TestH4Vparams.txt
$HDFTEST_HOME/testfiles/JUnit-TestH4VSparams.txt
"

#
# copy files from source dirs to test dir
#
COPY_LIBFILES="$LIST_LIBRARY_FILES"
COPY_JARTESTFILES="$LIST_JAR_TESTFILES"
COPY_JARFILES="$LIST_JAR_FILES"

COPY_LIBFILES_TO_BLDLIBDIR()
{
    # copy test files. Used -f to make sure get a new copy
    for tstfile in $COPY_LIBFILES
    do
        # ignore '#' comment
        echo $tstfile | tr -d ' ' | grep '^#' > /dev/null
        RET=$?
        if [ $RET -eq 1 ]; then
            # skip cp if srcdir is same as destdir
            # this occurs when build/test performed in source dir and
            # make cp fail
            SDIR=`$DIRNAME $tstfile`
            INODE_SDIR=`$LS -i -d $SDIR | $AWK -F' ' '{print $1}'`
            INODE_DDIR=`$LS -i -d $BLDLIBDIR | $AWK -F' ' '{print $1}'`
            if [ "$INODE_SDIR" != "$INODE_DDIR" ]; then
                $CP -f $tstfile $BLDLIBDIR
                if [ $? -ne 0 ]; then
                    echo "Error: FAILED to copy $tstfile ."

                    # Comment out this to CREATE expected file
                    exit $EXIT_FAILURE
                fi
            fi
        fi
    done
    # copy jar files. Used -f to make sure get a new copy
    for tstfile in $COPY_JARTESTFILES
    do
        # ignore '#' comment
        echo $tstfile | tr -d ' ' | grep '^#' > /dev/null
        RET=$?
        if [ $RET -eq 1 ]; then
            # skip cp if srcdir is same as destdir
            # this occurs when build/test performed in source dir and
            # make cp fail
            SDIR=`$DIRNAME $tstfile`
            INODE_SDIR=`$LS -i -d $SDIR | $AWK -F' ' '{print $1}'`
            INODE_DDIR=`$LS -i -d $BLDLIBDIR | $AWK -F' ' '{print $1}'`
            if [ "$INODE_SDIR" != "$INODE_DDIR" ]; then
                $CP -f $tstfile $BLDLIBDIR
                if [ $? -ne 0 ]; then
                    echo "Error: FAILED to copy $tstfile ."

                    # Comment out this to CREATE expected file
                    exit $EXIT_FAILURE
                fi
            fi
        fi
    done
    for tstfile in $COPY_JARFILES
    do
        # ignore '#' comment
        echo $tstfile | tr -d ' ' | grep '^#' > /dev/null
        RET=$?
        if [ $RET -eq 1 ]; then
            # skip cp if srcdir is same as destdir
            # this occurs when build/test performed in source dir and
            # make cp fail
            SDIR=`$DIRNAME $tstfile`
            INODE_SDIR=`$LS -i -d $SDIR | $AWK -F' ' '{print $1}'`
            INODE_DDIR=`$LS -i -d $BLDLIBDIR | $AWK -F' ' '{print $1}'`
            if [ "$INODE_SDIR" != "$INODE_DDIR" ]; then
                $CP -f $tstfile $BLDLIBDIR
                if [ $? -ne 0 ]; then
                    echo "Error: FAILED to copy $tstfile ."

                    # Comment out this to CREATE expected file
                    exit $EXIT_FAILURE
                fi
            fi
        fi
    done
}

CLEAN_LIBFILES_AND_BLDLIBDIR()
{
    # skip rm if srcdir is same as destdir
    # this occurs when build/test performed in source dir and
    # make cp fail
    SDIR=$HDFLIB_HOME
    INODE_SDIR=`$LS -i -d $SDIR | $AWK -F' ' '{print $1}'`
    INODE_DDIR=`$LS -i -d $BLDLIBDIR | $AWK -F' ' '{print $1}'`
    if [ "$INODE_SDIR" != "$INODE_DDIR" ]; then
        for tstfile in $COPY_JARTESTFILES
        do
            $RM $BLDLIBDIR/tstfile
        done
    fi
}

COPY_DATAFILES="$LIST_DATA_FILES"

COPY_DATAFILES_TO_BLDDIR()
{
    # copy test files. Used -f to make sure get a new copy
    for tstfile in $COPY_DATAFILES
    do
        # ignore '#' comment
        echo $tstfile | tr -d ' ' | grep '^#' > /dev/null
        RET=$?
        if [ $RET -eq 1 ]; then
            # skip cp if srcdir is same as destdir
            # this occurs when build/test performed in source dir and
            # make cp fail
            SDIR=`$DIRNAME $tstfile`
            INODE_SDIR=`$LS -i -d $SDIR | $AWK -F' ' '{print $1}'`
            INODE_DDIR=`$LS -i -d $BLDDIR | $AWK -F' ' '{print $1}'`
            if [ "$INODE_SDIR" != "$INODE_DDIR" ]; then
                $CP -f $tstfile $BLDDIR
                if [ $? -ne 0 ]; then
                    echo "Error: FAILED to copy $tstfile ."

                    # Comment out this to CREATE expected file
                    exit $EXIT_FAILURE
                fi
            fi
        fi
    done
}

CLEAN_DATAFILES_AND_BLDDIR()
{
        $RM $BLDDIR/JUnit-*.out
        $RM $BLDDIR/JUnit-*.err
        $RM $BLDDIR/JUnit-*.ext
    # skip rm if srcdir is same as destdir
    # this occurs when build/test performed in source dir and
    # make cp fail
    SDIR=`$DIRNAME $tstfile`
    INODE_SDIR=`$LS -i -d $SDIR | $AWK -F' ' '{print $1}'`
    INODE_DDIR=`$LS -i -d $BLDDIR | $AWK -F' ' '{print $1}'`
    if [ "$INODE_SDIR" != "$INODE_DDIR" ]; then
        $RM $BLDDIR/JUnit-*.ert
        $RM $BLDDIR/JUnit-*.txt
    fi
}

# Print a line-line message left justified in a field of 70 characters
# beginning with the word "Testing".
#
TESTING() {
   SPACES="                                                               "
   echo "Testing $* $SPACES" | cut -c1-70 | tr -d '\012'
}

# where Java is installed (requires jdk1.7.x)
JAVAEXE=
JAVAEXEFLAGS=

###############################################################################
#            DO NOT MODIFY BELOW THIS LINE
###############################################################################

# prepare for test
COPY_LIBFILES_TO_BLDLIBDIR
COPY_DATAFILES_TO_BLDDIR

CPATH=".:"$BLDLIBDIR"/"$JARFILE":"$BLDLIBDIR"/junit.jar:"$BLDLIBDIR"/hamcrest-core.jar:"$BLDLIBDIR"/slf4j-api-1.7.33.jar:"$BLDLIBDIR"/slf4j-simple-1.7.33.jar:"$TESTJARFILE""

TEST=/usr/bin/test
if [ ! -x /usr/bin/test ]
then
TEST=`which test`
fi

if $TEST -z "$CLASSPATH"; then
        CLASSPATH=""
fi
CLASSPATH=$CPATH":"$CLASSPATH
export CLASSPATH

if $TEST -n "$JAVAPATH" ; then
        PATH=$JAVAPATH":"$PATH
        export PATH
fi

if $TEST -e /bin/uname; then
   os_name=`/bin/uname -s`
elif $TEST -e /usr/bin/uname; then
   os_name=`/usr/bin/uname -s`
else
   os_name=unknown
fi

if $TEST -z "$LD_LIBRARY_PATH" ; then
        LD_LIBRARY_PATH=""
fi

case  $os_name in
    *)
    LD_LIBRARY_PATH=$BLDLIBDIR:$LD_LIBRARY_PATH
    ;;
esac

export LD_LIBRARY_PATH

echo "$JAVAEXE $JAVAEXEFLAGS -Xmx1024M -Dorg.slf4j.simpleLogger.defaultLog=trace -Djava.library.path=$BLDLIBDIR -cp $CLASSPATH -ea org.junit.runner.JUnitCore test.TestH4"
TESTING JUnit-TestH4
($RUNSERIAL $JAVAEXE $JAVAEXEFLAGS -Xmx1024M -Dorg.slf4j.simpleLogger.defaultLog=trace -Djava.library.path=$BLDLIBDIR -cp $CLASSPATH -ea org.junit.runner.JUnitCore test.TestH4 > JUnit-TestH4.ext)

# Extract file name, line number, version and thread IDs because they may be different
sed -e 's/thread [0-9]*/thread (IDs)/' -e 's/: .*\.c /: (file name) /' \
    -e 's/line [0-9]*/line (number)/' \
    -e 's/Time: [0-9]*\.[0-9]*/Time:  XXXX/' \
    -e 's/v[1-9]*\.[0-9]*\./version (number)\./' \
    -e 's/[1-9]*\.[0-9]*\.[0-9]*[^)]*/version (number)/' \
    JUnit-TestH4.ext > JUnit-TestH4.out

if diff JUnit-TestH4.out JUnit-TestH4.txt > /dev/null; then
    echo "  PASSED      JUnit-TestH4"
else
    echo "**FAILED**    JUnit-TestH4"
    echo "    Expected result differs from actual result"
    nerrors="`expr $nerrors + 1`"
    test yes = "$verbose" && $DIFF JUnit-TestH4.txt JUnit-TestH4.out |sed 's/^/    /'
fi

echo "$JAVAEXE $JAVAEXEFLAGS -Xmx1024M -Dorg.slf4j.simpleLogger.defaultLog=trace -Djava.library.path=$BLDLIBDIR -cp $CLASSPATH -ea org.junit.runner.JUnitCore test.TestH4ANparams"
TESTING JUnit-TestH4ANparams
($RUNSERIAL $JAVAEXE $JAVAEXEFLAGS -Xmx1024M -Dorg.slf4j.simpleLogger.defaultLog=trace -Djava.library.path=$BLDLIBDIR -cp $CLASSPATH -ea org.junit.runner.JUnitCore test.TestH4ANparams > JUnit-TestH4ANparams.ext)

# Extract file name, line number, version and thread IDs because they may be different
sed -e 's/thread [0-9]*/thread (IDs)/' -e 's/: .*\.c /: (file name) /' \
    -e 's/line [0-9]*/line (number)/' \
    -e 's/Time: [0-9]*\.[0-9]*/Time:  XXXX/' \
    -e 's/v[1-9]*\.[0-9]*\./version (number)\./' \
    -e 's/[1-9]*\.[0-9]*\.[0-9]*[^)]*/version (number)/' \
    JUnit-TestH4ANparams.ext > JUnit-TestH4ANparams.out

if diff JUnit-TestH4ANparams.out JUnit-TestH4ANparams.txt > /dev/null; then
    echo "  PASSED      JUnit-TestH4ANparams"
else
    echo "**FAILED**    JUnit-TestH4ANparams"
    echo "    Expected result differs from actual result"
    nerrors="`expr $nerrors + 1`"
    test yes = "$verbose" && $DIFF JUnit-TestH4ANparams.txt JUnit-TestH4ANparams.out |sed 's/^/    /'
fi

echo "$JAVAEXE $JAVAEXEFLAGS -Xmx1024M -Dorg.slf4j.simpleLogger.defaultLog=trace -Djava.library.path=$BLDLIBDIR -cp $CLASSPATH -ea org.junit.runner.JUnitCore test.TestH4DFparams"
TESTING JUnit-TestH4DFparams
($RUNSERIAL $JAVAEXE $JAVAEXEFLAGS -Xmx1024M -Dorg.slf4j.simpleLogger.defaultLog=trace -Djava.library.path=$BLDLIBDIR -cp $CLASSPATH -ea org.junit.runner.JUnitCore test.TestH4DFparams > JUnit-TestH4DFparams.ext)

# Extract file name, line number, version and thread IDs because they may be different
sed -e 's/thread [0-9]*/thread (IDs)/' -e 's/: .*\.c /: (file name) /' \
    -e 's/line [0-9]*/line (number)/' \
    -e 's/Time: [0-9]*\.[0-9]*/Time:  XXXX/' \
    -e 's/v[1-9]*\.[0-9]*\./version (number)\./' \
    -e 's/[1-9]*\.[0-9]*\.[0-9]*[^)]*/version (number)/' \
    JUnit-TestH4DFparams.ext > JUnit-TestH4DFparams.out

if diff JUnit-TestH4DFparams.out JUnit-TestH4DFparams.txt > /dev/null; then
    echo "  PASSED      JUnit-TestH4DFparams"
else
    echo "**FAILED**    JUnit-TestH4DFparams"
    echo "    Expected result differs from actual result"
    nerrors="`expr $nerrors + 1`"
    test yes = "$verbose" && $DIFF JUnit-TestH4DFparams.txt JUnit-TestH4DFparams.out |sed 's/^/    /'
fi

echo "$JAVAEXE $JAVAEXEFLAGS -Xmx1024M -Dorg.slf4j.simpleLogger.defaultLog=trace -Djava.library.path=$BLDLIBDIR -cp $CLASSPATH -ea org.junit.runner.JUnitCore test.TestH4DFPparams"
TESTING JUnit-TestH4DFPparams
($RUNSERIAL $JAVAEXE $JAVAEXEFLAGS -Xmx1024M -Dorg.slf4j.simpleLogger.defaultLog=trace -Djava.library.path=$BLDLIBDIR -cp $CLASSPATH -ea org.junit.runner.JUnitCore test.TestH4DFPparams > JUnit-TestH4DFPparams.ext)

# Extract file name, line number, version and thread IDs because they may be different
sed -e 's/thread [0-9]*/thread (IDs)/' -e 's/: .*\.c /: (file name) /' \
    -e 's/line [0-9]*/line (number)/' \
    -e 's/Time: [0-9]*\.[0-9]*/Time:  XXXX/' \
    -e 's/v[1-9]*\.[0-9]*\./version (number)\./' \
    -e 's/[1-9]*\.[0-9]*\.[0-9]*[^)]*/version (number)/' \
    JUnit-TestH4DFPparams.ext > JUnit-TestH4DFPparams.out

if diff JUnit-TestH4DFPparams.out JUnit-TestH4DFPparams.txt > /dev/null; then
    echo "  PASSED      JUnit-TestH4DFPparams"
else
    echo "**FAILED**    JUnit-TestH4DFPparams"
    echo "    Expected result differs from actual result"
    nerrors="`expr $nerrors + 1`"
    test yes = "$verbose" && $DIFF JUnit-TestH4DFPparams.txt JUnit-TestH4DFPparams.out |sed 's/^/    /'
fi

echo "$JAVAEXE $JAVAEXEFLAGS -Xmx1024M -Dorg.slf4j.simpleLogger.defaultLog=trace -Djava.library.path=$BLDLIBDIR -cp $CLASSPATH -ea org.junit.runner.JUnitCore test.TestH4DFRparams"
TESTING JUnit-TestH4DFRparams
($RUNSERIAL $JAVAEXE $JAVAEXEFLAGS -Xmx1024M -Dorg.slf4j.simpleLogger.defaultLog=trace -Djava.library.path=$BLDLIBDIR -cp $CLASSPATH -ea org.junit.runner.JUnitCore test.TestH4DFRparams > JUnit-TestH4DFRparams.ext)

# Extract file name, line number, version and thread IDs because they may be different
sed -e 's/thread [0-9]*/thread (IDs)/' -e 's/: .*\.c /: (file name) /' \
    -e 's/line [0-9]*/line (number)/' \
    -e 's/Time: [0-9]*\.[0-9]*/Time:  XXXX/' \
    -e 's/v[1-9]*\.[0-9]*\./version (number)\./' \
    -e 's/[1-9]*\.[0-9]*\.[0-9]*[^)]*/version (number)/' \
    JUnit-TestH4DFRparams.ext > JUnit-TestH4DFRparams.out

if diff JUnit-TestH4DFRparams.out JUnit-TestH4DFRparams.txt > /dev/null; then
    echo "  PASSED      JUnit-TestH4DFRparams"
else
    echo "**FAILED**    JUnit-TestH4DFRparams"
    echo "    Expected result differs from actual result"
    nerrors="`expr $nerrors + 1`"
    test yes = "$verbose" && $DIFF JUnit-TestH4DFRparams.txt JUnit-TestH4DFRparams.out |sed 's/^/    /'
fi

echo "$JAVAEXE $JAVAEXEFLAGS -Xmx1024M -Dorg.slf4j.simpleLogger.defaultLog=trace -Djava.library.path=$BLDLIBDIR -cp $CLASSPATH -ea org.junit.runner.JUnitCore test.TestH4GRparams"
TESTING JUnit-TestH4GRparams
($RUNSERIAL $JAVAEXE $JAVAEXEFLAGS -Xmx1024M -Dorg.slf4j.simpleLogger.defaultLog=trace -Djava.library.path=$BLDLIBDIR -cp $CLASSPATH -ea org.junit.runner.JUnitCore test.TestH4GRparams > JUnit-TestH4GRparams.ext)

# Extract file name, line number, version and thread IDs because they may be different
sed -e 's/thread [0-9]*/thread (IDs)/' -e 's/: .*\.c /: (file name) /' \
    -e 's/line [0-9]*/line (number)/' \
    -e 's/Time: [0-9]*\.[0-9]*/Time:  XXXX/' \
    -e 's/v[1-9]*\.[0-9]*\./version (number)\./' \
    -e 's/[1-9]*\.[0-9]*\.[0-9]*[^)]*/version (number)/' \
    JUnit-TestH4GRparams.ext > JUnit-TestH4GRparams.out

if diff JUnit-TestH4GRparams.out JUnit-TestH4GRparams.txt > /dev/null; then
    echo "  PASSED      JUnit-TestH4GRparams"
else
    echo "**FAILED**    JUnit-TestH4GRparams"
    echo "    Expected result differs from actual result"
    nerrors="`expr $nerrors + 1`"
    test yes = "$verbose" && $DIFF JUnit-TestH4GRparams.txt JUnit-TestH4GRparams.out |sed 's/^/    /'
fi

echo "$JAVAEXE $JAVAEXEFLAGS -Xmx1024M -Dorg.slf4j.simpleLogger.defaultLog=trace -Djava.library.path=$BLDLIBDIR -cp $CLASSPATH -ea org.junit.runner.JUnitCore test.TestH4HCparams"
TESTING JUnit-TestH4HCparams
($RUNSERIAL $JAVAEXE $JAVAEXEFLAGS -Xmx1024M -Dorg.slf4j.simpleLogger.defaultLog=trace -Djava.library.path=$BLDLIBDIR -cp $CLASSPATH -ea org.junit.runner.JUnitCore test.TestH4HCparams > JUnit-TestH4HCparams.ext)

# Extract file name, line number, version and thread IDs because they may be different
sed -e 's/thread [0-9]*/thread (IDs)/' -e 's/: .*\.c /: (file name) /' \
    -e 's/line [0-9]*/line (number)/' \
    -e 's/Time: [0-9]*\.[0-9]*/Time:  XXXX/' \
    -e 's/v[1-9]*\.[0-9]*\./version (number)\./' \
    -e 's/[1-9]*\.[0-9]*\.[0-9]*[^)]*/version (number)/' \
    JUnit-TestH4HCparams.ext > JUnit-TestH4HCparams.out

if diff JUnit-TestH4HCparams.out JUnit-TestH4HCparams.txt > /dev/null; then
    echo "  PASSED      JUnit-TestH4HCparams"
else
    echo "**FAILED**    JUnit-TestH4HCparams"
    echo "    Expected result differs from actual result"
    nerrors="`expr $nerrors + 1`"
    test yes = "$verbose" && $DIFF JUnit-TestH4HCparams.txt JUnit-TestH4HCparams.out |sed 's/^/    /'
fi

echo "$JAVAEXE $JAVAEXEFLAGS -Xmx1024M -Dorg.slf4j.simpleLogger.defaultLog=trace -Djava.library.path=$BLDLIBDIR -cp $CLASSPATH -ea org.junit.runner.JUnitCore test.TestH4SDparams"
TESTING JUnit-TestH4SDparams
($RUNSERIAL $JAVAEXE $JAVAEXEFLAGS -Xmx1024M -Dorg.slf4j.simpleLogger.defaultLog=trace -Djava.library.path=$BLDLIBDIR -cp $CLASSPATH -ea org.junit.runner.JUnitCore test.TestH4SDparams > JUnit-TestH4SDparams.ext)

# Extract file name, line number, version and thread IDs because they may be different
sed -e 's/thread [0-9]*/thread (IDs)/' -e 's/: .*\.c /: (file name) /' \
    -e 's/line [0-9]*/line (number)/' \
    -e 's/Time: [0-9]*\.[0-9]*/Time:  XXXX/' \
    -e 's/v[1-9]*\.[0-9]*\./version (number)\./' \
    -e 's/[1-9]*\.[0-9]*\.[0-9]*[^)]*/version (number)/' \
    JUnit-TestH4SDparams.ext > JUnit-TestH4SDparams.out

if diff JUnit-TestH4SDparams.out JUnit-TestH4SDparams.txt > /dev/null; then
    echo "  PASSED      JUnit-TestH4SDparams"
else
    echo "**FAILED**    JUnit-TestH4SDparams"
    echo "    Expected result differs from actual result"
    nerrors="`expr $nerrors + 1`"
    test yes = "$verbose" && $DIFF JUnit-TestH4SDparams.txt JUnit-TestH4SDparams.out |sed 's/^/    /'
fi

echo "$JAVAEXE $JAVAEXEFLAGS -Xmx1024M -Dorg.slf4j.simpleLogger.defaultLog=trace -Djava.library.path=$BLDLIBDIR -cp $CLASSPATH -ea org.junit.runner.JUnitCore test.TestH4Vparams"
TESTING JUnit-TestH4Vparams
($RUNSERIAL $JAVAEXE $JAVAEXEFLAGS -Xmx1024M -Dorg.slf4j.simpleLogger.defaultLog=trace -Djava.library.path=$BLDLIBDIR -cp $CLASSPATH -ea org.junit.runner.JUnitCore test.TestH4Vparams > JUnit-TestH4Vparams.ext)

# Extract file name, line number, version and thread IDs because they may be different
sed -e 's/thread [0-9]*/thread (IDs)/' -e 's/: .*\.c /: (file name) /' \
    -e 's/line [0-9]*/line (number)/' \
    -e 's/Time: [0-9]*\.[0-9]*/Time:  XXXX/' \
    -e 's/v[1-9]*\.[0-9]*\./version (number)\./' \
    -e 's/[1-9]*\.[0-9]*\.[0-9]*[^)]*/version (number)/' \
    JUnit-TestH4Vparams.ext > JUnit-TestH4Vparams.out

if diff JUnit-TestH4Vparams.out JUnit-TestH4Vparams.txt > /dev/null; then
    echo "  PASSED      JUnit-TestH4Vparams"
else
    echo "**FAILED**    JUnit-TestH4Vparams"
    echo "    Expected result differs from actual result"
    nerrors="`expr $nerrors + 1`"
    test yes = "$verbose" && $DIFF JUnit-TestH4Vparams.txt JUnit-TestH4Vparams.out |sed 's/^/    /'
fi

echo "$JAVAEXE $JAVAEXEFLAGS -Xmx1024M -Dorg.slf4j.simpleLogger.defaultLog=trace -Djava.library.path=$BLDLIBDIR -cp $CLASSPATH -ea org.junit.runner.JUnitCore test.TestH4VSparams"
TESTING JUnit-TestH4VSparams
($RUNSERIAL $JAVAEXE $JAVAEXEFLAGS -Xmx1024M -Dorg.slf4j.simpleLogger.defaultLog=trace -Djava.library.path=$BLDLIBDIR -cp $CLASSPATH -ea org.junit.runner.JUnitCore test.TestH4VSparams > JUnit-TestH4VSparams.ext)

# Extract file name, line number, version and thread IDs because they may be different
sed -e 's/thread [0-9]*/thread (IDs)/' -e 's/: .*\.c /: (file name) /' \
    -e 's/line [0-9]*/line (number)/' \
    -e 's/Time: [0-9]*\.[0-9]*/Time:  XXXX/' \
    -e 's/v[1-9]*\.[0-9]*\./version (number)\./' \
    -e 's/[1-9]*\.[0-9]*\.[0-9]*[^)]*/version (number)/' \
    JUnit-TestH4VSparams.ext > JUnit-TestH4VSparams.out

if diff JUnit-TestH4VSparams.out JUnit-TestH4VSparams.txt > /dev/null; then
    echo "  PASSED      JUnit-TestH4VSparams"
else
    echo "**FAILED**    JUnit-TestH4VSparams"
    echo "    Expected result differs from actual result"
    nerrors="`expr $nerrors + 1`"
    test yes = "$verbose" && $DIFF JUnit-TestH4VSparams.txt JUnit-TestH4VSparams.out |sed 's/^/    /'
fi


# Clean up temporary files/directories
CLEAN_LIBFILES_AND_BLDLIBDIR
CLEAN_DATAFILES_AND_BLDDIR

# Report test results and exit
if test $nerrors -eq 0 ; then
    echo "All $TESTNAME tests passed."
    exit $EXIT_SUCCESS
else
    echo "$TESTNAME tests failed with $nerrors errors."
    exit $EXIT_FAILURE
fi
