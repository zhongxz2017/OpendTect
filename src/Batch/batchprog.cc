/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : 14-9-1998
 * FUNCTION : Batch Program 'driver'
-*/
 
static const char* rcsID = "$Id: batchprog.cc,v 1.52 2004-01-22 14:54:02 dgb Exp $";

#include "batchprog.h"
#include "ioparlist.h"
#include "ioman.h"
#include "strmprov.h"
#include "strmdata.h"
#include "filegen.h"
#include "timefun.h"
#include "sighndl.h"
#include "socket.h"
#include "mmdefs.h"
#include "plugins.h"
#include "strmprov.h"
#ifndef __msvc__
#include <unistd.h>
#include <stdlib.h>
#endif
#include <iostream>

#ifndef __win__
# include "_execbatch.h"
#endif

#define mErrStrm (sdout_.ostrm ? *sdout_.ostrm : cerr)

BatchProgram* BatchProgram::inst_;


BatchProgram::BatchProgram( int* pac, char** av )
	: UserIDObject("")
	, pargc_(pac)
	, argv_(av)
	, argshift_(2)
	, stillok_(false)
	, fullpath_(av[0])
	, inbg_(NO)
	, sdout_(*new StreamData)
    	, masterport_(0)
	, usesock_( false )
	, jobid_( 0 )
	, timestamp_( Time_getMilliSeconds() )
	, exitstat_( mSTAT_UNDEF )
	, pausereq_( false )
{
    const char* fn = argv_[1];
    while ( fn && *fn == '-' )
    {
	if ( !strcmp(fn,"-bg") )
	    inbg_ = YES;
	else if ( !strncmp(fn,"-masterhost",11) )
	{
	    argshift_++;
	    fn = argv_[ argshift_ - 1 ];
	    masterhost_ = fn;
	}
	else if ( !strncmp(fn,"-masterport",11) )
	{
	    argshift_++;
	    fn = argv_[ argshift_ - 1 ];
	    masterport_ = atoi(fn);
	}
	else if ( !strncmp(fn,"-jobid",6) )
	{
	    argshift_++;
	    fn = argv_[ argshift_ - 1 ];
	    jobid_ = atoi(fn);
	}
	else if ( *(fn+1) )
	    opts_ += new BufferString( fn+1 );

	argshift_++;
	fn = argv_[ argshift_ - 1 ];
    }

    usesock_ = masterhost_.size() && masterport_ > 0; // both must be set.
     
    if ( !fn || !*fn )
    {
	BufferString msg( progName() );
	msg += ": No parameter file name specified";

	writeErrorMsg( msg ); cerr << msg << endl;
	return;
    }


    static BufferString parfilnm; parfilnm = fn;
    replaceCharacter(parfilnm.buf(),'%',' ');
    fn = parfilnm;

    setName( fn );
    StreamData sd = StreamProvider( fn ).makeIStream();
    if ( !sd.usable() )
    {
	BufferString msg( name() );
	msg += ": Cannot open parameter file: ";
	msg += fn;

	writeErrorMsg( msg ); cerr << msg << endl;
	return;
    }
 
    IOParList parlist( *sd.istrm );
    sd.close();
    if ( parlist.size() == 0 )
    {
	BufferString msg( argv_[0] );
	msg += ": Invalid input file: ";
	msg += fn;

	writeErrorMsg( msg ); cerr << msg << endl;
        return;
    }

    iopar_ = new IOPar( *parlist[0] );

    const char* res = iopar_->find( "Log file" );
    if ( !res )
	iopar_->set( "Log file", StreamProvider::sStdErr );
    res = iopar_->find( "Survey" );
    if ( !res || !strcmp(res,IOM().surveyName()) )
	IOMan::newSurvey();
    else
    {
#ifdef __debug__
	std::cerr << "Using survey: " << res << " instead of "
	    	  << IOM().surveyName() << endl;
#endif
	IOMan::setSurvey( res );
    }

    killNotify( true );

    stillok_ = true;
}


BatchProgram::~BatchProgram()
{
    mErrStrm << "Finished batch processing." << endl;

    if ( exitstat_ == mSTAT_UNDEF ) 
	exitstat_ = stillok_ ? mSTAT_DONE : mSTAT_ERROR;

    writeStatus( mEXIT_STATUS, exitstat_ );

    killNotify( false );

    sdout_.close();
    delete &sdout_;
    deepErase( opts_ );
}


