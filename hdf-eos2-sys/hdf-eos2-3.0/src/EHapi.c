/*
Copyright (C) 1996 Hughes and Applied Research Corporation

Permission to use, modify, and distribute this software and its documentation 
for any purpose without fee is hereby granted, provided that the above 
copyright notice appear in all copies and that both that copyright notice and 
this permission notice appear in supporting documentation.
*/

#include <errno.h>
#include <math.h>

#include "mfhdf.h"
#include "HdfEosDef.h"

#ifdef FORTRAN_API
#include "fortran_api.h"
#endif // FORTRAN_API



/* Set maximun number of HDF-EOS files to HDF limit (MAX_FILE) */
/* #define NEOSHDF MAX_FILE */

/* change NEOSHDF to 5 time MAX_FILE=200 that we used to set for ECS*/
#define NEOSHDF 1000

/* these should be the same numbers as in GDapi.c and SWapi.c */
#define NGRID 800
#define NSWATH 400

uint8 EHXtypeTable[NEOSHDF];
uint8 EHXacsTable[NEOSHDF];
int32 EHXfidTable[NEOSHDF];
int32 EHXsdTable[NEOSHDF];

/* define a macro for the string size of the utility strings and some dimension
   list strings. The value in previous versions of this code may not be 
   enough in some cases. The length now is 512 which seems to be more than 
   enough to hold larger strings. */
   
#define UTLSTR_MAX_SIZE 512
#define UTLSTRSIZE  32000

#define EHIDOFFSET 524288

#define HDFEOSVERSION 2.20
#define HDFEOSVERSION1 "2.20"
#include <HDFEOSVersion.h>

#define MAX_RETRIES 10


/* Function Prototypes */
intn EHmetalist(char *, char *);


/*----------------------------------------------------------------------------|
|  BEGIN_PROLOG                                                               |
|                                                                             |
|  FUNCTION: EHopen                                                           |
|                                                                             |
|  DESCRIPTION: Opens HDF-EOS file and returns file handle                    |
|                                                                             |
|                                                                             |
|  Return Value    Type     Units     Description                             |
|  ============   ======  =========   =====================================   |
|  fid            int32               HDF-EOS file ID                         |
|                                                                             |
|  INPUTS:                                                                    |
|  filename       char                Filename                                |
|  access         intn                HDF access code                         |
|                                                                             |
|  OUTPUTS:                                                                   |
|             None                                                            |
|                                                                             |
|  NOTES:                                                                     |
|                                                                             |
|                                                                             |
|   Date     Programmer   Description                                         |
|  ======   ============  =================================================   |
|  Jun 96   Joel Gales    Original Programmer                                 |
|  Jul 96   Joel Gales    Add file id offset EHIDOFFSET                       |
|  Aug 96   Joel Gales    Add "END" statement to structural metadata           |
|  Sep 96   Joel Gales    Reverse order of Hopen ane SDstart statements       |
|                         for RDWR and READ access                            |
|  Oct 96   Joel Gales    Trap CREATE & RDWR (no write permission)            |
|                         access errors                                       |
|  Apr 97   Joel Gales    Fix problem with RDWR open when file previously     |
|                         open for READONLY access                            |
|  May 15   Abe Taaheri   Modified to reset max number of HDF4 files that     |
|                         can be left open in the process                     |
|                                                                             |
|  END_PROLOG                                                                 |
-----------------------------------------------------------------------------*/
int32
EHopen(const char *filename, intn access)

{
    intn            i;		/* Loop index */
    intn            status = 0;	/* routine return status variable */
    intn            dum;	/* Dummy variable */

    int32           HDFfid = 0;	/* HDF file ID */
    int32           fid = -1;	/* HDF-EOS file ID */
    int32           sdInterfaceID = 0;	/* HDF SDS interface ID */
    int32           nfileopen = 0;	/* # of HDF files open */
    int32           attrIndex;	/* Structural Metadata attribute index */

    uint8           acs = 0;	/* Read (0) / Write (1) access code */

    char           *testname;	/* Test filename */
    char            errbuf[UTLSTR_MAX_SIZE];/* Error report buffer */
    char           *metabuf;	/* Pointer to structural metadata buffer */
    char            hdfeosVersion[32];	/* HDFEOS version string */

    intn            retryCount;
    intn            req_hdf_max, curr_hdf_max, sys_limit;
    intn            num_open_hdf_files;


    /* Determine number of files currently opened */
    /* ------------------------------------------ */
    for (i = 0; i < NEOSHDF; i++)
    {
	nfileopen += EHXtypeTable[i];
    }

    status = SDget_maxopenfiles(&curr_hdf_max, &sys_limit);
    num_open_hdf_files= SDget_numopenfiles();

    if(status == 0)
      {
	if(nfileopen > (curr_hdf_max-2))
	  {
	    req_hdf_max = 2 * curr_hdf_max;
	    if(req_hdf_max > NEOSHDF) req_hdf_max = NEOSHDF;
	    status = SDreset_maxopenfiles(req_hdf_max);
	  }
      }

     /* Setup file interface */
    /* -------------------- */
    if (nfileopen < NEOSHDF)
    {

	/*
	 * Check that file has not been previously opened for write access if
	 * current open request is not READONLY
	 */
	if (access != DFACC_READ)
	{
	    /* Loop through all files */
	    /* ---------------------- */
	    for (i = 0; i < NEOSHDF; i++)
	    {
		/* if entry is active file opened for write access ... */
		/* --------------------------------------------------- */
		if (EHXtypeTable[i] != 0 && EHXacsTable[i] == 1)
		{
		    /* Get filename (testname) */
		    /* ----------------------- */
		    Hfidinquire(EHXfidTable[i], &testname, &dum, &dum);


		    /* if same as filename then report error */
		    /* ------------------------------------- */
		    if (strcmp(testname, filename) == 0)
		    {
			status = -1;
			fid = -1;
			HEpush(DFE_ALROPEN, "EHopen", __FILE__, __LINE__);
			HEreport("\"%s\" already open.\n", filename);
			break;
		    }
		}
	    }
	}
	if (status == 0)
	{
	    /* Create HDF-EOS file */
	    /* ------------------- */
	    switch (access)
	    {
	    case DFACC_CREATE:

		/* Get SDS interface ID */
		/* -------------------- */
		sdInterfaceID = SDstart(filename, DFACC_CREATE);

		/* If SDstart successful ... */
		/* ------------------------- */
		if (sdInterfaceID != -1)
		{
		    /* Set HDFEOS version number in file */
		    /* --------------------------------- */
		  snprintf(hdfeosVersion, sizeof(hdfeosVersion), "%s%s", "HDFEOS_V",
			    HDFEOSVERSION1);
		    SDsetattr(sdInterfaceID, "HDFEOSVersion", DFNT_CHAR8,
			      (int)strlen(hdfeosVersion), hdfeosVersion);


		    /* Get HDF file ID */
		    /* --------------- */
		    HDFfid = Hopen(filename, DFACC_RDWR, 0);

		    /* Set open access to write */
		    /* ------------------------ */
		    acs = 1;

		    /* Setup structural metadata */
		    /* ------------------------- */
		    metabuf = (char *) calloc(32000, 1);
		    if(metabuf == NULL)
		    { 
			HEpush(DFE_NOSPACE,"EHopen", __FILE__, __LINE__);
			return(-1);
		    }

		    strcpy(metabuf, "GROUP=SwathStructure\n");
		    strcat(metabuf, "END_GROUP=SwathStructure\n");
		    strcat(metabuf, "GROUP=GridStructure\n");
		    strcat(metabuf, "END_GROUP=GridStructure\n");
		    strcat(metabuf, "GROUP=PointStructure\n");
		    strcat(metabuf, "END_GROUP=PointStructure\n");
		    strcat(metabuf, "END\n");

		    /* Write Structural metadata */
		    /* ------------------------- */
		    SDsetattr(sdInterfaceID, "StructMetadata.0",
			      DFNT_CHAR8, 32000, metabuf);
		    free(metabuf);
		} else
		{
		    /* If error in SDstart then report */
		    /* ------------------------------- */
		    fid = -1;
		    status = -1;
		    HEpush(DFE_FNF, "EHopen", __FILE__, __LINE__);
		    snprintf(errbuf, sizeof(errbuf), "%s%s%s", "\"", filename,
			    "\" cannot be created.");
		    HEreport("%s\n", errbuf);
		}

		break;

		/* Open existing HDF-EOS file for read/write access */
		/* ------------------------------------------------ */
	    case DFACC_RDWR:

		/* Get HDF file ID */
		/* --------------- */
#ifndef _PGS_OLDNFS
/* The following loop around the function Hopen is intended to deal with the NFS cache 
   problem when opening file fails with errno = 150 or 151. When NFS cache is updated,
   this part of change is no longer necessary.              10/18/1999   */
                retryCount = 0;
                HDFfid = -1;
                while ((HDFfid == -1) && (retryCount < MAX_RETRIES))
                {
                HDFfid = Hopen(filename, DFACC_RDWR, 0);
                if((HDFfid == -1) && (errno == 150 || errno == 151))
                    {
                    HEpush(DFE_FNF, "EHopen", __FILE__, __LINE__);
                    snprintf(errbuf, sizeof(errbuf), "\"%s\" cannot be opened for READ/WRITE access, will retry %d times.", filename,  (MAX_RETRIES - retryCount - 1));
                    HEreport("%s\n", errbuf);
		    /*
                    #ifndef VISUAL_CPLUSPLUS
                    sleep(1);
                    #endif
		    */
                    }
                retryCount++;
                }
#else
                HDFfid = Hopen(filename, DFACC_RDWR, 0);
#endif

		/* If Hopen successful ... */
		/* ----------------------- */
		if (HDFfid != -1)
		{
		    /* Get SDS interface ID */
		    /* -------------------- */
		    sdInterfaceID = SDstart(filename, DFACC_RDWR);

                    /* If SDstart successful ... */
                    /* ------------------------- */
                    if (sdInterfaceID != -1)
                    {
                       /* Set HDFEOS version number in file */
                       /* --------------------------------- */
		      
		      attrIndex = SDfindattr(sdInterfaceID, "HDFEOSVersion");
		      if (attrIndex == -1)
			{
			  snprintf(hdfeosVersion, sizeof(hdfeosVersion), "%s%s", "HDFEOS_V",
				  HDFEOSVERSION1);
			  SDsetattr(sdInterfaceID, "HDFEOSVersion", DFNT_CHAR8,
				    (int)strlen(hdfeosVersion), hdfeosVersion);
			}
		       /* Set open access to write */
		       /* ------------------------ */
		       acs = 1;

		       /* Get structural metadata attribute ID */
		       /* ------------------------------------ */
		       attrIndex = SDfindattr(sdInterfaceID, "StructMetadata.0");

		       /* Write structural metadata if it doesn't exist */
		       /* --------------------------------------------- */
		       if (attrIndex == -1)
		       {
		   	  metabuf = (char *) calloc(32000, 1);
			  if(metabuf == NULL)
			  { 
			      HEpush(DFE_NOSPACE,"EHopen", __FILE__, __LINE__);
			      return(-1);
			  }

			  strcpy(metabuf, "GROUP=SwathStructure\n");
			  strcat(metabuf, "END_GROUP=SwathStructure\n");
			  strcat(metabuf, "GROUP=GridStructure\n");
			  strcat(metabuf, "END_GROUP=GridStructure\n");
			  strcat(metabuf, "GROUP=PointStructure\n");
			  strcat(metabuf, "END_GROUP=PointStructure\n");
			  strcat(metabuf, "END\n");

			  SDsetattr(sdInterfaceID, "StructMetadata.0",
				  DFNT_CHAR8, 32000, metabuf);
			  free(metabuf);
		       }
                    } else
                    {
                        /* If error in SDstart then report */
                        /* ------------------------------- */
                        fid = -1;
                        status = -1;
                        HEpush(DFE_FNF, "EHopen", __FILE__, __LINE__);
                        snprintf(errbuf, sizeof(errbuf), "%s%s%s", "\"", filename,
                            "\" cannot be opened for read/write access.");
                        HEreport("%s\n", errbuf);
                    }
		} else
		{
		    /* If error in Hopen then report */
		    /* ----------------------------- */
		    fid = -1;
		    status = -1;
		    HEpush(DFE_FNF, "EHopen", __FILE__, __LINE__);
		    snprintf(errbuf, sizeof(errbuf), "%s%s%s", "\"", filename,
			    "\" cannot be opened for RDWR access.");
		    HEreport("%s\n", errbuf);
		}

		break;


		/* Open existing HDF-EOS file for read-only access */
		/* ----------------------------------------------- */
	    case DFACC_READ:

		/* Get HDF file ID */
		/* --------------- */
#ifndef _PGS_OLDNFS
/* The following loop around the function Hopen is intended to deal with the NFS cache 
   problem when opening file fails with errno = 150 or 151. When NFS cache is updated,
   this part of change is no longer necessary.              10/18/1999   */
                retryCount = 0;
                HDFfid = -1;
                while ((HDFfid == -1) && (retryCount < MAX_RETRIES))
                {
                HDFfid = Hopen(filename, DFACC_READ, 0);
                if((HDFfid == -1) && (errno == 150 || errno == 151))
                    {
                    HEpush(DFE_FNF, "EHopen", __FILE__, __LINE__);
                    snprintf(errbuf, sizeof(errbuf), "\"%s\" cannot be opened for READONLY access, will retry %d times.", filename,  (MAX_RETRIES - retryCount - 1));
                    HEreport("%s\n", errbuf);
		    /*
                    #ifndef VISUAL_CPLUSPLUS
                    sleep(1);
                    #endif
		    */
                    }
                retryCount++;
                }
#else
                HDFfid = Hopen(filename, DFACC_READ, 0);
#endif

		/* If file does not exist report error */
		/* ----------------------------------- */
		if (HDFfid == -1)
		{
		    fid = -1;
		    status = -1;
		    HEpush(DFE_FNF, "EHopen", __FILE__, __LINE__);
		    strcpy(errbuf, "\"");
		    strcat(errbuf, filename);
		    strcat(errbuf, "\" (opened for READONLY access)");
		    strcat(errbuf, " does not exist.");
		    HEreport("%s\n", errbuf);
		} else
		{
		    /* If file exists then get SD interface ID */
		    /* --------------------------------------- */
		    sdInterfaceID = SDstart(filename, DFACC_RDONLY);

                        /* If SDstart successful ... */
                        /* ------------------------- */
                        if (sdInterfaceID != -1)
                        {
 
		           /* Set open access to read-only */
		           /* ---------------------------- */
		           acs = 0;
		         } else
                        {
                            /* If error in SDstart then report */
                            /* ------------------------------- */
                            fid = -1;
                            status = -1;
                            HEpush(DFE_FNF, "EHopen", __FILE__, __LINE__);
                            snprintf(errbuf, sizeof(errbuf), "%s%s%s", "\"", filename,
                            "\" cannot be opened for read access.");
                            HEreport("%s\n", errbuf);
                        }
		}

		break;

	    default:
		/* Invalid Access Code */
		/* ------------------- */
		fid = -1;
		status = -1;
		HEpush(DFE_BADACC, "EHopen", __FILE__, __LINE__);
		HEreport("Access Code: %d (%s).\n", access, filename);
	    }

	}
    } else
    {
	/* Too many files opened */
	/* --------------------- */
	status = -1;
	fid = -1;
	HEpush(DFE_TOOMANY, "EHopen", __FILE__, __LINE__);
	HEreport("No more than %d files may be open simultaneously (%s).\n",
		 NEOSHDF, filename);
    }




    if (status == 0)
    {
	/* Initialize Vgroup Access */
	/* ------------------------ */
	Vstart(HDFfid);


	/* Assign HDFEOS fid # & Load HDF fid and sdInterfaceID tables */
	/* ----------------------------------------------------------- */
	for (i = 0; i < NEOSHDF; i++)
	{
	    if (EHXtypeTable[i] == 0)
	    {
		fid = i + EHIDOFFSET;
		EHXacsTable[i] = acs;
		EHXtypeTable[i] = 1;
		EHXfidTable[i] = HDFfid;
		EHXsdTable[i] = sdInterfaceID;
		break;
	    }
	}

    }
    return (fid);
}


