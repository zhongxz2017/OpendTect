/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : 18-8-2000
 * FUNCTION : Help viewing
-*/
 
static const char* rcsID = "$Id: helpview.cc,v 1.5 2002-05-17 11:06:39 dgb Exp $";

#include "helpview.h"
#include "settings.h"
#include "ascstream.h"
#include "multiid.h"
#include "errh.h"
#include "strmprov.h"
#include <stdlib.h>


void HelpViewer::use( const char* url )
{
    if ( !url || !*url ) url = "index.htm";

    FileNameString comm( "netscapeview" );
    const char* ptr = Settings::common()[ "Help viewer" ];
    if ( ptr ) comm = ptr;
    comm += " \""; comm += url;
    comm += "\" &";
    ExecOSCmd( comm );
}


BufferString HelpViewer::getURLForWinID( const char* winid )
{
    FileNameString fname = GetDataFileName( "Help.htm" );

    StreamData sd = StreamProvider( fname ).makeIStream();
    istream& hstrm = *sd.istrm;

    if ( !sd.usable() )
    {
	FileNameString msg( "Help file '" );
	msg += fname;
	msg += "' not available";
	ErrMsg( msg );
	sd.close();
	return BufferString("");
    }

    ascistream astream( hstrm );
    MultiID code[3];
    MultiID wid;
    int lvl;
    const char* ptr = 0;
    while ( 1 )
    {
	char c = hstrm.peek();
	while ( c == '\n' ) { hstrm.ignore( 1 ); c = hstrm.peek(); }
	lvl = 0;
	if ( c == '\t' )
	{
	    lvl++;
	    hstrm.ignore( 1 );
	    c = hstrm.peek();
	    if ( c == '\t' ) lvl++;
	}
	astream.next();
	if ( atEndOfSection(astream) ) break;

	code[lvl] = astream.keyWord();
	for ( int idx=2; idx>lvl; idx-- )
	    code[idx] = "";
	wid = code[0];
	if ( code[1] != "" )
	{
	    wid += code[1];
	    if ( code[2] != "" )
		wid += code[2];
	}
	if ( wid != winid ) continue;

	ptr = astream.value();
	skipLeadingBlanks(ptr);

	// Skip <a href="
	while ( *ptr && *ptr != '"' ) ptr++;
	if ( ! *ptr ) { hstrm.ignore(10000,'\n'); ptr = 0; continue; }
	ptr++;
	const char* endptr = ptr;
	while ( *endptr && *endptr != '"' ) endptr++;
	if ( ! *endptr ) { ptr = 0; continue; }
	*(char*)endptr = '\0';
	break;
    }

    if ( !ptr || ! *ptr )
    {
	BufferString msg = "No help for this window (ID=";
	msg += winid; msg += ").";
	UsrMsg( msg );
    }
    else if ( getenv("dGB_SHOW_HELP") )
    {
	BufferString msg = winid; msg += " -> "; msg += ptr;
	UsrMsg( msg );
    }

    sd.close();
    return BufferString( ptr );
}
