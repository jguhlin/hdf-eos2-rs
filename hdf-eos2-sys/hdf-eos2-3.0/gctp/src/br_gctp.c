
#ifdef unix


/*  Fortran bridge routine for the UNIX */

void gctp_(incoor,insys,inzone,inparm,inunit,indatum,ipr,efile,jpr,pfile,
               outcoor, outsys,outzone,outparm,outunit,fn27,fn83,iflg)

double *incoor;
long *insys;
long *inzone;
double *inparm;
long *inunit;
long *indatum;
long *ipr;        /* printout flag for error messages. 0=yes, 1=no*/
char *efile;
long *jpr;        /* printout flag for projection parameters 0=yes, 1=no*/
char *pfile;
double *outcoor;
long *outsys;
long *outzone;
double *outparm;
long *outunit;
long *iflg;

{
gctp(incoor,insys,inzone,inparm,inunit,indatum,ipr,efile,jpr,pfile,outcoor,
     outsys,outzone,outparm,outunit,fn27,fn83,iflg);

return;
}
#endif