/*----------------------------------------------------------------------------|
|  BEGIN_PROLOG                                                               |
|  FUNCTION:      EHchkname                                                   |
|  DESCRIPTION :  Checks if the passed string doesn't contain illegal         |
|                                             characters                      |
|  Return Value    Type     Units     Description                             |
|  ============   ======  =========   =====================================   |
|                                                                             |
|  INPUTS:                                                                    |
|      p          void *              Pointer                                 |
|  OUTPUTS:                                                                   |
|                                                                             |
|  NOTES:                                                                     |
|   Date      Programmer    Description                                       |
|  ========   ============  ================================================= |
|  06/01/01   A.Muslimov    Original development.                             |
|  01/27/16   Abe Taaheri   Added for avoiding illegal chacaters in fieldname |
|                           as in HDF-EOS5                                    |
|  END_PROLOG                                                                 |
-----------------------------------------------------------------------------*/
intn   
EHchkname(const char *p)
{
  intn     status = SUCCEED;
  char     errbuf[128];
 
  if(p == NULL)
    {
      status = -1;
      snprintf(errbuf, sizeof(errbuf), "HDF-EOS WARNING: The input name is a null pointer.\n");
      HEpush(DFE_GENAPP, "EHchkname", __FILE__, __LINE__);
      HEreport(errbuf);
    }
  /* this function and its use was added to hdfeos in early 2016. 
     It was noticed that MISR has already produced some products 
     that has "/" in the field name. So we need to remove checking
     "/" below to support those products. */
  /*if( strchr(p,',') || strchr(p,';') || strchr(p,'/')  || strchr(p,':'))*/
    if( strchr(p,',') || strchr(p,';'))
    {
      status = -1;
      snprintf(errbuf, sizeof(errbuf), "HDF-EOS WARNING: The input string named \"%s\" contains illegal character.\n", p);
      HEpush(DFE_GENAPP, "EHchkname", __FILE__, __LINE__);
      HEreport(errbuf);
    }
  
  return(status);
}



/*----------------------------------------------------------------------------|
|  BEGIN_PROLOG                                                               |
|                                                                             |
|  FUNCTION: EHchkfid                                                         |
|                                                                             |
|  DESCRIPTION: Checks for valid file id and returns HDF file ID and          |
|               SD interface ID                                               |
|                                                                             |
|                                                                             |
|  Return Value    Type     Units     Description                             |
|  ============   ======  =========   =====================================   |
|  status         intn                return status (0) SUCCEED, (-1) FAIL    |
|                                                                             |
|  INPUTS:                                                                    |
|  fid            int32               HDF-EOS file ID                         |
|  name           char                Structure name                          |
|                                                                             |
|  OUTPUTS:                                                                   |
|  HDFfid         int32               HDF File ID                             |
|  sdInterfaceID  int32               SDS interface ID                        |
|  access         uint8               access code                             |
|                                                                             |
|  NOTES:                                                                     |
|                                                                             |
|                                                                             |
|   Date     Programmer   Description                                         |
|  ======   ============  =================================================   |
|  Jun 96   Joel Gales    Original Programmer                                 |
|  Jul 96   Joel Gales    set status=-1 if failure                            |
|  Jul 96   Joel Gales    Add file id offset EHIDOFFSET                       |
|                                                                             |
|  END_PROLOG                                                                 |
-----------------------------------------------------------------------------*/
intn
EHchkfid(int32 fid, const char *name, int32 * HDFfid, int32 * sdInterfaceID,
	 uint8 * access)

{
    intn            status = 0;	/* routine return status variable */
    intn            fid0;	/* HDFEOS file ID - Offset */


    /* Check for valid HDFEOS file ID range */
    /* ------------------------------------ */
    if (fid < EHIDOFFSET || fid > NEOSHDF + EHIDOFFSET)
    {
	status = -1;
	HEpush(DFE_RANGE, "EHchkfid", __FILE__, __LINE__);
	HEreport("Invalid file id: %d.  ID must be >= %d and < %d (%s).\n",
		 fid, EHIDOFFSET, NEOSHDF + EHIDOFFSET, name);
    } else
    {
	/* Compute "reduced" file ID */
	/* ------------------------- */
	fid0 = fid % EHIDOFFSET;


	/* Check that HDFEOS file ID is active */
	/* ----------------------------------- */
	if (EHXtypeTable[fid0] == 0)
	{
	    status = -1;
	    HEpush(DFE_GENAPP, "EHchkfid", __FILE__, __LINE__);
	    HEreport("File id %d not active (%s).\n", fid, name);
	} else
	{
	    /*
	     * Get HDF file ID, SD interface ID and file access from external
	     * arrays
	     */
	    *HDFfid = EHXfidTable[fid0];
	    *sdInterfaceID = EHXsdTable[fid0];
	    *access = EHXacsTable[fid0];
	}
    }

    return (status);
}




/*----------------------------------------------------------------------------|
|  BEGIN_PROLOG                                                               |
|                                                                             |
|  FUNCTION: EHidinfo                                                         |
|                                                                             |
|  DESCRIPTION: Gets Hopen and SD interface IDs from HDF-EOS id              |
|                                                                             |
|                                                                             |
|  Return Value    Type     Units     Description                             |
|  ============   ======  =========   =====================================   |
|  status         intn                return status (0) SUCCEED, (-1) FAIL    |
|                                                                             |
|  INPUTS:                                                                    |
|  fid            int32               HDF-EOS file ID                         |
|                                                                             |
|  OUTPUTS:                                                                   |
|  HDFfid         int32               HDF File ID                             |
|  sdInterfaceID  int32               SDS interface ID                        |
|                                                                             |
|  NOTES:                                                                     |
|                                                                             |
|                                                                             |
|   Date     Programmer   Description                                         |
|  ======   ============  =================================================   |
|  Jul 96   Joel Gales    Original Programmer                                 |
|                                                                             |
|  END_PROLOG                                                                 |
-----------------------------------------------------------------------------*/
intn
EHidinfo(int32 fid, int32 * HDFfid, int32 * sdInterfaceID)

{
    intn            status = 0;	/* routine return status variable */
    uint8           dum;	/* Dummy variable */

    /* Call EHchkfid to get HDF and SD interface IDs */
    /* --------------------------------------------- */
    status = EHchkfid(fid, "EHidinfo", HDFfid, sdInterfaceID, &dum);

    return (status);
}




/*----------------------------------------------------------------------------|
|  BEGIN_PROLOG                                                               |
|                                                                             |
|  FUNCTION: EHfilename                                                       |
|                                                                             |
|  DESCRIPTION: Returns HDF filename                                          |
|                                                                             |
|                                                                             |
|  Return Value    Type     Units     Description                             |
|  ============   ======  =========   =====================================   |
|  status         intn                return status (0) SUCCEED, (-1) FAIL    |
|                                                                             |
|  INPUTS:                                                                    |
|  fid            int32               HDF-EOS file id                         |
|                                                                             |
|  OUTPUTS:                                                                   |
|  filename       char                HDF-EOS file name                       |
|                                                                             |
|  NOTES:                                                                     |
|                                                                             |
|                                                                             |
|   Date     Programmer   Description                                         |
|  ======   ============  =================================================   |
|  Sep 96   Joel Gales    Original Programmer                                 |
|                                                                             |
|  END_PROLOG                                                                 |
-----------------------------------------------------------------------------*/
intn
EHfilename(int32 fid, char *filename)
{
    intn            status = 0;	/* routine return status variable */
    intn            dum;	/* Dummy variable */

    char           *fname;	/* Pointer to filename */

    /* Get point to filename from Hfidinquire */
    /* -------------------------------------- */
    Hfidinquire(EHXfidTable[fid % EHIDOFFSET], &fname, &dum, &dum);
    strcpy(filename, fname);

    return (status);
}




/*----------------------------------------------------------------------------|
|  BEGIN_PROLOG                                                               |
|                                                                             |
|  FUNCTION: EHgetversion                                                     |
|                                                                             |
|  DESCRIPTION: Returns HDF-EOS version string                                |
|                                                                             |
|                                                                             |
|  Return Value    Type     Units     Description                             |
|  ============   ======  =========   =====================================   |
|  status         intn                return status (0) SUCCEED, (-1) FAIL    |
|                                                                             |
|  INPUTS:                                                                    |
|  fid            int32               HDF-EOS file id                         |
|                                                                             |
|  OUTPUTS:                                                                   |
|  version        char                HDF-EOS version string                  |
|                                                                             |
|  NOTES:                                                                     |
|                                                                             |
|                                                                             |
|   Date     Programmer   Description                                         |
|  ======   ============  =================================================   |
|  Mar 97   Joel Gales    Original Programmer                                 |
|                                                                             |
|  END_PROLOG                                                                 |
-----------------------------------------------------------------------------*/
intn
EHgetversion(int32 fid, char *version)
{
    intn            status = 0;	/* routine return status variable */

    uint8           access;	/* Access code */
    int32           dum;	/* Dummy variable */
    int32           sdInterfaceID = 0;	/* HDF SDS interface ID */
    int32           attrIndex;	/* HDFEOS version attribute index */
    int32           count;	/* Version string size */

    char            attrname[16];	/* Attribute name */


    /* Get SDS interface ID */
    /* -------------------- */
    status = EHchkfid(fid, "EHgetversion", &dum, &sdInterfaceID, &access);


    /* Get attribute index number */
    /* -------------------------- */
    attrIndex = SDfindattr(sdInterfaceID, "HDFEOSVersion");


    /* Get attribute size */
    /* ------------------ */
    status = SDattrinfo(sdInterfaceID, attrIndex, attrname, &dum, &count);


    /* Read version attribute */
    /* ---------------------- */
    status = SDreadattr(sdInterfaceID, attrIndex, (VOIDP) version);


    /* Place string terminator on version string */
    /* ----------------------------------------- */
    version[count] = 0;


    return (status);
}


/*----------------------------------------------------------------------------|
|  BEGIN_PROLOG                                                               |
|                                                                             |
|  FUNCTION: EHconvAng                                                        |
|                                                                             |
|  DESCRIPTION: Angle conversion Utility                                      |
|                                                                             |
|                                                                             |
|  Return Value    Type     Units     Description                             |
|  ============   ======  =========   =====================================   |
|  outAngle       float64             Output Angle value                      |
|                                                                             |
|  INPUTS:                                                                    |
|  inAngle        float64             Input Angle value                       |
|  code           intn                Conversion code                         |
!                                       HDFE_RAD_DEG (0)                      |
|                                       HDFE_DEG_RAD (1)                      |
|                                       HDFE_DMS_DEG (2)                      |
|                                       HDFE_DEG_DMS (3)                      |
|                                       HDFE_RAD_DMS (4)                      |
|                                       HDFE_DMS_RAD (5)                      |
|                                                                             |
|  OUTPUTS:                                                                   |
|     None                                                                    |
|                                                                             |
|  NOTES:                                                                     |
|                                                                             |
|                                                                             |
|   Date     Programmer   Description                                         |
|  ======   ============  =================================================   |
|  Jun 96   Joel Gales    Original Programmer                                 |
|  Feb 97   Joel Gales    Correct "60" min & "60" sec in _DMS conversion      |
|                                                                             |
|  END_PROLOG                                                                 |
-----------------------------------------------------------------------------*/
float64
EHconvAng(float64 inAngle, intn code)
{
    int32           min = 0;	/* Truncated Minutes */
    int32           deg = 0;	/* Truncated Degrees */

    float64         sec = 0;	/* Seconds */
    float64         outAngle = 0;	/* Angle in desired units */
    float64         pi = 3.14159265358979324;	/* Pi */
    float64         r2d = 180 / pi;	/* Radians to degrees conversion */
    float64         d2r = 1 / r2d;	/* Degrees to radians conversion */


    switch (code)
    {

	/* Convert radians to degrees */
	/* -------------------------- */
    case HDFE_RAD_DEG:
	outAngle = inAngle * r2d;
	break;


	/* Convert degrees to radians */
	/* -------------------------- */
    case HDFE_DEG_RAD:
	outAngle = inAngle * d2r;
	break;


	/* Convert packed degrees to degrees */
	/* --------------------------------- */
    case HDFE_DMS_DEG:
        deg = (int32)(inAngle / 1000000);
	min = (int32)((inAngle - deg * 1000000) / 1000);
	sec = (inAngle - deg * 1000000 - min * 1000);
	outAngle = deg + min / 60.0 + sec / 3600.0;
	break;


	/* Convert degrees to packed degrees */
	/* --------------------------------- */
    case HDFE_DEG_DMS:
        deg = (int32)(inAngle);
        min = (int32)((inAngle - deg) * 60);
	sec = (inAngle - deg - min / 60.0) * 3600;
	/*
	if ((intn) sec == 60)
	{
	    sec = sec - 60;
	    min = min + 1;
	}
	*/

	if ( fabs(sec - 0.0) < 1.e-7 )
	  {
	    sec = 0.0;
	  }

        if ( (fabs(sec - 60) < 1.e-7 ) || ( sec > 60.0 ))
        {
	  sec = sec - 60;
	  min = min + 1;
	  if(sec < 0.0)
	    {
	      sec = 0.0;
	    }
        }

	if (min == 60)
	{
	    min = min - 60;
	    deg = deg + 1;
	}
	outAngle = deg * 1000000 + min * 1000 + sec;
	break;


	/* Convert radians to packed degrees */
	/* --------------------------------- */
    case HDFE_RAD_DMS:
	inAngle = inAngle * r2d;
	deg = (int32)(inAngle);
	min = (int32)((inAngle - deg) * 60);
	sec = (inAngle - deg - min / 60.0) * 3600;
	/*
	if ((intn) sec == 60)
	{
	    sec = sec - 60;
	    min = min + 1;
	}
	*/

	if ( fabs(sec - 0.0) < 1.e-7 )
	  {
	    sec = 0.0;
	  }

        if ( (fabs(sec - 60) < 1.e-7 ) || ( sec > 60.0 ))
        {
	  sec = sec - 60;
	  min = min + 1;
	  if(sec < 0.0)
	    {
	      sec = 0.0;
	    }
        }


	if (min == 60)
	{
	    min = min - 60;
	    deg = deg + 1;
	}
	outAngle = deg * 1000000 + min * 1000 + sec;
	break;


	/* Convert packed degrees to radians */
	/* --------------------------------- */
    case HDFE_DMS_RAD:
        deg = (int32)(inAngle / 1000000);
        min = (int32)((inAngle - deg * 1000000) / 1000);
	sec = (inAngle - deg * 1000000 - min * 1000);
	outAngle = deg + min / 60.0 + sec / 3600.0;
	outAngle = outAngle * d2r;
	break;
    }
    return (outAngle);
}






