/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id: viscolortab.cc,v 1.12 2003-01-02 14:22:18 kristofer Exp $";

#include "viscolortab.h"
#include "visdataman.h"
#include "scaler.h"
#include "colortab.h"
#include "iopar.h"

mCreateFactoryEntry( visBase::VisColorTab );

const char* visBase::VisColorTab::colorseqidstr = "ColorSeq ID";
const char* visBase::VisColorTab::scalefactorstr = "Scale Factor";

visBase::VisColorTab::VisColorTab()
    : sequencechange( this )
    , rangechange( this )
    , colseq( 0 )
    , scale( *new LinScaler )
{
    setColorSeq( ColorSequence::create() );
}


visBase::VisColorTab::~VisColorTab()
{
    colseq->unRef();
}


Color  visBase::VisColorTab::color( float val ) const
{
    return colseq->colors().color( scale.scale( val ), false );
}


void visBase::VisColorTab::setNrSteps( int idx )
{
    return colseq->colors().calcList( idx );
}


int visBase::VisColorTab::nrSteps() const
{
    return colseq->colors().nrColors();
}


int visBase::VisColorTab::colIndex( float val ) const
{
    return colseq->colors().colorIdx( scale.scale( val ), nrSteps() );
}


Color visBase::VisColorTab::tableColor( int idx ) const
{
    return colseq->colors().tableColor(idx);
}


void visBase::VisColorTab::scaleTo( const Interval<float>& rg )
{
    float width = rg.width();
    if ( mIS_ZERO(width) )
	scaleTo( Interval<float>(rg.start -1, rg.start+1));
    else
    {
	scale.factor = 1.0/rg.width();
	if ( rg.start > rg.stop ) scale.factor *= -1;
	scale.constant = -rg.start*scale.factor;

	rangechange.trigger();
    }
}


Interval<float> visBase::VisColorTab::getInterval() const
{
    float start = -scale.constant / scale.factor;
    float stop = start + 1 / scale.factor;
    return Interval<float>(start,stop);
}


void visBase::VisColorTab::setColorSeq( ColorSequence* ns )
{
    if ( colseq )
    {
	colseq->change.remove( mCB( this, VisColorTab, colorseqchanged ));
	colseq->unRef();
    }

    colseq = ns;
    colseq->ref();
    colseq->change.notify( mCB( this, VisColorTab, colorseqchanged ));
    sequencechange.trigger();
}


void visBase::VisColorTab::colorseqchanged()
{
    sequencechange.trigger();
}


int visBase::VisColorTab::usePar( const IOPar& par )
{
    int res = DataObject::usePar( par );
    if ( res != 1 ) return res;

    int colseqid;
    if ( !par.get( colorseqidstr, colseqid ) )
	return -1;

    visBase::DataObject* dataobj = visBase::DM().getObj(colseqid);
    if ( !dataobj ) return 0;

    mDynamicCastGet(visBase::ColorSequence*,cs,dataobj);
    if ( !cs ) return -1;

    setColorSeq( cs );

    const char* scalestr = par.find( scalefactorstr );
    if ( !scalestr ) return -1;

    scale.fromString( scalestr );
    return 1;
}


void visBase::VisColorTab::fillPar( IOPar& par, TypeSet<int>& saveids ) const
{
    DataObject::fillPar( par, saveids );
    par.set( colorseqidstr, colseq->id() );
    if ( saveids.indexOf(colseq->id())==-1 ) saveids += colseq->id();
    par.set( scalefactorstr, scale.toString() );
}

