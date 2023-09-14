/*+
________________________________________________________________________

 Copyright:	(C) 1995-2023 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "ioman.h"
#include "filepath.h"
#include "moddepmgr.h"
#include "multiid.h"
#include "oddirs.h"
#include "survinfo.h"
#include "testprog.h"
#include "unitofmeasure.h"

BufferString basedatadir_;
const UnitOfMeasure* timeuom_ = nullptr;
const UnitOfMeasure* timemsuom_ = nullptr;
const UnitOfMeasure* meteruom_ = nullptr;
const UnitOfMeasure* feetuom_ = nullptr;
const UnitOfMeasure* metersecuom_ = nullptr;
const UnitOfMeasure* ftsecuom_ = nullptr;

static BufferStringSet survNames()
{
    static BufferStringSet survnames;
    if ( survnames.isEmpty() )
    {
	survnames.add( "F3_Test_Survey" )
	 .add( "F3_Test_Survey_DisplayFT" ).add( "F3_Test_Survey_XYinft" )
	 .add( "F3_Test_Survey_DepthM" ).add( "F3_Test_Survey_DepthM_XYinft" )
	 .add( "F3_Test_Survey_DepthFT").add("F3_Test_Survey_DepthFT__XYinft_");
    }

    return survnames;
}

static void assignUnitsFromRepository()
{
    timeuom_ = UoMR().get( "Seconds" );
    timemsuom_ = UoMR().get( "Milliseconds" );
    meteruom_ = UoMR().get( "Meter" );
    feetuom_ = UoMR().get( "Feet" );
    metersecuom_ = UoMR().get( "m/s" );
    ftsecuom_ = UoMR().get( "ft/s" );
}

static BoolTypeSet& zIsTime()
{
    static BoolTypeSet zistime;
    if ( zistime.isEmpty() )
	{ zistime.add( true ).add( true ).add( true )
		 .add( false ).add( false ).add( false ).add( false ); }
    return zistime;
}

static BoolTypeSet& zInMeter()
{
    static BoolTypeSet zinmeter;
    if ( zinmeter.isEmpty() )
	{ zinmeter.add( false ).add( false ).add( false )
		  .add( true ).add( true ).add( false ).add( false ); }
    return zinmeter;
}

static BoolTypeSet& zInFeet()
{
    static BoolTypeSet zinfeet;
    if ( zinfeet.isEmpty() )
	{ zinfeet.add( false ).add( false ).add( false )
		 .add( false ).add( false ).add( true ).add( true ); }
    return zinfeet;
}

static BoolTypeSet& xyInFeet()
{
    static BoolTypeSet xyinfeet;
    if ( xyinfeet.isEmpty() )
	{ xyinfeet.add( false ).add( false ).add( true )
		  .add( false ).add( true ).add( false ).add( true ); }
    return xyinfeet;
}

static BoolTypeSet& depthsInFeet()
{
    static BoolTypeSet depthsinfeet;
    if ( depthsinfeet.isEmpty() )
	{ depthsinfeet.add( false ).add( true ).add( true )
		      .add( false ).add( false ).add( true ).add( true ); }
    return depthsinfeet;
}

static ObjectSet<const UnitOfMeasure>& zUnits()
{	//Expected unit for UnitOfMeasure::surveyDefZUnit
    static ObjectSet<const UnitOfMeasure> ret;
    if ( ret.isEmpty() )
	{ ret.add( timemsuom_ ).add( timemsuom_ ).add( timemsuom_ )
	     .add( meteruom_ ).add( meteruom_ )
	     .add( feetuom_ ).add( feetuom_ ); }
    return ret;
}

static ObjectSet<const UnitOfMeasure>& zStorageUnits()
{	//Expected unit for UnitOfMeasure::surveyDefZStorageUnit
    static ObjectSet<const UnitOfMeasure> ret;
    if ( ret.isEmpty() )
	{ ret.add( timeuom_ ).add( timeuom_ ).add( timeuom_ )
	     .add( meteruom_ ).add( meteruom_ )
	     .add( feetuom_ ).add( feetuom_ ); }
    return ret;
}

static ObjectSet<const UnitOfMeasure>& depthUnits()
{	//Expected unit for UnitOfMeasure::surveyDefDepthUnit
    static ObjectSet<const UnitOfMeasure> ret;
    if ( ret.isEmpty() )
	{ ret.add( meteruom_ ).add( feetuom_ ).add( feetuom_ )
	     .add( meteruom_ ).add( meteruom_ )
	     .add( feetuom_ ).add( feetuom_ ); }
    return ret;
}

static ObjectSet<const UnitOfMeasure>& depthStorageUnits()
{	//Expected unit for UnitOfMeasure::surveyDefDepthStorageUnit
    static ObjectSet<const UnitOfMeasure> ret;
    if ( ret.isEmpty() )
	{ ret.add( meteruom_ ).add( meteruom_ ).add( meteruom_ )
	     .add( meteruom_ ).add( meteruom_ )
	     .add( feetuom_ ).add( feetuom_ ); }
    return ret;
}

static ObjectSet<const UnitOfMeasure>& velUnits()
{	//Expected unit for UnitOfMeasure::surveyDefVelUnit
    static ObjectSet<const UnitOfMeasure> ret;
    if ( ret.isEmpty() )
	{ ret.add( metersecuom_ ).add( ftsecuom_ ).add( ftsecuom_ )
	     .add( metersecuom_ ).add( metersecuom_ )
	     .add( ftsecuom_ ).add( ftsecuom_ ); }
    return ret;
}


#define mMsg(txt) BufferString( SI().name(), ": ", txt )

static bool testSurveyLocation( const SurveyDiskLocation& sdl )
{
    mRunStandardTest( StringView(GetBaseDataDir()) == basedatadir_ &&
		      sdl.basePath() == basedatadir_,
		      mMsg("Base data dir") );
    mRunStandardTest( StringView(GetSurveyName()) == sdl.dirName(),
		      mMsg("Survey name") )

    return true;
}


static bool testSurveyDefinitions( int isurv )
{
    mRunStandardTestWithError( SI().getDirName() == survNames()[isurv]->str(),
			       mMsg("Name"), SI().getDirName() );
    mRunStandardTest( SI().zIsTime() == zIsTime()[isurv] &&
		      SI().zInMeter() == zInMeter()[isurv] &&
		      SI().zInFeet() == zInFeet()[isurv],
		      mMsg("zDomain") );
    mRunStandardTest( SI().xyInFeet() == xyInFeet()[isurv],
		      mMsg("XY units") );
    mRunStandardTest( SI().depthsInFeet() == depthsInFeet()[isurv],
		      mMsg("Depths unit") );

    return true;
}


static bool testSurveyUnits( int isurv )
{
    const UnitOfMeasure* zuom = UnitOfMeasure::surveyDefZUnit();
    const UnitOfMeasure* zstoruom = UnitOfMeasure::surveyDefZStorageUnit();
    const UnitOfMeasure* twtuom = UnitOfMeasure::surveyDefTimeUnit();
    const UnitOfMeasure* twtstoruom = UnitOfMeasure::surveyDefTimeStorageUnit();
    const UnitOfMeasure* depthuom = UnitOfMeasure::surveyDefDepthUnit();
    const UnitOfMeasure* depthstoruom =
				    UnitOfMeasure::surveyDefDepthStorageUnit();
    const UnitOfMeasure* veluom = UnitOfMeasure::surveyDefVelUnit();

    mRunStandardTestWithError( zuom == zUnits()[isurv],
		   mMsg("Vertical unit in displays (ms,m,ft)"),
		   BufferString( "Should not be in: ", zuom->name() ) );
    mRunStandardTestWithError( zstoruom == zStorageUnits()[isurv],
		   mMsg("Vertical unit in storage (s,m,ft)"),
		   BufferString( "Should not be in: ", zstoruom->name() ) );
    mRunStandardTestWithError( twtuom == timemsuom_,
		   mMsg("Time unit in displays (ms)"),
		   BufferString( "Should not be in: ", twtuom->name() ) );
    mRunStandardTestWithError( twtstoruom == timeuom_,
		   mMsg("Time unit in storage (s)"),
		   BufferString( "Should not be in: ", twtstoruom->name() ) );
    mRunStandardTestWithError( depthuom == depthUnits()[isurv],
		   mMsg("Depth unit in displays (m,ft)"),
		   BufferString( "Should not be in: ", depthuom->name() ) );
    mRunStandardTestWithError( depthstoruom == depthStorageUnits()[isurv],
		   mMsg("Depth unit in storage (m,ft)"),
		   BufferString( "Should not be in: ", depthstoruom->name() ) );
    mRunStandardTestWithError( veluom == velUnits()[isurv],
		      mMsg("Velocity unit in displays (m/s,ft/s)"),
		      BufferString("Should not be in ", veluom->name()) );

    return true;
}


int mTestMainFnName( int argc, char** argv )
{
    mInitTestProgDR();

    OD::ModDeps().ensureLoaded("General");

    clParser().setKeyHasValue( "datadir" );
    clParser().getVal( "datadir", basedatadir_ );
    const uiRetVal uirv = IOMan::setDataSource( basedatadir_.buf(),
						survNames().first()->str() );
    mRunStandardTestWithError( uirv.isOK(), "Initialize the first project",
			       toString(uirv) );

    assignUnitsFromRepository();
    mRunStandardTest( timeuom_ && timemsuom_ && meteruom_ && feetuom_ &&
		      metersecuom_ && ftsecuom_,
		      "Units of measure from repository" );

    SurveyDiskLocation sdl( IOM().rootDir() );
    for ( int isurv=0; isurv<survNames().size(); isurv++ )
    {
	sdl.setDirName( survNames().get(isurv) );
	SurveyChanger changer( sdl );
	if ( !testSurveyLocation(sdl) ||
	     !testSurveyDefinitions(isurv) ||
	     !testSurveyUnits(isurv) )
	    return 1;
    }

    return 0;
}