/*----------------------------------------------------------------------------|
|  BEGIN_PROLOG                                                               |
|                                                                             |
|  FUNCTION: EHparsestr                                                       |
|                                                                             |
|  DESCRIPTION: String Parser Utility                                         |
|                                                                             |
|                                                                             |
|  Return Value    Type     Units     Description                             |
|  ============   ======  =========   =====================================   |
|  count          int32               Number of string entries                |
|                                                                             |
|  INPUTS:                                                                    |
|  instring       char                Input string                            |
|  delim          char                string delimiter                        |
|                                                                             |
|  OUTPUTS:                                                                   |
|  pntr           char *              Pointer array to beginning of each      |
|                                     string entry                            |
|  len            int32               Array of string entry lengths           |
|                                                                             |
|  NOTES:                                                                     |
|                                                                             |
|                                                                             |
|   Date     Programmer   Description                                         |
|  ======   ============  =================================================   |
|  Jun 96   Joel Gales    Original Programmer                                 |
|  Aug 96   Joel Gales    NULL pointer array returns count only               |
|                                                                             |
|  END_PROLOG                                                                 |
-----------------------------------------------------------------------------*/
int32
EHparsestr(char *instring, char delim, char *pntr[], int32 len[])
{
    int32           i;		/* Loop index */
    int32           prevDelimPos = 0;	/* Previous delimiter position */
    int32           count;	/* Number of elements in string list */
    int32           slen;	/* String length */

    char           *delimitor;	/* Pointer to delimiter */

    /* Get length of input string list & Point to first delimiter */
    /* ---------------------------------------------------------- */
    slen = (int32)strlen(instring);
    delimitor = strchr(instring, delim);

    /* If NULL string set count to zero otherwise set to 1 */
    /* --------------------------------------------------- */
    count = (slen == 0) ? 0 : 1;


    /* if string pointers are requested set first one to beginning of string */
    /* --------------------------------------------------------------------- */
    if (&pntr[0] != NULL)
    {
	pntr[0] = instring;
    }
    /* If delimiter not found ... */
    /* -------------------------- */
    if (delimitor == NULL)
    {
	/* if string length requested then set to input string length */
	/* ---------------------------------------------------------- */
	if (len != NULL)
	{
	    len[0] = slen;
	}
    } else
	/* Delimitors Found */
	/* ---------------- */
    {
	/* Loop through all characters in string */
	/* ------------------------------------- */
	for (i = 1; i < slen; i++)
	{
	    /* If character is a delimiter ... */
	    /* ------------------------------- */
	    if (instring[i] == delim)
	    {

		/* If string pointer requested */
		/* --------------------------- */
		if (&pntr[0] != NULL)
		{
		    /* if requested then compute string length of entry */
		    /* ------------------------------------------------ */
		    if (len != NULL)
		    {
			len[count - 1] = i - prevDelimPos;
		    }
		    /* Point to beginning of string entry */
		    /* ---------------------------------- */
		    pntr[count] = instring + i + 1;
		}
		/* Reset previous delimiter position and increment counter */
		/* ------------------------------------------------------- */
		prevDelimPos = i + 1;
		count++;
	    }
	}

	/* Compute string length of last entry */
	/* ----------------------------------- */
	if (&pntr[0] != NULL && len != NULL)
	{
	    len[count - 1] = i - prevDelimPos;
	}
    }

    return (count);
}




/*----------------------------------------------------------------------------|
|  BEGIN_PROLOG                                                               |
|                                                                             |
|  FUNCTION: EHstrwithin                                                      |
|                                                                             |
|  DESCRIPTION: Searches for string within target string                       |
|                                                                             |
|                                                                             |
|  Return Value    Type     Units     Description                             |
|  ============   ======  =========   =====================================   |
|  indx           int32               Element index (0 - based)               |
|                                                                             |
|  INPUTS:                                                                    |
|  target         char                Target string                           |
|  search         char                Search string                           |
|  delim          char                Delimitor                               |
|                                                                             |
|  OUTPUTS:                                                                   |
|             None                                                            |
|                                                                             |
|  NOTES:                                                                     |
|                                                                             |
|                                                                             |
|   Date     Programmer   Description                                         |
|  ======   ============  =================================================   |
|  Jun 96   Joel Gales    Original Programmer                                 |
|  Jan 97   Joel Gales    Change ptr & slen to dynamic arrays                 |
|                                                                             |
|  END_PROLOG                                                                 |
-----------------------------------------------------------------------------*/
int32
EHstrwithin(const char *target, char *search, char delim)
{
    intn            found = 0;	/* Target string found flag */

    int32           indx;	/* Loop index */
    int32           nentries;	/* Number of entries in search string */
    int32          *slen;	/* Pointer to string length array */

    char          **ptr;	/* Pointer to string pointer array */
    char            buffer[128];/* Buffer to hold "test" string entry */


    /* Count number of entries in search string list */
    /* --------------------------------------------- */
    nentries = EHparsestr(search, delim, NULL, NULL);


    /* Allocate string pointer and length arrays */
    /* ----------------------------------------- */
    ptr = (char **) calloc(nentries, sizeof(char *));
    if(ptr == NULL)
    { 
	HEpush(DFE_NOSPACE,"EHstrwithin", __FILE__, __LINE__);
	return(-1);
    }
    slen = (int32 *) calloc(nentries, sizeof(int32));
    if(slen == NULL)
    { 
	HEpush(DFE_NOSPACE,"EHstrwithin", __FILE__, __LINE__);
	free(ptr);
	return(-1);
    }


    /* Parse search string */
    /* ------------------- */
    nentries = EHparsestr(search, delim, ptr, slen);


    /* Loop through all elements in search string list */
    /* ----------------------------------------------- */
    for (indx = 0; indx < nentries; indx++)
    {
	/* Copy string entry into buffer */
	/* ----------------------------- */
	memcpy(buffer, ptr[indx], slen[indx]);
	buffer[slen[indx]] = 0;


	/* Compare target string with string entry */
	/* --------------------------------------- */
	if (strcmp(target, buffer) == 0)
	{
	    found = 1;
	    break;
	}
    }

    /* If not found set return to -1 */
    /* ----------------------------- */
    if (found == 0)
    {
	indx = -1;
    }
    free(slen);
    free(ptr);

    return (indx);
}





/*----------------------------------------------------------------------------|
|  BEGIN_PROLOG                                                               |
|                                                                             |
|  FUNCTION: EHloadliststr                                                    |
|                                                                             |
|  DESCRIPTION: Builds list string from string array                          |
|                                                                             |
|                                                                             |
|  Return Value    Type     Units     Description                             |
|  ============   ======  =========   =====================================   |
|  status         intn                return status (0) SUCCEED, (-1) FAIL    |
|                                                                             |
|  INPUTS:                                                                    |
|  ptr            char                String pointer array                    |
|  nentries       int32               Number of string array elements         |
|  delim          char                Delimitor                               |
|                                                                             |
|  OUTPUTS:                                                                   |
|  liststr        char                Output list string                      |
|                                                                             |
|  NOTES:                                                                     |
|                                                                             |
|                                                                             |
|   Date     Programmer   Description                                         |
|  ======   ============  =================================================   |
|  Jun 96   Joel Gales    Original Programmer                                 |
|                                                                             |
|  END_PROLOG                                                                 |
-----------------------------------------------------------------------------*/
intn
EHloadliststr(char *ptr[], int32 nentries, char *liststr, char delim)
{
    intn            status = 0;	/* routine return status variable */

    int32           i;		/* Loop index */
    int32           slen;	/* String entry length */
    int32           off = 0;	/* Position of next entry along string list */
    char            dstr[2];    /* string version of input variable "delim" */

    dstr[0] = delim;
    dstr[1] = '\0';


    /* Loop through all entries in string array */
    /* ---------------------------------------- */
    for (i = 0; i < nentries; i++)
    {
	/* Get string length of string array entry */
	/* --------------------------------------- */
	slen = (int32)strlen(ptr[i]);


	/* Copy string entry to string list */
	/* -------------------------------- */
	memcpy(liststr + off, ptr[i], slen + 1);


	/* Concatenate with delimiter */
	/* -------------------------- */
	if (i != nentries - 1)
	{
	    strcat(liststr, dstr);
	}
	/* Get position of next entry for string list */
	/* ------------------------------------------ */
	off += slen + 1;
    }

    return (status);
}





/*----------------------------------------------------------------------------|
|  BEGIN_PROLOG                                                               |
|                                                                             |
|  FUNCTION: EHgetid                                                          |
|                                                                             |
|  DESCRIPTION: Get Vgroup/Vdata ID from name                                 |
|                                                                             |
|                                                                             |
|  Return Value    Type     Units     Description                             |
|  ============   ======  =========   =====================================   |
|  outID          int32               Output ID                               |
|                                                                             |
|  INPUTS:                                                                    |
|  fid            int32               HDF-EOS file ID                         |
|  vgid           int32               Vgroup ID                               |
|  objectname     char                object name                             |
|  code           intn                object code (0 - Vgroup, 1 - Vdata)     |
|  access         char                access ("w/r")                          |
|                                                                             |
|                                                                             |
|  OUTPUTS:                                                                   |
|             None                                                            |
|                                                                             |
|  NOTES:                                                                     |
|                                                                             |
|                                                                             |
|   Date     Programmer   Description                                         |
|  ======   ============  =================================================   |
|  Jun 96   Joel Gales    Original Programmer                                 |
|                                                                             |
|  END_PROLOG                                                                 |
-----------------------------------------------------------------------------*/
int32
EHgetid(int32 fid, int32 vgid, const char *objectname, intn code, const char *l_access)
{
    intn            i;		/* Loop index */

    int32           nObjects;	/* # of objects in Vgroup */
    int32          *tags;	/* Pnt to Vgroup object tags array */
    int32          *refs;	/* Pnt to Vgroup object refs array */
    int32           id;		/* Object ID */
    int32           outID = -1;	/* Desired object ID */

    char            name[UTLSTR_MAX_SIZE];	/* Object name */


    /* Get Number of objects */
    /* --------------------- */
    nObjects = Vntagrefs(vgid);

    /* If objects exist ... */
    /* -------------------- */
    if (nObjects != 0)
    {

	/* Get tags and references of objects */
	/* ---------------------------------- */
	tags = (int32 *) malloc(sizeof(int32) * nObjects);
	if(tags == NULL)
	{ 
	    HEpush(DFE_NOSPACE,"EHgetid", __FILE__, __LINE__);
	    return(-1);
	}
	refs = (int32 *) malloc(sizeof(int32) * nObjects);
	if(refs == NULL)
	{ 
	    HEpush(DFE_NOSPACE,"EHgetid", __FILE__, __LINE__);
	    free(tags);
	    return(-1);
	}

	Vgettagrefs(vgid, tags, refs, nObjects);


	/* Vgroup ID Section */
	/* ----------------- */
	if (code == 0)
	{
	    /* Loop through objects */
	    /* -------------------- */
	    for (i = 0; i < nObjects; i++)
	    {

		/* If object is Vgroup ... */
		/* ----------------------- */
		if (*(tags + i) == DFTAG_VG)
		{

		    /* Get ID and name */
		    /* --------------- */
		    id = Vattach(fid, *(refs + i), l_access);
		    Vgetname(id, name);

		    /* If name equals desired object name get ID */
		    /* ----------------------------------------- */
		    if (strcmp(name, objectname) == 0)
		    {
			outID = id;
			break;
		    }
		    /* If not desired object then detach */
		    /* --------------------------------- */
		    Vdetach(id);
		}
	    }
	} else if (code == 1)
	{

	    /* Loop through objects */
	    /* -------------------- */
	    for (i = 0; i < nObjects; i++)
	    {

		/* If object is Vdata ... */
		/* ---------------------- */
		if (*(tags + i) == DFTAG_VH)
		{

		    /* Get ID and name */
		    /* --------------- */
		    id = VSattach(fid, *(refs + i), l_access);
		    VSgetname(id, name);

		    /* If name equals desired object name get ID */
		    /* ----------------------------------------- */
		    if (EHstrwithin(objectname, name, ',') != -1)
		    {
			outID = id;
			break;
		    }
		    /* If not desired object then detach */
		    /* --------------------------------- */
		    VSdetach(id);
		}
	    }
	}
	free(tags);
	free(refs);
    }
    return (outID);
}


/*----------------------------------------------------------------------------|
|  BEGIN_PROLOG                                                               |
|                                                                             |
|  FUNCTION: EHrevflds                                                        |
|                                                                             |
|  DESCRIPTION: Reverses elements in a string list                            |
|                                                                             |
|                                                                             |
|  Return Value    Type     Units     Description                             |
|  ============   ======  =========   =====================================   |
|  status         intn                return status (0) SUCCEED, (-1) FAIL    |
|                                                                             |
|  INPUTS:                                                                    |
|  dimlist        char                Original dimension list                 |
|                                                                             |
|  OUTPUTS:                                                                   |
|  revdimlist     char                Reversed dimension list                 |
|                                                                             |
|  NOTES:                                                                     |
|                                                                             |
|                                                                             |
|   Date     Programmer   Description                                         |
|  ======   ============  =================================================   |
|  Jun 96   Joel Gales    Original Programmer                                 |
|                                                                             |
|  END_PROLOG                                                                 |
-----------------------------------------------------------------------------*/
intn
EHrevflds(const char *dimlist, char *revdimlist)
{
    intn            status = 0;	/* routine return status variable */

    int32           indx;	/* Loop index */
    int32           nentries;	/* Number of entries in search string */
    int32          *slen;	/* Pointer to string length array */

    char          **ptr;	/* Pointer to string pointer array */
    char           *tempPtr;	/* Temporary string pointer */
    char           *tempdimlist;/* Temporary dimension list */


    /* Copy dimlist into temp dimlist */
    /* ------------------------------ */
    tempdimlist = (char *) malloc((int32)strlen(dimlist) + 1);
    if(tempdimlist == NULL)
    { 
	HEpush(DFE_NOSPACE,"EHrevflds", __FILE__, __LINE__);
	return(-1);
    }
    strcpy(tempdimlist, dimlist);


    /* Count number of entries in search string list */
    /* --------------------------------------------- */
    nentries = EHparsestr(tempdimlist, ',', NULL, NULL);


    /* Allocate string pointer and length arrays */
    /* ----------------------------------------- */
    ptr = (char **) calloc(nentries, sizeof(char *));
    if(ptr == NULL)
    { 
	HEpush(DFE_NOSPACE,"EHrevflds", __FILE__, __LINE__);
	free(tempdimlist);
	return(-1);
    }
    slen = (int32 *) calloc(nentries, sizeof(int32));
    if(slen == NULL)
    { 
	HEpush(DFE_NOSPACE,"EHrevflds", __FILE__, __LINE__);
	free(ptr);
	free(tempdimlist);
	return(-1);
    }


    /* Parse search string */
    /* ------------------- */
    nentries = EHparsestr(tempdimlist, ',', ptr, slen);


    /* Reverse entries in string pointer array */
    /* --------------------------------------- */
    for (indx = 0; indx < nentries / 2; indx++)
    {
	tempPtr = ptr[indx];
	ptr[indx] = ptr[nentries - 1 - indx];
	ptr[nentries - 1 - indx] = tempPtr;
    }


    /* Replace comma delimiters by nulls */
    /* --------------------------------- */
    for (indx = 0; indx < nentries - 1; indx++)
    {
	*(ptr[indx] - 1) = 0;
    }


    /* Build new string list */
    /* --------------------- */
    status = EHloadliststr(ptr, nentries, revdimlist, ',');


    free(slen);
    free(ptr);
    free(tempdimlist);

    return (status);
}



