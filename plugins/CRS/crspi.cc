/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "odplugin.h"

#include "crssystem.h"
#include "file.h"
#include "filepath.h"
#include "genc.h"
#include "legal.h"
#include "od_istream.h"
#include "oddirs.h"
#include "survinfo.h"

mDefODPluginEarlyLoad(CRS)
mDefODPluginInfo(CRS)
{
    static PluginInfo retpi(
	"Coordinate Reference System (base)",
	"Coordinate Reference System - base" );
    return &retpi;
}


static mUnusedVar uiString* legalText()
{
    uiString* ret = new uiString;
    FilePath fp( mGetSetupFileName("CRS"), "COPYING" );
    if ( File::exists(fp.fullPath()) )
    {
	BufferString legaltxt;
	od_istream strm( fp.fullPath() );
	if ( strm.getAll(legaltxt) )
	    *ret = toUiString( legaltxt );
    }

    return ret;
}

namespace Coords
{ extern "C" { mGlobal(Basic) void SetWGS84(const char*,CoordSystem*); } }

bool initCRSPlugin( bool withdatabase )
{
#ifdef OD_NO_PROJ
    return false
#else
    Coords::initCRSDatabase();
    Coords::ProjectionBasedSystem::initClass();
    if ( withdatabase )
	SI().readSavedCoordSystem();

    SetWGS84( Coords::Projection::sWGS84ProjDispString(),
	      Coords::ProjectionBasedSystem::getWGS84LLSystem() );
    return true;
#endif
}


mDefODInitPlugin(CRS)
{
    if ( !NeedDataBase() )

	return nullptr;
#ifndef OD_NO_PROJ
    legalInformation().addCreator( legalText, "PROJ" );
#endif
    initCRSPlugin( true );

    return nullptr;
}
