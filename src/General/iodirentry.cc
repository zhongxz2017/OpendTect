/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : April 2001
 * FUNCTION : IODir entries for selectors

-*/
 
static const char* rcsID = "$Id: iodirentry.cc,v 1.3 2001-07-26 09:38:13 windev Exp $";

#include "iodirentry.h"
#include "transl.h"
#include "iodir.h"
#include "ioman.h"
#include "globexpr.h"

#include "errh.h"

IODirEntry::IODirEntry( IOObj* iob, int selres, bool maychgdir )
	: UserIDObject("")
	, ioobj(iob)
	, beingsorted(false)
{
    if ( !maychgdir )
	*name_ = " ";
    else
    {
	if ( !ioobj->isLink() )
	    *name_ = "  ";
	else if ( selres == 1 )
	    *name_ = "->"; // Link, must go there to get usable
	else
	    *name_ = "> "; // Link but also directly usable
    }
    if ( ioobj )
	*name_ += ioobj->name();
    else
	*name_ = "..";
}


const UserIDString& IODirEntry::name() const
{
    return beingsorted && ioobj ? ioobj->name() : *name_;
}


IODirEntryList::IODirEntryList( IODir* id, const Translator* tr, bool maycd,
				const char* f )
	: UserIDObjectSet<IODirEntry>(
		id && id->main() ? (const char*)id->main()->name() : "Objects" )
	, trsel(tr->getSelector())
	, maychgdir(maycd)
	, trfilt(f)
{
    fill( id );
}


IODirEntryList::~IODirEntryList()
{
    deepErase();
}


void IODirEntryList::fill( IODir* iodir )
{
    if ( !iodir ) { pErrMsg("Can't fill IODirEntryList. No iodir"); return; }
    deepErase();
    setName( iodir->main() ? (const char*)iodir->main()->name() : "Objects" );
    const UserIDObjectSet<IOObj>& ioobjs = iodir->getObjs();
    int curset = 0;
    if ( maychgdir && strcmp(iodir->dirName(),IOM().rootDir()) )
    {
        *this += new IODirEntry( 0, 0, false );
	curset++;
    }

    GlobExpr ge( trfilt );
    for ( int idx=0; idx<ioobjs.size(); idx++ )
    {
	IOObj* ioobj = ioobjs[idx];

	int selres = 2;
	if ( trsel )
	{
	    selres = trsel( ioobj->group() );
	    if ( !selres || (selres == 1 && !ioobj->isLink()) ) continue;
	}
	if ( *((const char*)trfilt) )
	{
	    if ( !ge.matches( ioobj->translator() ) )
		continue;
	}

        *this += new IODirEntry( ioobj, selres, maychgdir );
    }
    sort();
    if ( lastiokey == "" )
	{ if ( size() > curset ) setCurrent( curset ); }
    else
	setSelected( lastiokey );
}


void IODirEntryList::setSelected( const MultiID& iniokey )
{
    bool matches = false;
    for ( int idx=0; idx<size(); idx++ )
    {
	IODirEntry* entry = (*this)[idx];
	MultiID iokey( iniokey );
	if ( !entry->ioobj )
	{
	    if ( iokey == "" )
		matches = true;
	}
	else
	{
	    if ( iokey == entry->ioobj->key() )
		matches = true;
	    else
	    {
		while ( 1 )
		{
		    iokey = iokey.upLevel();
		    if ( iokey == "" ) break;
		    if ( iokey == entry->ioobj->key() )
			matches = true;
		}
	    }
	}
	if ( matches )
	{
	    setCurrent( entry );
	    lastiokey = iokey;
	    return;
	}
    }
}


void IODirEntryList::curRemoved()
{
    IODirEntry* cur = current();
    if ( !cur ) return;
    *this -= cur;
    delete cur;
    cur = current();
    lastiokey = cur && cur->ioobj ? (const char*)cur->ioobj->key() : "";
}


IOObj* IODirEntryList::selected()
{
    return current() ? current()->ioobj : 0 ;
}


bool IODirEntryList::mustChDir()
{
    if ( !current() ) return false;
    char ch1 = *(const char*)current()->name();
    return ch1 == '-' || ch1 == '.';
}


bool IODirEntryList::canChDir()
{
    if ( !current() ) return false;
    char ch1 = *(const char*)current()->name();
    return ch1 != ' ';
}


void IODirEntryList::sort()
{
    for ( int idx=0; idx<size(); idx++ )
	(*this)[idx]->beingsorted = true;
    ::sort((UserIDObjectSet<UserIDObject>*)this);
    for ( int idx=0; idx<size(); idx++ )
	(*this)[idx]->beingsorted = false;
}