/*----------------------------------------------------------------------------|
|  BEGIN_PROLOG                                                               |
|                                                                             |
|  FUNCTION: EHcntOBJECT                                                      |
|                                                                             |
|  DESCRIPTION: Determines number of OBJECTs in metadata GROUP                |
|                                                                             |
|                                                                             |
|  Return Value    Type     Units     Description                             |
|  ============   ======  =========   =====================================   |
|  count          int32               Number of OBJECTs in GROUP              |
|                                                                             |
|  INPUTS:                                                                    |
|  metabuf        char                Begin & end metadata pointer array      |
|                                                                             |
|  OUTPUTS:                                                                   |
|             None                                                            |
|                                                                             |
|  NOTES:                                                                     |
|                                                                             |
|                                                                             |
|   Date     Programmer   Description                                         |
|  ======   ============  =================================================   |
|  Sep 96   Joel Gales    Original Programmer                                 |
|                                                                             |
|  END_PROLOG                                                                 |
-----------------------------------------------------------------------------*/
int32
EHcntOBJECT(char *metabuf[])
{
    int32           count = 0;	/* Counter */

    char           *metaptr;	/* Beginning of metadata section */
    char           *endptr;	/* End of metadata section */
    char           *tempptr;	/* Pointer within metadata section */


    /* Get Pointers to beginning and ending of metadata section */
    /* -------------------------------------------------------- */
    metaptr = metabuf[0];
    endptr = metabuf[1];


    /* Find number of "END_OBJECT" strings within section */
    /* -------------------------------------------------- */
    tempptr = metaptr;
    while (tempptr < endptr && tempptr != NULL)
    {
	tempptr = strstr(tempptr + 1, "END_OBJECT");
	count++;
    }
    count--;

    return (count);
}





/*----------------------------------------------------------------------------|
|  BEGIN_PROLOG                                                               |
|                                                                             |
|  FUNCTION: EHcntGROUP                                                       |
|                                                                             |
|  DESCRIPTION: Determines number of GROUPs in metadata GROUP                 |
|                                                                             |
|                                                                             |
|  Return Value    Type     Units     Description                             |
|  ============   ======  =========   =====================================   |
|  count          int32               Number of GROUPs in GROUP               |
|                                                                             |
|  INPUTS:                                                                    |
|  metabuf        char                Begin & end metadata pointer array      |
|                                                                             |
|  OUTPUTS:                                                                   |
|             None                                                            |
|                                                                             |
|  NOTES:                                                                     |
|                                                                             |
|                                                                             |
|   Date     Programmer   Description                                         |
|  ======   ============  =================================================   |
|  Sep 96   Joel Gales    Original Programmer                                 |
|                                                                             |
|  END_PROLOG                                                                 |
-----------------------------------------------------------------------------*/
int32
EHcntGROUP(char *metabuf[])
{
    int32           count = 0;	/* Counter */

    char           *metaptr;	/* Beginning of metadata section */
    char           *endptr;	/* End of metadata section */
    char           *tempptr;	/* Pointer within metadata section */


    /* Get Pointers to beginning and ending of metadata section */
    /* -------------------------------------------------------- */
    metaptr = metabuf[0];
    endptr = metabuf[1];


    /* Find number of "END_GROUP" strings within section */
    /* ------------------------------------------------- */
    tempptr = metaptr;
    while (tempptr < endptr && tempptr != NULL)
    {
	tempptr = strstr(tempptr + 1, "END_GROUP");
	count++;
    }
    count--;

    return (count);
}




/*----------------------------------------------------------------------------|
|  BEGIN_PROLOG                                                               |
|                                                                             |
|  FUNCTION: EHmetalist                                                       |
|                                                                             |
|  DESCRIPTION: Converts string list to metadata list                         |
|                                                                             |
|                                                                             |
|  Return Value    Type     Units     Description                             |
|  ============   ======  =========   =====================================   |
|  status         intn                return status (0) SUCCEED, (-1) FAIL    |
|                                                                             |
|  INPUTS:                                                                    |
|  instring       char                Input string list                       |
|                                                                             |
|  OUTPUTS:                                                                   |
|  outstring      char                Output metadata string                  |
|                                                                             |
|  NOTES:                                                                     |
|                                                                             |
|                                                                             |
|   Date     Programmer   Description                                         |
|  ======   ============  =================================================   |
|  Jun 96   Joel Gales    Original Programmer                                 |
|                                                                             |
|  END_PROLOG                                                                 |
-----------------------------------------------------------------------------*/
intn
EHmetalist(char *instring, char *outstring)
{
    intn            i;		/* Loop index */
    intn            status = 0;	/* routine return status variable */

    int32           nentries;	/* Number of entries in search string */
    int32           listlen = 1;/* String list length */
    int32          *slen;	/* Pointer to string length array */

    char          **ptr;	/* Pointer to string pointer array */


    /* Count number of entries in search string list */
    /* --------------------------------------------- */
    nentries = EHparsestr(instring, ',', NULL, NULL);


    /* Allocate string pointer and length arrays */
    /* ----------------------------------------- */
    ptr = (char **) calloc(nentries, sizeof(char *));
    if(ptr == NULL)
    { 
	HEpush(DFE_NOSPACE,"EHmetalist", __FILE__, __LINE__);
	return(-1);
    }
    slen = (int32 *) calloc(nentries, sizeof(int32));
    if(slen == NULL)
    { 
	HEpush(DFE_NOSPACE,"EHmetalist", __FILE__, __LINE__);
	free(ptr);
	return(-1);
    }


    /* Parse input string */
    /* ------------------ */
    nentries = EHparsestr(instring, ',', ptr, slen);


    /* Start output string with leading "(" */
    /* ------------------------------------ */
    strcpy(outstring, "(");


    /* Loop through all entries */
    /* ------------------------ */
    for (i = 0; i < nentries; i++)
    {
	/* Add double quote (") to output string */
	/* ------------------------------------- */
	strcat(outstring, "\"");
	listlen++;

	/* Add input string entry to output string */
	/* --------------------------------------- */
	memcpy(outstring + listlen, ptr[i], slen[i]);
	listlen += slen[i];
	outstring[listlen] = 0;


	/* Add closing double quote (") to output string */
	/* --------------------------------------------- */
	strcat(outstring, "\"");
	listlen++;
	outstring[listlen] = 0;


	/* Add comma delimiter to output string */
	/* ------------------------------------ */
	if (i != (nentries - 1))
	{
	    strcat(outstring, ",");
	    listlen++;
	}
	/* Place null terminator in output string */
	/* -------------------------------------- */
	outstring[listlen] = 0;
    }


    /* End output string with trailing ")" */
    /* ----------------------------------- */
    strcat(outstring, ")");

    free(ptr);
    free(slen);

    return (status);
}





