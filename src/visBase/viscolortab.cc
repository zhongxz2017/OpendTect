/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          Mar 2002
 RCS:           $Id: viscolortab.cc,v 1.42 2008-04-08 09:23:43 cvsnanne Exp $
________________________________________________________________________

-*/

#include "viscolortab.h"

#include "coltabsequence.h"
#include "coltabmapper.h"
#include "coltabindex.h"
#include "dataclipper.h"
#include "iopar.h"
#include "scaler.h"
#include "visdataman.h"
#include "valseries.h"
#include "math2.h"

mCreateFactoryEntry( visBase::VisColorTab );

namespace visBase
{

const char* VisColorTab::sKeyColorSeqID()	{ return "ColorSeq ID"; }
const char* VisColorTab::sKeyScaleFactor()	{ return "Scale Factor"; }
const char* VisColorTab::sKeyClipRate()		{ return "Cliprate"; }
const char* VisColorTab::sKeyAutoScale()	{ return "Auto scale"; }
const char* VisColorTab::sKeySymmetry()		{ return "Symmetry"; }
const char* VisColorTab::sKeySymMidval()	{ return "Symmetry Midvalue"; }


VisColorTab::VisColorTab()
    : sequencechange( this )
    , rangechange( this )
    , autoscalechange( this )
    , viscolseq_( 0 )
    , indextable_( 0 )
    , ctmapper_(new ColTab::Mapper())
    , autoscale_( true )
{
    setColorSeq( ColorSequence::create() );
}


VisColorTab::~VisColorTab()
{
    if ( viscolseq_ )
    {
	viscolseq_->change.remove( mCB(this,VisColorTab,colorseqchanged) );
	viscolseq_->unRef();
    }

    delete indextable_;
    delete ctmapper_;
}


bool VisColorTab::autoScale() const
{ return autoscale_; }


void VisColorTab::setAutoScale( bool yn )
{
    if ( yn==autoscale_ ) return;

    autoscale_ = yn;
    if ( autoscale_ ) autoscalechange.trigger();
}


void VisColorTab::setSymMidval( float symmidval )
{
    if (  mIsEqual(symmidval,ctmapper_->symmidval_,mDefEps) ) return;

    ctmapper_->symmidval_ = symmidval;
    ctmapper_->update( true );
}


float VisColorTab::symMidval() const
{ return ctmapper_->symmidval_; }


float VisColorTab::clipRate() const
{ return ctmapper_->cliprate_; }


void VisColorTab::setClipRate( float ncr )
{
    if ( mIsEqual(ncr,ctmapper_->cliprate_,mDefEps) ) return;

    ctmapper_->cliprate_ = ncr;
    ctmapper_->update( false );
}


void VisColorTab::scaleTo( const float* values, int nrvalues )
{
    float* valuesnc = const_cast<float*>(values);
    const ArrayValueSeries<float,float>* arrvs =
	new ArrayValueSeries<float,float>( valuesnc, false, nrvalues );
    scaleTo( arrvs, nrvalues );
}


void VisColorTab::scaleTo( const ValueSeries<float>* values, int nrvalues )
{
    ctmapper_->setData( values, nrvalues );
    rangechange.trigger();
}


Color VisColorTab::color( float val ) const
{
    return indextable_->color( val );
}


void VisColorTab::setNrSteps( int idx )
{
    if ( !indextable_ )
	indextable_ = new ColTab::IndexedLookUpTable( viscolseq_->colors(),
	    					  idx, ctmapper_ );
    else
    {
	indextable_->setNrCols( idx );
	indextable_->update();
    }
}


int VisColorTab::nrSteps() const
{
    return indextable_ ? indextable_->nrCols() : 0;
}


int VisColorTab::colIndex( float val ) const
{
    if ( !Math::IsNormalNumber(val) || mIsUdf(val) )
	return nrSteps();
    return indextable_->indexForValue( val );
}


Color VisColorTab::tableColor( int idx ) const
{
    return idx==nrSteps()
    	? viscolseq_->colors().undefColor()
	: indextable_->colorForIndex( idx );  
}


void VisColorTab::scaleTo( const Interval<float>& rg )
{
    ctmapper_->setRange( rg );
    rangechange.trigger();
}


Interval<float> VisColorTab::getInterval() const
{ return ctmapper_->range(); }


void VisColorTab::setColorSeq( ColorSequence* ns )
{
    if ( viscolseq_ )
    {
	viscolseq_->change.remove( mCB(this,VisColorTab,colorseqchanged) );
	viscolseq_->unRef();
    }

    viscolseq_ = ns;
    viscolseq_->ref();
    viscolseq_->change.notify( mCB(this,VisColorTab,colorseqchanged) );
    sequencechange.trigger();
}


const ColorSequence& VisColorTab::colorSeq() const
{ return *viscolseq_; }


ColorSequence& VisColorTab::colorSeq()
{ return *viscolseq_; }


void VisColorTab::colorseqchanged()
{
    const int nrcols = nrSteps();
    delete indextable_;
    indextable_ = new ColTab::IndexedLookUpTable( viscolseq_->colors(),
	    					  nrcols, ctmapper_ );
    sequencechange.trigger();
}


void VisColorTab::triggerRangeChange()
{ rangechange.trigger(); }


void VisColorTab::triggerSeqChange()
{ sequencechange.trigger(); }


void VisColorTab::triggerAutoScaleChange()
{ autoscalechange.trigger(); }


int VisColorTab::usePar( const IOPar& par )
{
    int res = DataObject::usePar( par );
    if ( res != 1 ) return res;

    int colseqid;
    if ( !par.get( sKeyColorSeqID(), colseqid ) )
	return -1;

    DataObject* dataobj = DM().getObject(colseqid);
    if ( !dataobj ) return 0;

    mDynamicCastGet(ColorSequence*,cs,dataobj);
    if ( !cs ) return -1;

    setColorSeq( cs );

    float cliprate = ColTab::defClipRate();
    par.get( sKeyClipRate(), cliprate );
    setClipRate( cliprate );

    bool autoscale = true;
    par.getYN( sKeyAutoScale(), autoscale );
    setAutoScale( autoscale );

    bool symmetry = false;
    par.getYN( sKeySymmetry(), symmetry );

    float symmidval = mUdf(float);
    par.get( sKeySymMidval(), symmidval );
    setSymMidval( symmetry ? 0 : symmidval );

    /*
    TODO: re-implement
    const char* scalestr = par.find( sKeyScaleFactor() );
    if ( !scalestr ) return -1;
    scale_.fromString( scalestr );
    */

    return 1;
}


void VisColorTab::fillPar( IOPar& par, TypeSet<int>& saveids ) const
{
    DataObject::fillPar( par, saveids );
    par.set( sKeyColorSeqID(), viscolseq_->id() );
    if ( saveids.indexOf(viscolseq_->id())==-1 ) saveids += viscolseq_->id();
//    par.set( sKeyScaleFactor(), scale_.toString() );
    par.set( sKeyClipRate(), ctmapper_->cliprate_ );
    par.setYN( sKeyAutoScale(), autoscale_ );
    par.set( sKeySymMidval(), ctmapper_->symmidval_ );
}

}; // namespace visBase
