/*******************************************************************************
NAME                            POLAR STEREOGRAPHIC 

PURPOSE:	Transforms input Easting and Northing to longitude and
		latitude for the Polar Stereographic projection.  The
		Easting and Northing must be in meters.  The longitude
		and latitude values will be returned in radians.

PROGRAMMER              DATE
----------              ----
T. Mittan		2-26-93

ALGORITHM REFERENCES

1.  Snyder, John P., "Map Projections--A Working Manual", U.S. Geological
    Survey Professional Paper 1395 (Supersedes USGS Bulletin 1532), United
    State Government Printing Office, Washington D.C., 1987.

2.  Snyder, John P. and Voxland, Philip M., "An Album of Map Projections",
    U.S. Geological Survey Professional Paper 1453 , United State Government
    Printing Office, Washington D.C., 1989.

Note: The equations used here are for Stereographic projection with North Polar 
or South Polar aspect (making the projection Polar Stereographic). 
The lat-true-scale can be any angle other than 90 or -90.
*******************************************************************************/
#include <stdio.h>
#include "cproj.h"

/* Variables common to all subroutines in this code file
  -----------------------------------------------------*/
  static double r_major;                /* major axis                   */
  static double r_minor;                /* minor axis                   */
  static double e;                      /* eccentricity                 */
  static double e4;                     /* e4 calculated from eccentricity*/
  static double center_lon;             /* center longitude             */
  static double center_lat;             /* latitude of true scale 	*/
  static double fac;                    /* sign variable                */
  static int ind;                       /* flag variable                */
  static double mcs;			/* small value m		*/
  static double tcs;			/* small value t	        */
  static double false_northing;		/* y offset in meters		*/
  static double false_easting;		/* x offset in meters		*/

/* Initialize the Polar Stereographic projection
  --------------------------------------------*/
int psinvint(
double r_maj,				/* major axis			*/
double r_min,				/* minor axis			*/
double c_lon,				/* center longitude		*/
double c_lat,				/* latitude of true scale	*/
double false_east,			/* x offset in meters		*/
double false_north)			/* y offset in meters		*/

{
double temp;				/* temporary variable   	*/
double con1;				/* temporary angle		*/
double sinphi;				/* sin value			*/
double cosphi;				/* cos value			*/
double es;                              /* eccentricity squared         */

r_major = r_maj;
r_minor = r_min;
false_easting = false_east;
false_northing = false_north;
temp = r_minor / r_major;
es = 1.0 - SQUARE(temp);
e = sqrt(es);
e4 = e4fn(e);
center_lon = c_lon;
center_lat = c_lat;

/* fac=1 is for North Polar Aspect, fac=-1 is for South Polar Aspect */
if (c_lat < 0)
   fac = -1.0;
else
   fac = 1.0;

/* for latitude of true scale = 90 degrees we will use eq 21-33 of Snyder */

ind = 0;
if (fabs(fabs(c_lat) - HALF_PI) > EPSLN) 
   {/* eq 21-34 of Snyder */
     ind = 1;
     con1 = fac * center_lat; 
     tsincos(con1,&sinphi,&cosphi);
     mcs = msfnz(e,sinphi,cosphi);
     tcs = tsfnz(e,con1,sinphi);
   }
/* Report parameters to the user
  -----------------------------*/
ptitle("POLAR STEREOGRAPHIC");
radius2(r_major, r_minor);
cenlon(center_lon);
offsetp(false_east,false_north);

return(OK);
}

/* Polar Stereographic inverse equations--mapping x,y to lat/long
  --------------------------------------------------------------*/
int psinv(
double x,			/* (O) X projection coordinate 	*/
double y,			/* (O) Y projection coordinate 	*/
double *lon,			/* (I) Longitude 		*/
double *lat)			/* (I) Latitude 	   	*/

{
double rh;			/* height above ellipsiod	*/
double ts;			/* small value t   		*/
double temp;			/* temporary variable    	*/
long   flag;			/* error flag			*/

flag = 0;
x = (x - false_easting) * fac;
y = (y - false_northing) *fac;
rh = sqrt(x * x + y * y);
if (ind != 0)
  ts = rh * tcs/(r_major * mcs);
else
  ts = rh * e4 / (r_major * 2.0); /* assuming k_0 = 1 in eq 21-33 */

*lat = fac * phi2z(e,ts,&flag);

if (flag != 0)
   return(flag);

if (rh == 0)
   *lon = fac * center_lon;
else
   {
   temp = atan2(x, -y);
   *lon = adjust_lon(fac *temp + center_lon);
   }

return(OK);
}