/*----------------------------------------------------------------------------|
|  BEGIN_PROLOG                                                               |
|                                                                             |
|  FUNCTION: EHinsertmeta                                                     |
|                                                                             |
|  DESCRIPTION: Writes metadata                                               |
|                                                                             |
|                                                                             |
|  Return Value    Type     Units     Description                             |
|  ============   ======  =========   =====================================   |
|  status         intn                return status (0) SUCCEED, (-1) FAIL    |
|                                                                             |
|  INPUTS:                                                                    |
|  sdInterfaceID  int32               SDS interface ID                        |
|  structname     char                HDF-EOS structure name                  |
|  structcode     char                Structure code ("s/g/p")                |
|  metacode       int32               Metadata code type                      |
|  metastr        char                Metadata input string                   |
|  metadata       int32               Metadata utility array                  |
|                                                                             |
|  OUTPUTS:                                                                   |
|             None                                                            |
|                                                                             |
|  NOTES:                                                                     |
|                                                                             |
|                                                                             |
|   Date     Programmer   Description                                         |
|  ======   ============  =================================================   |
|  Jun 96   Joel Gales    Original Programmer                                 |
|  Aug 96   Joel Gales    Make metadata ODL compliant                         |
|  Sep 96   Joel Gales    Allow new metadata object to be written in          |
|                         old metadata.                                       |
|  Dec 96   Joel Gales    Fix Point metadata problem                          |
|  Oct 98   David Wynne   Change utlstr/utlstr2 to dynamic allocation from    |
|                         static                                              |
|  Oct 11  Abe Taaheri    Corrected end of structre metadata by adding one    |
|                         more NULL character to the end of it (without this  |
|                         change a garbage character was present at the end   |
|                         of last piece of multiple 32000 size  structre      |
|                         metadatas).                                         |
|                                                                             |
|  END_PROLOG                                                                 |
-----------------------------------------------------------------------------*/
intn
EHinsertmeta(int32 sdInterfaceID, const char *structname, const char *structcode,
	     int32 metacode, char *metastr, int32 metadata[])
{
    intn            i;		/* Loop index */
    intn            status = 0;	/* routine return status variable */

    int32           attrIndex;	/* Structural metadata attribute index */
    int32           slen[8];	/* String length array (for dim map parsing) */
    int32           nmeta;	/* Number of 32000 byte metadata sections */
    int32           metalen;	/* Length of structural metadata */
    int32           seglen;	/* Length of metadata string to insert */
    int32           count;	/* Objects/Groups counter */
    int32           offset;	/* Offset insertion position of new metadata
				 * section within existing metadata */

    char           *metabuf;	/* Pointer (handle) to structural metadata */
    char           *begptr;	/* Pointer to beginning of metadata section */
    char           *metaptr;	/* Metadata pointer */
    char           *prevmetaptr;/* Previous position of metadata pointer */
    char           *ptr[8];	/* String pointer array (for dim map parsing) */
    char            type[32];	/* Number type descriptor string */
    char           *metaArr[2];	/* Array of metadata positions */
    char           *colon;	/* Colon position */
    char           *colon2;	/* 2nd colon position */
    char           *slash;	/* Slash position */
    char           *utlstr;	/* Utility string */
    char           *utlstr2;	/* Utility string 2 */


    /* Allocate space for utility strings */
    /* ---------------------------------- */
    utlstr = (char *) calloc(UTLSTRSIZE, sizeof(char));
    if(utlstr == NULL)
    { 
	HEpush(DFE_NOSPACE,"EHinsertmeta", __FILE__, __LINE__);
	return(-1);
    }

    utlstr2 = (char *) calloc(UTLSTRSIZE, sizeof(char));
    if(utlstr2 == NULL)
    { 
	HEpush(DFE_NOSPACE,"EHinsertmeta", __FILE__, __LINE__);
	free(utlstr);
	return(-1);
    }

    /* Determine number of structural metadata "sections" */
    /* -------------------------------------------------- */
    nmeta = 0;
    while (1)
    {
	/* Search for "StructMetadata.x" attribute */
	/* --------------------------------------- */
      snprintf(utlstr, UTLSTRSIZE, "%s%d", "StructMetadata.", nmeta);
      attrIndex = SDfindattr(sdInterfaceID, utlstr);
      
      
      /* If found then increment metadata section counter else exit loop */
      /* --------------------------------------------------------------- */
      if (attrIndex != -1)
	{
	  nmeta++;
	} else
	{
	  break;
	}
    }


    /* Allocate space for metadata (in units of 32000 bytes) */
    /* ----------------------------------------------------- */
    metabuf = (char *) calloc(32000 * nmeta, 1);
    if(metabuf == NULL)
    { 
	HEpush(DFE_NOSPACE,"EHinsertmeta", __FILE__, __LINE__);
	free(utlstr);
	free(utlstr2);
	return(-1);
    }


    /* Read structural metadata */
    /* ------------------------ */
    for (i = 0; i < nmeta; i++)
    {
      snprintf(utlstr, UTLSTRSIZE, "%s%d", "StructMetadata.", i);
      attrIndex = SDfindattr(sdInterfaceID, utlstr);
      metalen = (int32)strlen(metabuf);
      SDreadattr(sdInterfaceID, attrIndex, metabuf + metalen);
    }

    /* Determine length (# of characters) of metadata */
    /* ---------------------------------------------- */
    metalen = (int32)strlen(metabuf);



    /* Find HDF-EOS structure "root" group in metadata */
    /* ----------------------------------------------- */

    /* Setup proper search string */
    /* -------------------------- */
    if (strcmp(structcode, "s") == 0)
    {
	strcpy(utlstr, "GROUP=SwathStructure");
    } else if (strcmp(structcode, "g") == 0)
    {
	strcpy(utlstr, "GROUP=GridStructure");
    } else if (strcmp(structcode, "p") == 0)
    {
	strcpy(utlstr, "GROUP=PointStructure");
    }
    /* Use string search routine (strstr) to move through metadata */
    /* ----------------------------------------------------------- */
    metaptr = strstr(metabuf, utlstr);



    /* Find specific (named) structure */
    /* ------------------------------- */
    if (metacode < 1000)
    {
	/* Save current metadata pointer */
	/* ----------------------------- */
	prevmetaptr = metaptr;


	/* First loop for "old-style" (non-ODL) metadata string */
	/* ---------------------------------------------------- */
	if (strcmp(structcode, "s") == 0)
	{
	  snprintf(utlstr, UTLSTRSIZE, "%s%s", "SwathName=\"", structname);
	} else if (strcmp(structcode, "g") == 0)
	{
	  snprintf(utlstr, UTLSTRSIZE, "%s%s", "GridName=\"", structname);
	} else if (strcmp(structcode, "p") == 0)
	{
	  snprintf(utlstr, UTLSTRSIZE, "%s%s", "PointName=\"", structname);
	}
	/* Do string search */
	/* ---------------- */
	metaptr = strstr(metaptr, utlstr);


	/*
	 * If not found then return to previous position in metadata and look
	 * for "new-style" (ODL) metadata string
	 */
	if (metaptr == NULL)
	{
	  snprintf(utlstr, UTLSTRSIZE, "%s%s", "GROUP=\"", structname);
	    metaptr = strstr(prevmetaptr, utlstr);
	}
    }
    /*
     * If searching for geo fields (3), data fields (4), or point fields (11)
     * convert type code to string designator.
     */
    if (metacode == 3 || metacode == 4 || metacode == 11)
    {
	switch (metadata[0])
	{
	case 3:
	    strcpy(type, "DFNT_UCHAR8");
	    break;
	case 4:
	    strcpy(type, "DFNT_CHAR8");
	    break;
	case 5:
	    strcpy(type, "DFNT_FLOAT32");
	    break;
	case 6:
	    strcpy(type, "DFNT_FLOAT64");
	    break;
	case 20:
	    strcpy(type, "DFNT_INT8");
	    break;
	case 21:
	    strcpy(type, "DFNT_UINT8");
	    break;
	case 22:
	    strcpy(type, "DFNT_INT16");
	    break;
	case 23:
	    strcpy(type, "DFNT_UINT16");
	    break;
	case 24:
	    strcpy(type, "DFNT_INT32");
	    break;
	case 25:
	    strcpy(type, "DFNT_UINT32");
	    break;
	}
    }
    /* Metadata Section Switch */
    /* ----------------------- */
    switch (abs(metacode))
    {

    case 0:
	/* Dimension Section */
	/* ----------------- */

	/* Find beginning and ending of metadata section */
	/* --------------------------------------------- */
	strcpy(utlstr, "\t\tGROUP=Dimension");
	begptr = strstr(metaptr, utlstr);

	strcpy(utlstr, "\t\tEND_GROUP=Dimension");
	metaptr = strstr(metaptr, utlstr);


	/* Count number of existing entries and increment */
	/* ---------------------------------------------- */
	metaArr[0] = begptr;
	metaArr[1] = metaptr;
	count = EHcntOBJECT(metaArr) + 1;


	/* Build metadata entry string */
	/* --------------------------- */
	snprintf(utlstr, UTLSTRSIZE, "%s%d%s%s%s%d%s%d%s", "\t\t\tOBJECT=Dimension_", count,
		"\n\t\t\t\tDimensionName=\"", &metastr[0],
		"\"\n\t\t\t\tSize=", metadata[0],
		"\n\t\t\tEND_OBJECT=Dimension_", count, "\n");
	break;


    case 1:
	/* Dimension Map Section */
	/* --------------------- */

	/* Find beginning and ending of metadata section */
	/* --------------------------------------------- */
	strcpy(utlstr, "\t\tGROUP=DimensionMap");
	begptr = strstr(metaptr, utlstr);

	strcpy(utlstr, "\t\tEND_GROUP=DimensionMap");
	metaptr = strstr(metaptr, utlstr);


	/* Count number of existing entries and increment */
	/* ---------------------------------------------- */
	metaArr[0] = begptr;
	metaArr[1] = metaptr;
	count = EHcntOBJECT(metaArr) + 1;


	/* Find slash within input mapping string and replace with NULL */
	/* ------------------------------------------------------------ */
	EHparsestr(metastr, '/', ptr, slen);
	metastr[slen[0]] = 0;


	/* Build metadata entry string */
	/* --------------------------- */
	snprintf(utlstr, UTLSTRSIZE, "%s%d%s%s%s%s%s%d%s%d%s%d%s",
		"\t\t\tOBJECT=DimensionMap_", count,
		"\n\t\t\t\tGeoDimension=\"", &metastr[0],
		"\"\n\t\t\t\tDataDimension=\"", &metastr[slen[0] + 1],
		"\"\n\t\t\t\tOffset=", metadata[0],
		"\n\t\t\t\tIncrement=", metadata[1],
		"\n\t\t\tEND_OBJECT=DimensionMap_", count, "\n");
	break;


    case 2:
	/* Index Dimension Map Section */
	/* --------------------------- */

	/* Find beginning and ending of metadata section */
	/* --------------------------------------------- */
	strcpy(utlstr, "\t\tGROUP=IndexDimensionMap");
	begptr = strstr(metaptr, utlstr);

	strcpy(utlstr, "\t\tEND_GROUP=IndexDimensionMap");
	metaptr = strstr(metaptr, utlstr);


	/* Count number of existing entries and increment */
	/* ---------------------------------------------- */
	metaArr[0] = begptr;
	metaArr[1] = metaptr;
	count = EHcntOBJECT(metaArr) + 1;


	/* Find slash within input mapping string and replace with NULL */
	/* ------------------------------------------------------------ */
	EHparsestr(metastr, '/', ptr, slen);
	metastr[slen[0]] = 0;


	/* Build metadata entry string */
	/* --------------------------- */
	snprintf(utlstr, UTLSTRSIZE, "%s%d%s%s%s%s%s%d%s",
		"\t\t\tOBJECT=IndexDimensionMap_", count,
		"\n\t\t\t\tGeoDimension=\"", &metastr[0],
		"\"\n\t\t\t\tDataDimension=\"", &metastr[slen[0] + 1],
		"\"\n\t\t\tEND_OBJECT=IndexDimensionMap_", count, "\n");
	break;


    case 3:
	/* Geolocation Field Section */
	/* ------------------------- */

	/* Find beginning and ending of metadata section */
	/* --------------------------------------------- */
	strcpy(utlstr, "\t\tGROUP=GeoField");
	begptr = strstr(metaptr, utlstr);

	strcpy(utlstr, "\t\tEND_GROUP=GeoField");
	metaptr = strstr(metaptr, utlstr);


	/* Count number of existing entries and increment */
	/* ---------------------------------------------- */
	metaArr[0] = begptr;
	metaArr[1] = metaptr;
	count = EHcntOBJECT(metaArr) + 1;


	/* Find colon (parse off field name) */
	/* --------------------------------- */
	colon = strchr(metastr, ':');
	*colon = 0;


	/* Search for next colon (compression and/or tiling parameters) */
	/* ------------------------------------------------------------ */
	colon2 = strchr(colon + 1, ':');
	if (colon2 != NULL)
	{
	    *colon2 = 0;
	}
	/* Make metadata string list for dimension list */
	/* -------------------------------------------- */
	EHmetalist(colon + 1, utlstr2);


	/* Build metadata entry string */
	/* --------------------------- */
	snprintf(utlstr, UTLSTRSIZE, "%s%d%s%s%s%s%s%s",
		"\t\t\tOBJECT=GeoField_", count,
		"\n\t\t\t\tGeoFieldName=\"", metastr,
		"\"\n\t\t\t\tDataType=", type,
		"\n\t\t\t\tDimList=", utlstr2);


	/* If compression and/or tiling parameters add to string */
	/* ----------------------------------------------------- */
	if (colon2 != NULL)
	{
	    strcat(utlstr, colon2 + 1);
	}
	/* Add END_OBJECT terminator to metadata string */
	/* -------------------------------------------- */
	snprintf(utlstr2, UTLSTRSIZE, "%s%d%s",
		"\n\t\t\tEND_OBJECT=GeoField_", count, "\n");
	strcat(utlstr, utlstr2);

	break;


    case 4:
	/* Data Field Section */
	/* ------------------ */

	/* Find beginning and ending of metadata section */
	/* --------------------------------------------- */
        strcpy(utlstr, "\t\tGROUP=DataField");
	begptr = strstr(metaptr, utlstr);

	strcpy(utlstr, "\t\tEND_GROUP=DataField");
	metaptr = strstr(metaptr, utlstr);


	/* Count number of existing entries and increment */
	/* ---------------------------------------------- */
	metaArr[0] = begptr;
	metaArr[1] = metaptr;
	count = EHcntOBJECT(metaArr) + 1;


	/* Find colon (parse off field name) */
	/* --------------------------------- */
	colon = strchr(metastr, ':');
	*colon = 0;


	/* Search for next colon (compression and/or tiling parameters) */
	/* ------------------------------------------------------------ */
	colon2 = strchr(colon + 1, ':');
	if (colon2 != NULL)
	{
	    *colon2 = 0;
	}
	/* Make metadata string list from dimension list */
	/* --------------------------------------------- */
	EHmetalist(colon + 1, utlstr2);


	/* Build metadata entry string */
	/* --------------------------- */
	snprintf(utlstr, UTLSTRSIZE, "%s%d%s%s%s%s%s%s",
		"\t\t\tOBJECT=DataField_", count,
		"\n\t\t\t\tDataFieldName=\"", metastr,
		"\"\n\t\t\t\tDataType=", type,
		"\n\t\t\t\tDimList=", utlstr2);


	/* If compression and/or tiling parameters add to string */
	/* ----------------------------------------------------- */
	if (colon2 != NULL)
	{
	    strcat(utlstr, colon2 + 1);
	}
	/* Add END_OBJECT terminator to metadata string */
	/* -------------------------------------------- */
	snprintf(utlstr2, UTLSTRSIZE, "%s%d%s",
		"\n\t\t\tEND_OBJECT=DataField_", count, "\n");
	strcat(utlstr, utlstr2);

	break;


    case 6:
	/* Merged Field Section */
	/* -------------------- */

	/* Find beginning and ending of metadata section */
	/* --------------------------------------------- */
	strcpy(utlstr, "\t\tGROUP=MergedFields");
	begptr = strstr(metaptr, utlstr);

	strcpy(utlstr, "\t\tEND_GROUP=MergedFields");
	metaptr = strstr(metaptr, utlstr);


	/* Count number of existing entries and increment */
	/* ---------------------------------------------- */
	metaArr[0] = begptr;
	metaArr[1] = metaptr;
	count = EHcntOBJECT(metaArr) + 1;


	/* Find colon (parse off merged fieldname) */
	/* --------------------------------------- */
	colon = strchr(metastr, ':');


	/* Make metadata string list from field list */
	/* ----------------------------------------- */
	EHmetalist(colon + 1, utlstr2);
	*colon = 0;


	/* Build metadata entry string */
	/* --------------------------- */
	snprintf(utlstr, UTLSTRSIZE, "%s%d%s%s%s%s%s%s%d%s",
		"\t\t\tOBJECT=MergedFields_", count,
		"\n\t\t\t\tMergedFieldName=\"", metastr, "\"",
		"\n\t\t\t\tFieldList=", utlstr2,
		"\n\t\t\tEND_OBJECT=MergedFields_", count, "\n");
	break;


    case 10:
	/* Point Level Section */
	/* ------------------- */

	/* Find beginning and ending of metadata section */
	/* --------------------------------------------- */
	strcpy(utlstr, "\t\tGROUP=Level");
	begptr = strstr(metaptr, utlstr);

	strcpy(utlstr, "\n\t\tEND_GROUP=Level");
	metaptr = strstr(metaptr, utlstr) + 1;


	/* Count number of existing entries and increment */
	/* ---------------------------------------------- */
	metaArr[0] = begptr;
	metaArr[1] = metaptr;
	count = EHcntGROUP(metaArr);


	/* Build metadata entry string */
	/* --------------------------- */
	snprintf(utlstr, UTLSTRSIZE, "%s%d%s%s%s%d%s",
		"\t\t\tGROUP=Level_", count,
		"\n\t\t\t\tLevelName=\"", metastr,
		"\"\n\t\t\tEND_GROUP=Level_", count, "\n");
	break;


    case 11:
	/* Point Field Section */
	/* ------------------- */

	/* Find colon (parse off point field name) */
	/* --------------------------------------- */
	colon = strchr(metastr, ':');
	*colon = 0;


	/* Find beginning and ending of metadata section */
	/* --------------------------------------------- */
	strcpy(utlstr, "\t\t\t\tLevelName=\"");
	strcat(utlstr, colon + 1);
	begptr = strstr(metaptr, utlstr);

	strcpy(utlstr, "\t\t\tEND_GROUP=Level_");
	metaptr = strstr(begptr, utlstr);


	/* Count number of existing entries and increment */
	/* ---------------------------------------------- */
	metaArr[0] = begptr;
	metaArr[1] = metaptr;
	count = EHcntOBJECT(metaArr) + 1;


	/* Build metadata entry string */
	/* --------------------------- */
	snprintf(utlstr, UTLSTRSIZE, "%s%d%s%s%s%s%s%d%s%d%s",
		"\t\t\t\tOBJECT=PointField_", count,
		"\n\t\t\t\t\tPointFieldName=\"", metastr,
		"\"\n\t\t\t\t\tDataType=", type,
		"\n\t\t\t\t\tOrder=", metadata[1],
		"\n\t\t\t\tEND_OBJECT=PointField_", count, "\n");
	break;



    case 12:
	/* Level Link Section */
	/* ------------------ */

	/* Find beginning and ending of metadata section */
	/* --------------------------------------------- */
	strcpy(utlstr, "\t\tGROUP=LevelLink");
	begptr = strstr(metaptr, utlstr);

	strcpy(utlstr, "\t\tEND_GROUP=LevelLink");
	metaptr = strstr(metaptr, utlstr);


	/* Count number of existing entries and increment */
	/* ---------------------------------------------- */
	metaArr[0] = begptr;
	metaArr[1] = metaptr;
	count = EHcntOBJECT(metaArr) + 1;


	/* Find colon (parse off parent/child level names from link field) */
	/* --------------------------------------------------------------- */
	colon = strchr(metastr, ':');
	*colon = 0;


	/* Find slash (divide parent and child levels) */
	/* ------------------------------------------- */
	slash = strchr(metastr, '/');
	*slash = 0;


	/* Build metadata entry string */
	/* --------------------------- */
	snprintf(utlstr, UTLSTRSIZE, "%s%d%s%s%s%s%s%s%s%d%s",
		"\t\t\tOBJECT=LevelLink_", count,
		"\n\t\t\t\tParent=\"", metastr,
		"\"\n\t\t\t\tChild=\"", slash + 1,
		"\"\n\t\t\t\tLinkField=\"", colon + 1,
		"\"\n\t\t\tEND_OBJECT=LevelLink_", count, "\n");

	break;


    case 101:
	/* Position metadata pointer for Grid proj parms, pix reg, origin */
	/* -------------------------------------------------------------- */
	strcpy(utlstr, "\t\tGROUP=Dimension");
	metaptr = strstr(metaptr, utlstr);
	strcpy(utlstr, metastr);

	break;


    case 1001:
	/* Position metadata pointer for new swath structure (SWcreate) */
	/* ------------------------------------------------------------ */
	strcpy(utlstr, "END_GROUP=SwathStructure");
	metaptr = strstr(metaptr, utlstr);
	strcpy(utlstr, metastr);
	break;


    case 1002:
	/* Position metadata pointer for new grid structure (GDcreate) */
	/* ----------------------------------------------------------- */
	strcpy(utlstr, "END_GROUP=GridStructure");
	metaptr = strstr(metaptr, utlstr);
	strcpy(utlstr, metastr);
	break;


    case 1003:
	/* Position metadata pointer for new point structure (PTcreate) */
	/* ------------------------------------------------------------ */
	strcpy(utlstr, "END_GROUP=PointStructure");
	metaptr = strstr(metaptr, utlstr);
	strcpy(utlstr, metastr);
	break;
    }



    /* Get length of metadata string to insert */
    /* --------------------------------------- */
    seglen = (int32)strlen(utlstr);

    /* Get offset of entry position within existing metadata */
    /* ---------------------------------------------------- */
    offset = (int)(metaptr - metabuf);


    /* If end of new metadata string outside of current metadata buffer ... */
    /* -------------------------------------------------------------------- */
    if (metalen + seglen > 32000 * nmeta - 1)
    {
	/* Reallocate metadata buffer with additional 32000 bytes */
	/* ------------------------------------------------------ */
	metabuf = (char *) realloc((void *) metabuf, 32000 * (nmeta + 1));
	if(metabuf == NULL)
	{ 
	    HEpush(DFE_NOSPACE,"EHinsertmeta", __FILE__, __LINE__);
	    free(utlstr);
	    free(utlstr2);
	    return(-1);
	}

	/* Increment metadata section counter */
	/* ---------------------------------- */
	nmeta++;

	/* Reposition metadata pointer (entry position) */
	/* -------------------------------------------- */
	metaptr = metabuf + offset;
    }
    /* Move metadata following entry point to its new position */
    /* ------------------------------------------------------- */
    for (i = metalen - 1; i > offset - 1; i--)
    {
	*(metabuf + seglen + i) = *(metabuf + i);
    }

    /* Copy new metadata string (utlstr) into metadata */
    /* ---------------------------------------------- */
    memcpy(metaptr, utlstr, seglen);

    /* set to null character remaining of the metabuf */

    memset((metabuf + metalen + seglen), '\0', (nmeta*32000 - (metalen +
								  seglen)));
    /* Add new null string terminator */
    /* ------------------------------ */
    metabuf[metalen + seglen] = 0;


    /* Write Back to Global Attribute(s) */
    /* --------------------------------- */
    for (i = 0; i < nmeta; i++)
    {
      snprintf(utlstr, UTLSTRSIZE, "%s%d", "StructMetadata.", i);
	SDsetattr(sdInterfaceID, utlstr, DFNT_CHAR8,
		  32000, metabuf + i * 32000);
    }

    free(metabuf);
    free(utlstr);
    free(utlstr2);

    return (status);
}


