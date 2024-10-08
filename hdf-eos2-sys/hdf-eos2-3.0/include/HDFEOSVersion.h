#include <stdlib.h>
#include <HE2_config.h>

/* define strings that will be displayed by the using the UNIX "what" command
   on a file containing these strings */
  
#ifndef EOS2_AUTO_CONFIG
#define  HDFEOSd_BANNER  "@(#)## =================  HDFEOS  ================"
#define  HDFEOSd_HDFEOS_VER  "@(#)## HDFEOS Version: "HDFEOSVERSION1
#define  HDFEOSd_DATE    "@(#)## Build date: "__DATE__" @ "__TIME__
 
const char *hdfeosg_LibraryVersionString01 = HDFEOSd_BANNER;
const char *hdfeosg_LibraryVersionString02 = HDFEOSd_HDFEOS_VER;
const char *hdfeosg_LibraryVersionString03 = HDFEOSd_DATE;
const char *hdfeosg_LibraryVersionString04 = HDFEOSd_BANNER;
#else
const char *hdfeosg_LibraryVersionString01 = "=================  HDFEOS  ================";
const char *hdfeosg_LibraryVersionString02 = "HDFEOS Version: 2.19";
const char *hdfeosg_LibraryVersionString03 = "Build date: check lib date";
const char *hdfeosg_LibraryVersionString04 = "=================  HDFEOS  ================";
#endif