const char* BatchProgram::progName() const
{
    static UserIDString ret;
    ret = File_getFileName( fullpath_ );
    return ret;
}


void BatchProgram::progKilled( CallBacker* )
{
    mErrStrm << "BatchProgram Killed." << endl;

    exitstat_ = mSTAT_KILLED;
    writeStatus( mEXIT_STATUS, exitstat_ );
    killNotify( false );

#ifdef __debug__
    abort();
#endif

}


void BatchProgram::killNotify( bool yn )
{
    CallBack cb( mCB(this,BatchProgram,progKilled) );

    if ( yn )
	SignalHandling::startNotify( SignalHandling::Kill, cb );
    else
	SignalHandling::stopNotify( SignalHandling::Kill, cb );
}



bool BatchProgram::writeStatus_( char tag , int status, const char* errmsg,
				 bool force )
{
    static int maxelapsed  = atoi( getenv("DTECT_MM_MILLISECS")
				 ? getenv("DTECT_MM_MILLISECS") : "1000");
    // if ( !usesock_ ) return true;

    int elapsed = Time_getMilliSeconds() - timestamp_;
    if ( elapsed < 0 )
    {
	mErrStrm << "System clock skew detected (Ignored)." << endl;
	force = true;
    }

    bool hasmessage = errmsg && *errmsg;
    if ( !force && !hasmessage && elapsed < maxelapsed )
	return true;

    timestamp_ = Time_getMilliSeconds();

    if ( !usesock_ ) return true;

    Socket* sock = mkSocket();
    if ( !sock || !sock->ok() )
    {
	mErrStrm << "Cannot open communication socket to Master." << endl;
	return false;
    }

    if ( hasmessage )
	sock->writeErrorMsg( errmsg );

    sock->writetag( tag, jobid_, status );

    bool ret = true;

    char masterinfo;
    BufferString errbuf;

    ret = sock->readtag( masterinfo );
    if ( !ret )
    {
	sock->fetchMsg( errbuf );
    }
    else if ( masterinfo != mRSP_ACK )
    {
	if ( masterinfo == mRSP_REQ_STOP ) 
	{
	    mErrStrm << "Exiting on request of Master." << endl;
	    exit( -1 );
	}

	else if ( masterinfo == mRSP_REQ_PAUSE )
	    pausereq_ = true; 

	else if ( masterinfo == mRSP_REQ_CONT )
	    pausereq_ = false;  

	else
	{
	    errbuf = "Master sent an unkown response code.";
	    ret = false;
	}
    }

    if ( !ret )
    {
	mErrStrm << "Error writing status to Master: " << errbuf << endl;
    }

    delete sock;
    return ret;
}


bool BatchProgram::writeErrorMsg( const char* msg )
{
    if ( !usesock_ ) return false;

    if ( Socket* sock = mkSocket() )
    {
	return sock->writeErrorMsg( msg );
    }

    return false;
}


Socket* BatchProgram::mkSocket()
{
    if ( !usesock_ ) return 0;

    return new Socket( masterhost_, masterport_ );
}

bool BatchProgram::initOutput()
{
    stillok_ = false;
    if ( !writeStatus( mPID_TAG, getPID() ) )
    {
	mErrStrm << "Could not write status. Exiting." << endl;
	exit( 0 );
    }

    const char* res = pars()["Log file"];
    if ( !*res || !strcmp(res,"stdout") ) res = 0;
 
#ifndef __win__
    if ( res && !strcmp(res,"window") )
    {
	FileNameString comm( "@view_progress " );
	comm += getPID();
	StreamProvider sp( comm );
	sdout_ = sp.makeOStream();
	if ( !sdout_.usable() )
	{
	    cerr << name() << ": Cannot open window for output" << endl;
	    cerr << "Using std output instead" << endl;
	    res = 0;
	}
    }
#endif
 
    if ( !res || strcmp(res,"window") )
    {
	StreamProvider spout( res );
	sdout_ = spout.makeOStream();
	if ( !sdout_.ostrm )
	{
	    cerr << name() << ": Cannot open log file" << endl;
	    cerr << "Using std output instead" << endl;
	    sdout_.ostrm = &cout;
	}
    }

    stillok_ = sdout_.usable();
    if ( stillok_ )
	PIM().loadAuto( true );
    return stillok_;
}