/*----------------------------------------------------------------------------|
|  BEGIN_PROLOG                                                               |
|                                                                             |
|  FUNCTION: EHgetmetavalue                                                   |
|                                                                             |
|  DESCRIPTION: Returns metadata value                                        |
|                                                                             |
|                                                                             |
|  Return Value    Type     Units     Description                             |
|  ============   ======  =========   =====================================   |
|  status         intn                return status (0) SUCCEED, (-1) FAIL    |
|                                                                             |
|  INPUTS:                                                                    |
|  metaptrs        char               Begin and end of metadata section       |
|  parameter      char                parameter to access                     |
|                                                                             |
|  OUTPUTS:                                                                   |
|  metaptr        char                Ptr to (updated) beginning of metadata  |
|  retstr         char                return string containing value          |
|                                                                             |
|  NOTES:                                                                     |
|                                                                             |
|                                                                             |
|   Date     Programmer   Description                                         |
|  ======   ============  =================================================   |
|  Jun 96   Joel Gales    Original Programmer                                 |
|  Jan 97   Joel Gales    Check string pointer against end of meta section    |
|                                                                             |
|  END_PROLOG                                                                 |
-----------------------------------------------------------------------------*/
intn
EHgetmetavalue(char *metaptrs[], const char *parameter, char *retstr)
{
    intn            status = 0;	/* routine return status variable */

    int32           slen;	/* String length */
    char           *newline;	/* Position of new line character */
    char           *sptr;	/* string pointer within metadata */
  
    /* Get string length of parameter string + 1 */
    /* ----------------------------------------- */
    slen = (int32)strlen(parameter) + 1;

    /* Build search string (parameter string + "=") */
    /* -------------------------------------------- */
    strcpy(retstr, parameter);
    strcat(retstr, "=");


    /* Search for string within metadata (beginning at metaptrs[0]) */
    /* ------------------------------------------------------------ */
    sptr = strstr(metaptrs[0], retstr);


    /* If string found within desired section ... */
    /* ------------------------------------------ */
    if (sptr != NULL && sptr < metaptrs[1])
    {
	/* Store position of string within metadata */
	/* ---------------------------------------- */
	metaptrs[0] = sptr;

	/* Find newline "\n" character */
	/* --------------------------- */
	newline = strchr(metaptrs[0], '\n');

	/* Copy from "=" to "\n" (exclusive) into return string */
	/* ---------------------------------------------------- */
	memcpy(retstr, metaptrs[0] + slen, newline - metaptrs[0] - slen);

	/* Terminate return string with null */
	/* --------------------------------- */
	retstr[newline - metaptrs[0] - slen] = 0;
    } else
    {
	/*
	 * if parameter string not found within section, null return string
	 * and set status to -1.
	 */
	retstr[0] = 0;
	status = -1;
    }

    return (status);
}




/*----------------------------------------------------------------------------|
|  BEGIN_PROLOG                                                               |
|                                                                             |
|  FUNCTION: EHmetagroup                                                      |
|                                                                             |
|  DESCRIPTION: Returns pointers to beginning and end of metadata group       |
|                                                                             |
|                                                                             |
|  Return Value    Type     Units     Description                             |
|  ============   ======  =========   =====================================   |
|  metabuf        char                Pointer to HDF-EOS object in metadata   |
|                                                                             |
|  INPUTS:                                                                    |
|  sdInterfaceID  int32               SDS interface ID                        |
|  structname     char                HDF-EOS structure name                  |
|  structcode     char                Structure code ("s/g/p")                |
|  groupname      char                Metadata group name                     |
|                                                                             |
|  OUTPUTS:                                                                   |
|  metaptrs       char                pointers to begin and end of metadata   |
|                                                                             |
|  NOTES:                                                                     |
|                                                                             |
|                                                                             |
|   Date     Programmer   Description                                         |
|  ======   ============  =================================================   |
|  Jun 96   Joel Gales    Original Programmer                                 |
|  Aug 96   Joel Gales    Make metadata ODL compliant                         |
|                                                                             |
|  END_PROLOG                                                                 |
-----------------------------------------------------------------------------*/
char           *
EHmetagroup(int32 sdInterfaceID, char *structname, const char *structcode,
	    const char *groupname, char *metaptrs[])
{
    intn            i;		/* Loop index */

    int32           attrIndex;	/* Structural metadata attribute index */
    int32           nmeta;	/* Number of 32000 byte metadata sections */
    int32           metalen;	/* Length of structural metadata */

    char           *metabuf;	/* Pointer (handle) to structural metadata */
    char           *endptr;	/* Pointer to end of metadata section */
    char           *metaptr;	/* Metadata pointer */
    char           *prevmetaptr;/* Previous position of metadata pointer */
    char           *utlstr;     /* Utility string */



     /* Allocate memory for utility string */
     /* ---------------------------------- */
    utlstr = (char *) calloc(UTLSTR_MAX_SIZE,sizeof(char));
    if(utlstr == NULL)
    { 
        HEpush(DFE_NOSPACE,"EHEHmetagroup", __FILE__, __LINE__);
	
        return( NULL);
    }
    /* Determine number of structural metadata "sections" */
    /* -------------------------------------------------- */
    nmeta = 0;
    while (1)
    {
	/* Search for "StructMetadata.x" attribute */
	/* --------------------------------------- */
      snprintf(utlstr, UTLSTR_MAX_SIZE, "%s%d", "StructMetadata.", nmeta);
      attrIndex = SDfindattr(sdInterfaceID, utlstr);
      
      
      /* If found then increment metadata section counter else exit loop */
      /* --------------------------------------------------------------- */
      if (attrIndex != -1)
	{
	  nmeta++;
	} else
	{
	  break;
	}
    }


    /* Allocate space for metadata (in units of 32000 bytes) */
    /* ----------------------------------------------------- */
    metabuf = (char *) calloc(32000 * nmeta, 1);
    
    if(metabuf == NULL)
    { 
	HEpush(DFE_NOSPACE,"EHmetagroup", __FILE__, __LINE__);
	free(utlstr);
	return(metabuf);
    }
    

    /* Read structural metadata */
    /* ------------------------ */
    for (i = 0; i < nmeta; i++)
    {
      snprintf(utlstr, UTLSTR_MAX_SIZE, "%s%d", "StructMetadata.", i);
	attrIndex = SDfindattr(sdInterfaceID, utlstr);
	metalen = (int32)strlen(metabuf);
	SDreadattr(sdInterfaceID, attrIndex, metabuf + metalen);
    }

    /* Determine length (# of characters) of metadata */
    /* ---------------------------------------------- */
    metalen = (int32)strlen(metabuf);



    /* Find HDF-EOS structure "root" group in metadata */
    /* ----------------------------------------------- */

    /* Setup proper search string */
    /* -------------------------- */
    if (strcmp(structcode, "s") == 0)
    {
	strcpy(utlstr, "GROUP=SwathStructure");
    } else if (strcmp(structcode, "g") == 0)
    {
	strcpy(utlstr, "GROUP=GridStructure");
    } else if (strcmp(structcode, "p") == 0)
    {
	strcpy(utlstr, "GROUP=PointStructure");
    }
    /* Use string search routine (strstr) to move through metadata */
    /* ----------------------------------------------------------- */
    metaptr = strstr(metabuf, utlstr);



    /* Save current metadata pointer */
    /* ----------------------------- */
    prevmetaptr = metaptr;


    /* First loop for "old-style" (non-ODL) metadata string */
    /* ---------------------------------------------------- */
    if (strcmp(structcode, "s") == 0)
    {
      snprintf(utlstr, UTLSTR_MAX_SIZE, "%s%s", "SwathName=\"", structname);
    } else if (strcmp(structcode, "g") == 0)
    {
      snprintf(utlstr, UTLSTR_MAX_SIZE, "%s%s", "GridName=\"", structname);
    } else if (strcmp(structcode, "p") == 0)
    {
      snprintf(utlstr, UTLSTR_MAX_SIZE, "%s%s", "PointName=\"", structname);
    }
    /* Do string search */
    /* ---------------- */
    metaptr = strstr(metaptr, utlstr);


    /*
     * If not found then return to previous position in metadata and look for
     * "new-style" (ODL) metadata string
     */
    if (metaptr == NULL)
    {
      snprintf(utlstr, UTLSTR_MAX_SIZE, "%s%s", "GROUP=\"", structname);
	metaptr = strstr(prevmetaptr, utlstr);
    }
    /* Find group within structure */
    /* --------------------------- */
    if (groupname != NULL)
    {
      snprintf(utlstr, UTLSTR_MAX_SIZE, "%s%s", "GROUP=", groupname);
      metaptr = strstr(metaptr, utlstr);

      snprintf(utlstr, UTLSTR_MAX_SIZE, "%s%s", "\t\tEND_GROUP=", groupname);
      endptr = strstr(metaptr, utlstr);
    } else
      {
	/* If groupname == NULL then find end of structure in metadata */
	/* ----------------------------------------------------------- */
	snprintf(utlstr, UTLSTR_MAX_SIZE, "%s", "\n\tEND_GROUP=");
	endptr = strstr(metaptr, utlstr);
      }


    /* Return beginning and ending pointers */
    /* ------------------------------------ */
    metaptrs[0] = metaptr;
    metaptrs[1] = endptr;

    free(utlstr);

    return (metabuf);
}





/*----------------------------------------------------------------------------|
|  BEGIN_PROLOG                                                               |
|                                                                             |
|  FUNCTION: EHfillfld                                                        |
|                                                                             |
|  DESCRIPTION: Fills field with fill value                                   |
|                                                                             |
|                                                                             |
|  Return Value    Type     Units     Description                             |
|  ============   ======  =========   =====================================   |
|  status         intn                return status (0) SUCCEED, (-1) FAIL    |
|                                                                             |
|  INPUTS:                                                                    |
|  sdid           int32               SD element ID                           |
|  rank           int32               Rank of field                           |
|  truerank       int32               True rank of field (merging)            |
|  size           int32               size of fill element                    |
|  off            int32               Offset of field within merged field     |
|  dims           int32               Dimensions of field                     |
|  fillval        void                fill value                              |
|                                                                             |
|                                                                             |
|  OUTPUTS:                                                                   |
|             None                                                            |
|                                                                             |
|  NOTES:                                                                     |
|                                                                             |
|                                                                             |
|   Date     Programmer   Description                                         |
|  ======   ============  =================================================   |
|  Jun 96   Joel Gales    Original Programmer                                 |
|                                                                             |
|  END_PROLOG                                                                 |
-----------------------------------------------------------------------------*/
intn
EHfillfld(int32 sdid, int32 rank, int32 truerank, int32 size, int32 off,
	  int32 dims[], VOIDP fillval)
{
    intn            i;		/* Loop index */
    intn            j;		/* Loop index */
    intn            status = 0;	/* routine return status variable */

    int32           n;		/* Max number of planes or rows in fill
				 * buffer */
    int32           start[3] = {0, 0, 0};	/* Start array (SDwritedata) */
    int32           edge[3];	/* Edge (count) array (SDwritedata) */
    int32           totN;	/* Total number of elements in field */
    int32           planeN;	/* Number of elements in plane */

    char           *fillbuf;	/* Fill buffer */


    /* Get total number of elements in field */
    /* ------------------------------------- */
    totN = dims[0];
    for (i = 1; i < rank; i++)
    {
	totN *= dims[i];
    }


    /* Get number of elements in a plane of the field */
    /* ---------------------------------------------- */
    planeN = dims[1] * dims[2];



    /* Allocate & Write Fill buffer */
    /* ---------------------------- */
    if (totN * size < HDFE_MAXMEMBUF)
    {
	/* Entire field size (in bytes) smaller than max fill buffer */
	/* --------------------------------------------------------- */


	/* Allocate fill buffer */
	/* -------------------- */
	fillbuf = (char *) malloc(totN * size);
	if(fillbuf == NULL)
	{ 
	    HEpush(DFE_NOSPACE,"EHfillfld", __FILE__, __LINE__);
	    return(-1);
	}
	

	/* Fill buffer with fill value */
	/* --------------------------- */
	for (i = 0; i < totN; i++)
	{
	    memcpy(fillbuf + i * size, fillval, size);
	}


	/* Write fill buffer to field */
	/* -------------------------- */
	start[0] = off;
	edge[0] = dims[0];
	edge[1] = dims[1];
	edge[2] = dims[2];
	status = SDwritedata(sdid, start, NULL, edge,
			     (VOIDP) fillbuf);

	free(fillbuf);

    } else if (planeN * size < HDFE_MAXMEMBUF)
    {
	/* Single plane size (in bytes) smaller than max fill buffer */
	/* --------------------------------------------------------- */


	/* Compute number of planes that can be written at one time */
	/* -------------------------------------------------------- */
	n = HDFE_MAXMEMBUF / (planeN * size);


	/* Allocate fill buffer */
	/* -------------------- */
	fillbuf = (char *) malloc(planeN * size * n);
	if(fillbuf == NULL)
	{ 
	    HEpush(DFE_NOSPACE,"EHfillfld", __FILE__, __LINE__);
	    return(-1);
	}


	/* Fill buffer with fill value */
	/* --------------------------- */
	for (i = 0; i < planeN * n; i++)
	{
	    memcpy(fillbuf + i * size, fillval, size);
	}


	/* Write (full) fill buffer to field */
	/* --------------------------------- */
	for (i = 0; i < (dims[0] / n); i++)
	{
	    start[0] = off + i * n;
	    edge[0] = n;
	    edge[1] = dims[1];
	    edge[2] = dims[2];
	    status = SDwritedata(sdid, start, NULL, edge,
				 (VOIDP) fillbuf);
	}


	/* Write (partial) last fill buffer to field (if necessary) */
	/* -------------------------------------------------------- */
	if (i * n != dims[0])
	{
	    start[0] = off + i * n;
	    edge[0] = dims[0] - i * n;
	    edge[1] = dims[1];
	    edge[2] = dims[2];
	    status = SDwritedata(sdid, start, NULL, edge,
				 (VOIDP) fillbuf);
	}
	free(fillbuf);

    } else
    {
	/* Single plane size (in bytes) greater than max fill buffer */
	/* --------------------------------------------------------- */


	/* Compute number of "rows" than can be written at one time */
	/* -------------------------------------------------------- */
	n = HDFE_MAXMEMBUF / (dims[rank - 1] * size);


	/* Allocate fill buffer */
	/* -------------------- */
	fillbuf = (char *) malloc(dims[rank - 1] * size * n);
	if(fillbuf == NULL)
	{ 
	    HEpush(DFE_NOSPACE,"EHfillfld", __FILE__, __LINE__);
	    return(-1);
	}


	/* Fill buffer with fill value */
	/* --------------------------- */
	for (i = 0; i < dims[rank - 1] * n; i++)
	{
	    memcpy(fillbuf + i * size, fillval, size);
	}


	/* For every plane in field ... */
	/* ---------------------------- */
	for (j = 0; j < dims[0]; j++)
	{

	    /* Write (full) fill buffer to field */
	    /* --------------------------------- */
	    for (i = 0; i < (dims[1] / n); i++)
	    {
		start[0] = off + j;
		start[1] = i * n;
		edge[0] = 1;
		edge[1] = n;
		edge[2] = dims[2];
		status = SDwritedata(sdid, start, NULL, edge,
				     (VOIDP) fillbuf);
	    }


	    /* Write (partial) last fill buffer to field (if necessary) */
	    /* -------------------------------------------------------- */
	    if (i * n != dims[1])
	    {
		start[0] = off + j;
		start[1] = i * n;
		edge[0] = 1;
		edge[1] = dims[1] - i * n;
		edge[2] = dims[2];
		status = SDwritedata(sdid, start, NULL, edge,
				     (VOIDP) fillbuf);
	    }
	}

	free(fillbuf);

    }

    return (status);
}






