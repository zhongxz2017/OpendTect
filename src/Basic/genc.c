/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : March 1994
 * FUNCTION : general utilities
-*/

static const char* rcsID = "$Id: genc.c,v 1.108 2010-05-21 14:58:21 cvsbert Exp $";

#include "genc.h"
#include "string2.h"
#include "envvars.h"
#include "mallocdefs.h"
#include "debugmasks.h"
#include "oddirs.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef __win__
# include <unistd.h>
#else
# include <float.h>
# include <time.h>
# include <sys/timeb.h>
# include <shlobj.h>
#endif


static int insysadmmode_ = 0;
mGlobal int InSysAdmMode() { return insysadmmode_; }
mGlobal void SetInSysAdmMode() { insysadmmode_ = 1; }


const char* GetLocalHostName()
{
    static char ret[256];
    gethostname( ret, 256 );
    return ret;
}


void SwapBytes( void* p, int n )
{
    int nl = 0;
    unsigned char* ptr = (unsigned char*)p;
    unsigned char c;

    if ( n < 2 ) return;
    n--;
    while ( nl < n )
    { 
	c = ptr[nl]; ptr[nl] = ptr[n]; ptr[n] = c;
	nl++; n--;
    }
}


void PutIsLittleEndian( unsigned char* ptr )
{
#ifdef __little__
    *ptr = 1;
#else
    *ptr = 0;
#endif
}

#ifdef __msvc__
#define getpid	_getpid
#endif

int GetPID()
{
    return getpid();
}


void NotifyExitProgram( PtrAllVoidFn fn )
{
    static int nrfns = 0;
    static PtrAllVoidFn fns[100];
    int idx;
    if ( ((od_int64)fn) == ((od_int64)(-1)) )
    {
	for ( idx=0; idx<nrfns; idx++ )
	    (*(fns[idx]))();
    }
    else
    {
	fns[nrfns] = fn;
	nrfns++;
    }
}


extern const char* errno_message();

mGlobal void forkProcess()
{
#if !defined( __mac__ ) && !defined( __win__ )
    switch ( fork() )
    {
    case 0:     break;
    case -1:
	fprintf( stderr, "Cannot fork new process: %s\n", errno_message() );
    default:
	ExitProgram( 0 );
    }
#endif
}


#define isBadHandle(h) ( (h) == NULL || (h) == INVALID_HANDLE_VALUE )

int isProcessAlive( int pid )
{
#ifdef __win__
    HANDLE hProcess = OpenProcess( PROCESS_QUERY_INFORMATION, FALSE, GetPID() );
    return isBadHandle(hProcess) ? 0 : 1;
#else
    const int res = kill( pid, 0 );
    return res == 0 ? 1 : 0;
#endif
}


const char* getFullDateString( void )
{
    char *chp ;
    int lastch ;

#ifdef __win__
    return 0; // TODO
#else
	const time_t timer = time(NULL);
    chp = ctime( &timer );

    lastch = strlen( chp ) - 1 ;
    if ( chp[lastch] == '\n' ) chp[lastch] = '\0' ;

    return chp;
#endif
}


int ExitProgram( int ret )
{
    if ( od_debug_isOn(DBG_PROGSTART) )
	printf( "\nExitProgram (PID: %d) at %s\n",
		GetPID(), getFullDateString() );

    NotifyExitProgram( (PtrAllVoidFn)(-1) );

// On Mac OpendTect crashes when calling the usual exit and shows error message:
// dyld: odmain bad address of lazy symbol pointer passed to stub_binding_helper
// _Exit does not call registered exit functions and prevents crash
#ifdef __mac__
    _Exit(0);
    return 0;
#endif

#ifdef __msvc__
    exit( EXIT_SUCCESS );
    return 0;
#else

#ifdef __win__


    // open process
    HANDLE hProcess = OpenProcess( PROCESS_TERMINATE, FALSE, GetPID() );
    if ( isBadHandle( hProcess ) )
	printf( "OpenProcess() failed, err = %lu\n", GetLastError() );
    else
    {
	// kill process
	if ( ! TerminateProcess( hProcess, (DWORD) -1 ) )
	    printf( "TerminateProcess() failed, err = %lu\n", GetLastError() );

	// close handle
	CloseHandle( hProcess );
    }
#endif

    exit(ret);
    return ret;
#endif
}


/*-> envvar.h */

char* GetOSEnvVar( const char* env )
{
    return getenv( env );
}


#define mMaxNrEnvEntries 1024
typedef struct _GetEnvVarEntry
{
    char	varname[128];
    char	value[1024];
} GetEnvVarEntry;


static void loadEntries( const char* fnm, int* pnrentries,
    			 GetEnvVarEntry* entries[] )
{
    static FILE* fp;
    static char linebuf[1024];
    static char* ptr;
    static const char* varptr;

    fp = fnm && *fnm ? fopen( fnm, "r" ) : 0;
    if ( !fp ) return;

    while ( fgets(linebuf,1024,fp) )
    {
	ptr = linebuf;
	mSkipBlanks(ptr);
	varptr = ptr;
	if ( *varptr == '#' || !*varptr ) continue;

	mSkipNonBlanks( ptr );
	if ( !*ptr ) continue;
	*ptr++ = '\0';
	mTrimBlanks(ptr);
	if ( !*ptr ) continue;

	entries[*pnrentries] = mMALLOC(1,GetEnvVarEntry);
	strcpy( entries[*pnrentries]->varname, varptr );
	strcpy( entries[*pnrentries]->value, ptr );
	(*pnrentries)++;
    }
    fclose( fp );
}


const char* GetEnvVar( const char* env )
{
    static int filesread = 0;
    static int nrentries = 0;
    static GetEnvVarEntry* entries[mMaxNrEnvEntries];
    int idx;

    if ( !env || !*env ) return 0;
    if ( insysadmmode_ ) return GetOSEnvVar( env );

    if ( !filesread )
    {
	filesread = 1;
	loadEntries( GetSettingsFileName("envvars"), &nrentries, entries );
	loadEntries( GetSetupDataFileName(ODSetupLoc_ApplSetupOnly,"EnvVars",1),
		     &nrentries, entries );
	loadEntries( GetSetupDataFileName(ODSetupLoc_SWDirOnly,"EnvVars",1),
		     &nrentries, entries );
    }

    for ( idx=0; idx<nrentries; idx++ )
    {
	if ( !strcmp( entries[idx]->varname, env ) )
	    return entries[idx]->value;
    }

    return GetOSEnvVar( env );
}


int GetEnvVarYN( const char* env )
{
    const char* s = GetEnvVar( env );
    return !s || *s == '0' || *s == 'n' || *s == 'N' ? 0 : 1;
}


int GetEnvVarIVal( const char* env, int defltval )
{
    const char* s = GetEnvVar( env );
    return s ? atoi(s) : defltval;
}


double GetEnvVarDVal( const char* env, double defltval )
{
    const char* s = GetEnvVar( env );
    return s ? atof(s) : defltval;
}


int SetEnvVar( const char* env, const char* val )
{
    char* buf;
    if ( !env || !*env ) return mC_False;
    if ( !val ) val = "";

#ifdef __msvc__
    SetEnvironmentVariable( env, val );
    return mC_True;
#else
    buf = mMALLOC( strlen(env)+strlen(val) + 2, char );
    strcpy( buf, env );
    if ( *val ) strcat( buf, "=" );
    strcat( buf, val );

    putenv( buf );
    return mC_True;
#endif
}


char GetEnvSeparChar()
{
#ifdef __win__
    return ';';
#else
    return ':';
#endif
}
