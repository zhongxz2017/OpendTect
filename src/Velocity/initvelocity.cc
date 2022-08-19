/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "moddepmgr.h"
#include "velocityfunctionvolume.h"
#include "velocityfunctionstored.h"


mDefModInitFn(Velocity)
{
    mIfNotFirstTime( return );

    Vel::VolumeFunctionSource::initClass();
    Vel::StoredFunctionSource::initClass();
}