/*----------------------------------------------------------------------------|
|  BEGIN_PROLOG                                                               |
|                                                                             |
|  FUNCTION: EHbisect                                                         |
|                                                                             |
|  DESCRIPTION: Finds root of function using bisection                        |
|                                                                             |
|                                                                             |
|  Return Value    Type     Units     Description                             |
|  ============   ======  =========   =====================================   |
|  status         intn                return status (0) SUCCEED, (-1) FAIL    |
|                                                                             |
|  INPUTS:                                                                    |
|  func()         float64             Function to bisect                      |
|  funcParms      float64             Function parameters (fixed)             |
|  nParms         int32               Number of function parameters           |
|  limLft         float64             Lower limit of function argument       |
|  limRgt         float64             Upper limit of function argument       |
|  convCrit       float64             Convergence criterion                   |
|                                                                             |
|  OUTPUTS:                                                                   |
|  root           float64             Function root                           |
|                                                                             |
|  NOTES:                                                                     |
|                                                                             |
|                                                                             |
|   Date     Programmer   Description                                         |
|  ======   ============  =================================================   |
|  Nov 96   Joel Gales    Original Programmer                                 |
|                                                                             |
|  END_PROLOG                                                                 |
-----------------------------------------------------------------------------*/
intn
EHbisect(float64(*func) (float64[]), float64 funcParms[], int32 nParms,
	 float64 limLft, float64 limRgt, float64 convCrit, float64 * root)
{
    intn            i;		/* Loop index */
    intn            status = 0;	/* routine return status variable */

    float64         midPnt;	/* Mid-point value */
    float64         newmidPnt;	/* New mid-point value */
    float64         funcLft;	/* Function value at left-hand limit */
    float64         funcMid;	/* Function value at mid-point */
    float64         funcRgt;	/* Function value at right-hand limit */
    float64        *parms;	/* Function parameters */


    /* Allocate space for function parameters */
    /* -------------------------------------- */
    parms = (float64 *) calloc(nParms + 1, sizeof(float64));
    if(parms == NULL)
    { 
	HEpush(DFE_NOSPACE, "EHbisect", __FILE__, __LINE__);
	return(-1);
    }


    /* Copy (fixed) function parameters */
    /* -------------------------------- */
    for (i = 0; i < nParms; i++)
    {
	parms[i + 1] = funcParms[i];
    }


    /* Copy left-hand limit to "floating" parameter */
    /* -------------------------------------------- */
    parms[0] = limLft;


    /* Determine function value */
    /* ------------------------ */
    funcLft = (*func) (parms);


    /* Copy right-hand limit to "floating" parameter */
    /* --------------------------------------------- */
    parms[0] = limRgt;


    /* Determine function value */
    /* ------------------------ */
    funcRgt = (*func) (parms);


    /* If left and right limits function values of same sign then no root */
    /* ------------------------------------------------------------------ */
    if (funcLft * funcRgt > 0)
    {
	free(parms);
	return (-1);
    }
    /* Compute (initial) mid-point */
    /* --------------------------- */
    newmidPnt = 0.5 * (limLft + limRgt);


    /* Bisection Loop */
    /* -------------- */
    while (1)
    {
	/* Compute function at new mid-point */
	/* --------------------------------- */
	midPnt = newmidPnt;
	parms[0] = midPnt;
	funcMid = (*func) (parms);


	/* If left limit same sign as mid-point move it to mid-point */
	/* --------------------------------------------------------- */
	if (funcLft * funcMid > 0.0)
	{
	    limLft = midPnt;
	} else
	{
	    /* Otherwise move over right-hand limit */
	    /* ------------------------------------ */
	    limRgt = midPnt;
	}


	/* Compute new mid-point */
	/* --------------------- */
	newmidPnt = 0.5 * (limLft + limRgt);


	/* If relative change in midpoint < convergence crit then exit loop */
	/* ---------------------------------------------------------------- */
	if (fabs((newmidPnt - midPnt) / midPnt) < convCrit)
	{
	    break;
	}
    }

    /* Save root */
    /* --------- */
    *root = newmidPnt;


    free(parms);

    return (status);
}




/*----------------------------------------------------------------------------|
|  BEGIN_PROLOG                                                               |
|                                                                             |
|  FUNCTION: EHattr                                                           |
|                                                                             |
|  DESCRIPTION: Reads/Writes attributes for HDF-EOS structures                |
|                                                                             |
|                                                                             |
|  Return Value    Type     Units     Description                             |
|  ============   ======  =========   =====================================   |
|  status         intn                return status (0) SUCCEED, (-1) FAIL    |
|                                                                             |
|  INPUTS:                                                                    |
|  fid            int32               HDF-EOS file ID                         |
|  attrVgrpID     int32               Attribute Vgroup ID                     |
|  attrname       char                attribute name                          |
|  numbertype     int32               attribute HDF numbertype                |
|  count          int32               Number of attribute elements            |
|  wrcode         char                Read/Write Code "w/r"                   |
|  datbuf         void                I/O buffer                              |
|                                                                             |
|                                                                             |
|  OUTPUTS:                                                                   |
|  datbuf         void                I/O buffer                              |
|                                                                             |
|  NOTES:                                                                     |
|                                                                             |
|                                                                             |
|   Date     Programmer   Description                                         |
|  ======   ============  =================================================   |
|  Jun 96   Joel Gales    Original Programmer                                 |
|  Oct 96   Joel Gales    Pass Vgroup id as routine parameter                 |
|  Oct 96   Joel Gales    Remove Vdetach call                                 |
|                                                                             |
|  END_PROLOG                                                                 |
-----------------------------------------------------------------------------*/
intn
EHattr(int32 fid, int32 attrVgrpID, const char *attrname, int32 numbertype,
       int32 count, const char *wrcode, VOIDP datbuf)

{
    intn            status = 0;	/* routine return status variable */
    int32           vdataID;	/* Attribute Vdata ID */
    uint8          *p=NULL;

    p = (uint8 *)datbuf;

    /*
     * Attributes are stored as Vdatas with name given by the user, class:
     * "Attr0.0" and fieldname: "AttrValues"
     */


    /* Get Attribute Vdata ID and "open" with appropriate I/O code */
    /* ----------------------------------------------------------- */
    vdataID = EHgetid(fid, attrVgrpID, attrname, 1, wrcode);

    /* Write Attribute Section */
    /* ----------------------- */
    if (strcmp(wrcode, "w") == 0)
    {
	/* Create Attribute Vdata (if it doesn't exist) */
	/* -------------------------------------------- */
	if (vdataID == -1)
	{
	    vdataID = VSattach(fid, -1, "w");
	    VSsetname(vdataID, attrname);
	    VSsetclass(vdataID, "Attr0.0");

	    VSfdefine(vdataID, "AttrValues", numbertype, count);
	    Vinsert(attrVgrpID, vdataID);
	}
	/* Write Attribute */
	/* --------------- */
	VSsetfields(vdataID, "AttrValues");
	(void) EHsizeof(vdataID, "AttrValues");
	/* VSwrite(vdataID, datbuf, 1, FULL_INTERLACE); */
	VSwrite(vdataID, p, 1, FULL_INTERLACE);

	VSdetach(vdataID);
    }
    /* Read Attribute Section */
    /* ---------------------- */
    if (strcmp(wrcode, "r") == 0)
    {
	/* If attribute doesn't exist report error */
	/* --------------------------------------- */
	if (vdataID == -1)
	{
	    status = -1;
	    HEpush(DFE_GENAPP, "EHattr", __FILE__, __LINE__);
	    HEreport("Attribute %s not defined.\n", attrname);
	} else
	{
	    VSsetfields(vdataID, "AttrValues");
	    (void) EHsizeof(vdataID, "AttrValues");
	    /*VSread(vdataID, datbuf, 1, FULL_INTERLACE);*/
	    VSread(vdataID, p, 1, FULL_INTERLACE);
	    VSdetach(vdataID);
	}
    }
    return (status);
}




/*----------------------------------------------------------------------------|
|  BEGIN_PROLOG                                                               |
|                                                                             |
|  FUNCTION: EHattrinfo                                                       |
|                                                                             |
|  DESCRIPTION: Returns numbertype and count of given HDF-EOS attribute       |
|                                                                             |
|                                                                             |
|  Return Value    Type     Units     Description                             |
|  ============   ======  =========   =====================================   |
|  status         intn                return status (0) SUCCEED, (-1) FAIL    |
|                                                                             |
|  INPUTS:                                                                    |
|  fid            int32               HDF-EOS file ID                         |
|  attrVgrpID     int32               Attribute Vgroup ID                     |
|  attrname       char                attribute name                          |
|                                                                             |
|  OUTPUTS:                                                                   |
|  numbertype     int32               attribute HDF numbertype                |
|  count          int32               Number of attribute elements            |
|                                                                             |
|  NOTES:                                                                     |
|                                                                             |
|                                                                             |
|   Date     Programmer   Description                                         |
|  ======   ============  =================================================   |
|  Jun 96   Joel Gales    Original Programmer                                 |
|  Oct 96   Joel Gales    Pass Vgroup id as routine parameter                 |
|  Oct 96   Joel Gales    Remove Vdetach call                                 |
|                                                                             |
|  END_PROLOG                                                                 |
-----------------------------------------------------------------------------*/
intn
EHattrinfo(int32 fid, int32 attrVgrpID, const char *attrname, int32 * numbertype,
	   int32 * count)

{
    intn            status = 0;	/* routine return status variable */
    int32           vdataID;	/* Attribute Vdata ID */

    /* Get Attribute Vdata ID */
    /* ---------------------- */
    vdataID = EHgetid(fid, attrVgrpID, attrname, 1, "r");

    /* If attribute not defined then report error */
    /* ------------------------------------------ */
    if (vdataID == -1)
    {
	status = -1;
	HEpush(DFE_GENAPP, "EHattr", __FILE__, __LINE__);
	HEreport("Attribute %s not defined.\n", attrname);
    } else
    {
	/* Get attribute info */
	/* ------------------ */
	VSsetfields(vdataID, "AttrValues");
	*count = EHsizeof(vdataID, "AttrValues");
	*numbertype = VFfieldtype(vdataID, 0);
	VSdetach(vdataID);
    }

    return (status);
}


int32 EHsizeof( int32 vdataID, const char *name)
{
  char *tmpname=NULL;
  int32 recSize = 0;
  tmpname = (char *)malloc(((int)strlen(name)+1)*sizeof(char));
  strcpy(tmpname, name);
  recSize = VSsizeof(vdataID, tmpname);
  free(tmpname);
  tmpname = NULL;
  return recSize;
}


/*----------------------------------------------------------------------------|
|  BEGIN_PROLOG                                                               |
|                                                                             |
|  FUNCTION: EHattrcat                                                        |
|                                                                             |
|  DESCRIPTION: Returns a listing of attributes within an HDF-EOS structure   |
|                                                                             |
|                                                                             |
|  Return Value    Type     Units     Description                             |
|  ============   ======  =========   =====================================   |
|  nattr          int32               Number of attributes in swath struct    |
|                                                                             |
|  INPUTS:                                                                    |
|  fid            int32               HDF-EOS file ID                         |
|  attrVgrpID     int32               Attribute Vgroup ID                     |
|  structcode     char                Structure Code ("s/g/p")                |
|                                                                             |
|  OUTPUTS:                                                                   |
|  attrnames      char                Attribute names in swath struct         |
|                                     (Comma-separated list)                  |
|  strbufsize     int32               Attributes name list string length      |
|                                                                             |
|  NOTES:                                                                     |
|                                                                             |
|                                                                             |
|   Date     Programmer   Description                                         |
|  ======   ============  =================================================   |
|  Jun 96   Joel Gales    Original Programmer                                 |
|  Oct 96   Joel Gales    Pass Vgroup id as routine parameter                 |
|  Oct 96   Joel Gales    Remove Vdetach call                                 |
|                                                                             |
|  END_PROLOG                                                                 |
-----------------------------------------------------------------------------*/
int32
EHattrcat(int32 fid, int32 attrVgrpID, char *attrnames, int32 * strbufsize)
{
    intn            i;		/* Loop index */

    int32           nObjects;	/* # of objects in Vgroup */
    int32          *tags;	/* Pnt to Vgroup object tags array */
    int32          *refs;	/* Pnt to Vgroup object refs array */
    int32           vdataID;	/* Attribute Vdata ID */

    int32           nattr = 0;	/* Number of attributes */
    int32           slen;	/* String length */

    char            name[80];	/* Attribute name */
    const char     *indxstr = "INDXMAP:";	/* Index Mapping reserved
						 * string */
    const char     *fvstr = "_FV_";	/* Flag Value reserved string */
    const char     *bsom = "_BLKSOM:";	/* Block SOM Offset reserved string */


    /* Set string buffer size to 0 */
    /* --------------------------- */
    *strbufsize = 0;


    /* Get number of attributes within Attribute Vgroup */
    /* ------------------------------------------------ */
    nObjects = Vntagrefs(attrVgrpID);


    /* If attributes exist ... */
    /* ----------------------- */
    if (nObjects > 0)
    {
	/* Get tags and references of attribute Vdatas */
	/* ------------------------------------------- */
	tags = (int32 *) malloc(sizeof(int32) * nObjects);
	if(tags == NULL)
	{ 
	    HEpush(DFE_NOSPACE,"EHattrcat", __FILE__, __LINE__);
	    return(-1);
	}
	refs = (int32 *) malloc(sizeof(int32) * nObjects);
	if(refs == NULL)
	{ 
	    HEpush(DFE_NOSPACE,"EHattrcat", __FILE__, __LINE__);
	    free(tags);
	    return(-1);
	}

	Vgettagrefs(attrVgrpID, tags, refs, nObjects);

	/* Get attribute vdata IDs and names */
	/* --------------------------------- */
	for (i = 0; i < nObjects; i++)
	{
	    vdataID = VSattach(fid, *(refs + i), "r");
	    VSgetname(vdataID, name);

	    /*
	     * Don't return fill value, index mapping & block SOM attributes
	     */
	    if (memcmp(name, indxstr, strlen(indxstr)) != 0 &&
		memcmp(name, fvstr, strlen(fvstr)) != 0 &&
		memcmp(name, bsom, strlen(bsom)) != 0)
	    {
		/* Increment attribute counter and add name to list */
		/* ------------------------------------------------ */
		nattr++;
		if (attrnames != NULL)
		{
		    if (nattr == 1)
		    {
			strcpy(attrnames, name);
		    } else
		    {
			strcat(attrnames, ",");
			strcat(attrnames, name);
		    }
		}
		/* Increment attribute names string length */
		/* --------------------------------------- */
		slen = (nattr == 1) ? (int32)strlen(name) : (int32)strlen(name) + 1;
		*strbufsize += slen;
	    }
	    VSdetach(vdataID);
	}
	free(tags);
	free(refs);
    }
    return (nattr);
}



/*----------------------------------------------------------------------------|
|  BEGIN_PROLOG                                                               |
|                                                                             |
|  FUNCTION: EHinquire                                                        |
|                                                                             |
|  DESCRIPTION: Returns number and names of HDF-EOS structures in file        |
|                                                                             |
|                                                                             |
|  Return Value    Type     Units     Description                             |
|  ============   ======  =========   =====================================   |
|  nobj           int32               Number of HDF-EOS structures in file    |
|                                                                             |
|  INPUTS:                                                                    |
|  filename       char                HDF-EOS filename                        |
|  type           char                Object Type ("SWATH/GRID/POINT")        |
|                                                                             |
|  OUTPUTS:                                                                   |
|  objectlist     char                List of object names (comma-separated)  |
|  strbufsize     int32               Length of objectlist                    |
|                                                                             |
|  NOTES:                                                                     |
|                                                                             |
|                                                                             |
|   Date     Programmer   Description                                         |
|  ======   ============  =================================================   |
|  Jun 96   Joel Gales    Original Programmer                                 |
|  Jun 10   Abe Taaheri   Fixed core dump for internal HDF4 VGroup(created by |
|                         SDstart()                                           |
|                                                                             |
|  END_PROLOG                                                                 |
-----------------------------------------------------------------------------*/
int32
EHinquire(const char *filename, const char *type, char *objectlist, int32 * strbufsize)
{
    int32           HDFfid;	/* HDF file ID */
    int32           vgRef;	/* Vgroup reference number */
    int32           vGrpID;	/* Vgroup ID */
    int32           nobj = 0;	/* Number of HDFEOS objects in file */
    int32           slen;	/* String length */

    char            name[80];	/* Object name */
    char            a_class[UTLSTR_MAX_SIZE];	/* Object class */
    char            tempname[UTLSTR_MAX_SIZE];
    int32           status;

    /* Open HDFEOS file of read-only access */
    /* ------------------------------------ */
    HDFfid = Hopen(filename, DFACC_READ, 0);


    /* Start Vgroup Interface */
    /* ---------------------- */
    Vstart(HDFfid);


    /* If string buffer size is requested then zero out counter */
    /* -------------------------------------------------------- */
    if (strbufsize != NULL)
    {
	*strbufsize = 0;
    }
    /* Search for objects from beginning of HDF file */
    /* -------------------------------------------- */
    vgRef = -1;

    /* Loop through all objects */
    /* ------------------------ */
    while (1)
    {
	/* Get Vgroup reference number */
	/* --------------------------- */
	vgRef = Vgetid(HDFfid, vgRef);

	/* If no more then exist search loop */
	/* --------------------------------- */
	if (vgRef == -1)
	{
	    break;
	}
	/* Get Vgroup ID, name, and class */
	/* ------------------------------ */
	vGrpID = Vattach(HDFfid, vgRef, "r");
	Vgetname(vGrpID, tempname);
	Vgetclass(vGrpID, a_class);


	/* If object of desired type (SWATH, POINT, GRID) ... */
	/* -------------------------------------------------- */
	if (strcmp(a_class, type) == 0)
	{
	  /* Objects with name length of less than VGNAMELENMAX are real hdf-eos objects */
 
	    if ((intn) strlen(tempname) < VGNAMELENMAX)
	      {
		strcpy(name,tempname);

		/* Increment counter */
		/* ----------------- */
		nobj++;

		/* If object list requested add name to list */
		/* ----------------------------------------- */
		if (objectlist != NULL)
		  {
		    if (nobj == 1)
		      {
			strcpy(objectlist, name);
		      } else
		      {
			strcat(objectlist, ",");
			strcat(objectlist, name);
		      }
		  }
		/* Compute string length of object entry */
		/* ------------------------------------- */
		slen = (nobj == 1) ? (int32)strlen(name) : (int32)strlen(name) + 1;
		
		
		/* If string buffer size is requested then increment buffer size */
		/* ------------------------------------------------------------- */
		if (strbufsize != NULL)
		  {
		    *strbufsize += slen;
		  }
	      }
	}
	/* Detach Vgroup */
	/* ------------- */
	Vdetach(vGrpID);
    }

    /* "Close" Vgroup interface and HDFEOS file */
    /* ---------------------------------------- */
    Vend(HDFfid);
    Hclose(HDFfid);

    return (nobj);
}



/*----------------------------------------------------------------------------|
|  BEGIN_PROLOG                                                               |
|                                                                             |
|  FUNCTION: EHclose                                                          |
|                                                                             |
|  DESCRIPTION: Closes HDF-EOS file                                           |
|                                                                             |
|                                                                             |
|  Return Value    Type     Units     Description                             |
|  ============   ======  =========   =====================================   |
|  status         intn                return status (0) SUCCEED, (-1) FAIL    |
|                                                                             |
|  INPUTS:                                                                    |
|  fid            int32               HDF-EOS File ID                         |
|                                                                             |
|  OUTPUTS:                                                                   |
|             None                                                            |
|                                                                             |
|  NOTES:                                                                     |
|                                                                             |
|                                                                             |
|   Date     Programmer   Description                                         |
|  ======   ============  =================================================   |
|  Jun 96   Joel Gales    Original Programmer                                 |
|  Jul 96   Joel Gales    Add file id offset EHIDOFFSET                       |
|  Aug 96   Joel Gales    Add HE error report if file id out of bounds        |
|  Nov 96   Joel Gales    Add EHXacsTable array to "garbage collection"       |
|                                                                             |
|  END_PROLOG                                                                 |
-----------------------------------------------------------------------------*/
intn
EHclose(int32 fid)
{
    intn            status = 0;	/* routine return status variable */

    int32           HDFfid;	/* HDF file ID */
    int32           sdInterfaceID = 0;	/* HDF SDS interface ID */
    int32           fid0;	/* HDF EOS file id - offset */


    /* Check for valid HDFEOS file ID range */
    /* ------------------------------------ */
    if (fid >= EHIDOFFSET && fid < NEOSHDF + EHIDOFFSET)
    {
	/* Compute "reduced" file ID */
	/* ------------------------- */
	fid0 = fid % EHIDOFFSET;


	/* Get HDF file ID and SD interface ID */
	/* ----------------------------------- */
	HDFfid = EHXfidTable[fid0];
	sdInterfaceID = EHXsdTable[fid0];

	/* "Close" SD interface, Vgroup interface, and HDF file */
	/* ---------------------------------------------------- */
	status = SDend(sdInterfaceID);
	status = Vend(HDFfid);
	status = Hclose(HDFfid);

	/* Clear out external array entries */
	/* -------------------------------- */
	EHXtypeTable[fid0] = 0;
	EHXacsTable[fid0] = 0;
	EHXfidTable[fid0] = 0;
	EHXsdTable[fid0] = 0;
    } else
    {
	status = -1;
	HEpush(DFE_RANGE, "EHclose", __FILE__, __LINE__);
	HEreport("Invalid file id: %d.  ID must be >= %d and < %d.\n",
		 fid, EHIDOFFSET, NEOSHDF + EHIDOFFSET);
    }

    return (status);
}



/*----------------------------------------------------------------------------|
|  BEGIN_PROLOG                                                               |
|                                                                             |
|  FUNCTION: EHHEisHE2                                                        |
|                                                                             |
|  DESCRIPTION: Find out if hdf file is HDF-EOS2 file (HDF4 based hdf-eos)    |
|                                                                             |
|                                                                             |
|  Return Value    Type     Units     Description                             |
|  ============   ======  =========   =====================================   |
|  retstatus      intn                return TRUE  (or 1) if file is HE2      |
|                                       (can be opened by EHopen and has      |
|                                        SWATH, GRID or POINT objects )       |
|                                     return FALSE (or 0) otherwise           |
|                                     return -1 if file does not exist        |
|                                                                             |
|  INPUTS:                                                                    |
|  filename        char               Input file name                         |
|                                                                             |
|  OUTPUTS:                                                                   |
|  None                                                                       |
|                                                                             |
|  NOTES: Although any HDF4 file with ECS and structure metadata is HDFEOS    |
|         file, but in practice we will consider a file as HDFEOS2 file if it |
|         is HDF4 based and contains one or more of the objects GRID, SWATH,  |
|         and POINT.                                                          |
|                                                                             |
|   Date     Programmer   Description                                         |
|  ======   ============  =================================================   |
|  Dec 13   Abe Taaheri   Original Programmer                                 |
|                                                                             |
|  END_PROLOG                                                                 |
-----------------------------------------------------------------------------*/
intn
EHHEisHE2(char *filename)
{
  intn  status = 0;	/* return status variable for calls */
  intn  retstatus = 0;	/* routine return status variable */
  int32 fid;
  FILE *fp;
  int32           nSwath;	/* Number of swath structures in file */
  int32           nGrid;	/* Number of grid structures in file */
  int32           nPoint;	/* Number of point structures in file */

  if ((fp = fopen(filename,"r")) == NULL)
    {
      /* file does not exist */
      return(-1);
    }
  else
    {
      fclose( fp );
    }

  /* Open file using EHopen                   */
  /* If fails, then file is not HE2 file      */
  /* ---------------------------------------- */
  
  /* open HDF-EOS file for reading */
  fid = EHopen( filename, DFACC_READ );
  if ( fid < 0 )
    {
      return(0); /* File is not HDF4 or HDFEOS2 file */
    }
  else
    {
      EHclose( fid );
    }

  /* File is  HDF4 or HDFEOS2 file. See if it is HDFEOS2 */

  /* Call "EHinquire" routine for grid */
  /* --------------------------------- */
  nGrid = EHinquire(filename, "GRID", NULL, NULL);

  if(nGrid == 0)
    {
      /* Call "EHinquire" routine for swath */
      /* ---------------------------------- */
      nSwath = EHinquire(filename, "SWATH", NULL, NULL);
      if(nSwath == 0)
	{
	  /* Call EHinquire for point */
	  /* ------------------------ */
	  nPoint = EHinquire(filename, "POINT", NULL, NULL);
	  if(nPoint == 0)
	    {
	      retstatus = 0;
	    }
	  else
	    {
	      retstatus = 1;
	    }
	}
      else
	{
	  retstatus = 1;
	}
    }
  else
    {
      retstatus = 1;
    }

  if( retstatus == 0 )
    {
      /* Failed to find any grid, swath, or point opjects */
      /* File is not HE2 file */
      return(0);
    }
  else
    {
      /* File is HE2 file */
      return(1);
    }
}

/* Close all HDF files left open */

intn EHcloseall()
{
  intn            status=0;
  int32           HDFfid;	        /* HDF file ID */
  int32           sdInterfaceID = 0;	/* HDF SDS interface ID */
  int32           i;	                /* HDF EOS file id - offset */

  /* Loop through all files */
  /* ---------------------- */
  for (i = 0; i < NEOSHDF; i++)
    {
      /* if entry is active file opened for write access ... */
      /* --------------------------------------------------- */
      if (EHXtypeTable[i] != 0 && EHXacsTable[i] == 1)
	{
	  /* Get HDF file ID and SD interface ID */
	  /* ----------------------------------- */
	  HDFfid = EHXfidTable[i];
	  sdInterfaceID = EHXsdTable[i];
	  
	  /* "Close" SD interface, Vgroup interface, and HDF file */
	  /* ---------------------------------------------------- */
	  status = SDend(sdInterfaceID);
	  status = Vend(HDFfid);
	  status = Hclose(HDFfid);
	  /* Clear out external array entries */
	  /* -------------------------------- */
	  EHXtypeTable[i] = 0;
	  EHXacsTable[i] = 0;
	  EHXfidTable[i] = 0;
	  EHXsdTable[i] = 0;
	}
      return (status);
    }
}



/* FORTRAN bindings */
/* -------------------------- */

#ifdef FORTRAN_API
FORTRAN_API_FN(INT, EHidinfo, EHIDINFO, ehidinfo, INT32, OUT INT32, OUT INT32)
FORTRAN_API_FN(FLOAT64, EHconvAng, EHCONVANG, ehconvang, FLOAT64, INT)
FORTRAN_API_FN(INT, EHHEisHE2, EHHEISHE2, ehheishe2, STRING)
FORTRAN_API_FN(INT, EHgetversion, EHGETVER, ehgetver, INT32, OUT STRING)
#endif // FORTRAN_API