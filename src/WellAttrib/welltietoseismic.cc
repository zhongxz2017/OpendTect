/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bruno
 Date:		Jan 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: welltietoseismic.cc,v 1.48 2010-04-27 08:21:09 cvsbruno Exp $";

#include "welltietoseismic.h"

#include "arrayndimpl.h"
#include "arrayndutils.h"
#include "cubesampling.h"
#include "ioman.h"
#include "linear.h"
#include "mousecursor.h"
#include "survinfo.h"
#include "task.h"
#include "wavelet.h"

#include "welldata.h"
#include "welllog.h"
#include "welllogset.h"
#include "wellman.h"
#include "welld2tmodel.h"
#include "welltrack.h"

#include "welltiedata.h"
#include "welltieextractdata.h"
#include "welltiegeocalculator.h"
#include "welltiesetup.h"
#include "welltieunitfactors.h"


namespace WellTie
{

DataPlayer::DataPlayer( WellTie::DataHolder& dh, TaskRunner* tr ) 
    	: wtsetup_(dh.setup())
	, dholder_(dh)  
	, params_(*dh.dpms())		 
	, logset_(*dh.logset())	   
	, tr_(tr)		  
      	, d2tmgr_(dh.d2TMGR())
{
    geocalc_ = new WellTie::GeoCalculator(dh);
} 


DataPlayer::~DataPlayer()
{
    delete geocalc_;
    delete tr_;
}


Well::Data* DataPlayer::wd()
{
    return dholder_.wd();
}


bool DataPlayer::computeAll()
{
    dholder_.resetLogData();

    if ( !resampleLogs() ) 	   return false;
    if ( !computeReflectivity() )  return false;
    if ( !extractSeismics() ) 	   return false;
    if ( !computeWvltPack() ) 	   return false;

    return true;	
}


bool DataPlayer::computeWvltPack()
{
    if ( !logset_.size() ) 	   return false;
    if ( !convolveWavelet() ) 	   return false;
    if ( !estimateWavelet() )	   return false;
    if ( !computeCrossCorrel() )   return false;

    return true;
}


#define mSetData(lognm,dahlognm,arr)\
    dholder_.setLogVal( lognm, &arr, dholder_.getLogVal( dahlognm, true ) );
#define mReturnExec(yn) { MouseCursorManager::restoreOverride(); return yn; }
bool DataPlayer::extractSeismics()
{
    MouseCursorManager::setOverride( MouseCursor::Wait );
    
    WellTie::TrackExtractor wtextr( wd() );
    wtextr.timeintv_ = params_.timeintvs_[1];
    if ( !tr_->execute( wtextr ) ) 
	mReturnExec(false)
   
    const IOObj& ioobj = *IOM().get( wtsetup_.seisid_ );
    IOObj* seisobj = ioobj.clone();

    WellTie::SeismicExtractor seisextr( *seisobj );
    TypeSet<BinID> bids;
    for ( int idx=0; idx<wtextr.timeintv_.nrSteps(); idx++ )
	bids += wtextr.getBIDs()[idx];
    seisextr.setTimeIntv( params_.timeintvs_[1] ); 
    seisextr.setLineKey( &wtsetup_.linekey_ );
    seisextr.setBIDValues( bids );
    if ( !tr_->execute( seisextr ) ) 
	mReturnExec(false)
    
    const int sz =  params_.timeintvs_[1].nrSteps();
    Array1DImpl<float> tmpseis ( sz );
    memcpy( tmpseis.getData(), seisextr.vals_->getData(), sz*sizeof(float) );
    mSetData( params_.seisnm_, params_.refnm_, tmpseis );
    
    
    mReturnExec(true);
}


bool DataPlayer::resampleLogs()
{
    MouseCursorManager::setOverride( MouseCursor::Wait );

    BufferStringSet lognms; 
    lognms.add( wtsetup_.corrvellognm_ );
    lognms.add( wtsetup_.vellognm_ );
    lognms.add( wtsetup_.denlognm_ );

    resLogExecutor( lognms, true, params_.timeintvs_[0] );

    MouseCursorManager::restoreOverride();

    return true;
}


bool DataPlayer::resLogExecutor( const BufferStringSet& lognms, bool fromwd, 
				 const StepInterval<float>& intv )
{
    bool wasdone = false;
    for ( int idx=0; idx<lognms.size(); idx++ )
    {
	Well::Log* newlog = logset_.getLog( lognms.get(idx) );
	const Well::Log* log = fromwd ? wd()->logs().getLog(lognms.get(idx)) 
				      : newlog;
	if ( !newlog || !log )
	    continue;
	
	const Well::Log* orglog = new Well::Log( *log );
	WellTie::LogResampler logres( newlog, *orglog, wd(), &dholder_ );
	logres.setTimeIntv( intv ); logres.isavg_ = fromwd;
	wasdone = tr_->execute( logres );
	delete orglog;
    }
    return wasdone;
}


bool DataPlayer::computeReflectivity()
{ 
    BufferStringSet lognms; lognms.add( params_.currvellognm_ );
    lognms.add( params_.denlognm_ ); lognms.add( params_.ainm_ );

    Array1DImpl<float> ai( params_.timeintvs_[0].nrSteps() );
    Array1DImpl<float> ref( params_.timeintvs_[1].nrSteps() );
    geocalc_->computeAI( *dholder_.arr( lognms.get(0) ), 
			 *dholder_.arr( lognms.get(1) ), ai ); 

    geocalc_->lowPassFilter( ai, 1/( 3*SI().zStep() ) );
    geocalc_->computeReflectivity( ai, ref, params_.step_ );

    mSetData( params_.ainm_, lognms.get(1), ai );
    resLogExecutor( lognms, false, params_.timeintvs_[1] );
    mSetData( params_.refnm_, lognms.get(1), ref );

    return true;
}


bool DataPlayer::convolveWavelet()
{
    StepInterval<float> si = params_.timeintvs_[1];
    si.start -= params_.timeintvs_[1].step/2;
    si.stop  -= params_.timeintvs_[1].step/2;
    
    WellTie::LogResampler synres( 0, *logset_.getLog(params_.refnm_), wd() );
    synres.setTimeIntv( si ); 
    synres.isavg_ = false;
    synres.execute(); 

    bool isinitwvltactive = params_.isinitwvltactive_;
    Wavelet* wvlt = isinitwvltactive ? dholder_.wvltset()[0] 
				     : dholder_.wvltset()[1]; 
    if ( !wvlt ) return false;

    const int wvltsz = wvlt->size();
    if ( wvltsz <= 0 || wvltsz > params_.timeintvs_[1].nrSteps() ) 
	return false;

    Array1DImpl<float> wvltvals( wvlt->size() );
    memcpy( wvltvals.getData(), wvlt->samples(), wvltsz*sizeof(float) );

    const int wvltidx = wvlt->centerSample();

    Array1DImpl<float> tmpsynth ( params_.timeintvs_[1].nrSteps() );

    geocalc_->convolveWavelet( wvltvals, *synres.vals_, 
				tmpsynth, wvltidx );
    mSetData( params_.synthnm_, params_.refnm_, tmpsynth );

    return true;
}


bool DataPlayer::estimateWavelet()
{
    if ( !params_.isinitwvltactive_ ) return true;
    const StepInterval<float> si = params_.timeintvs_[2];
    const int datasz = si.nrSteps(); 

    WellTie::LogResampler refres( 0, *logset_.getLog(params_.refnm_), wd() );
    refres.setTimeIntv( si ); refres.isavg_ = false;
    refres.execute(); 
    
    WellTie::LogResampler seisres( 0, *logset_.getLog(params_.seisnm_), wd() );
    seisres.setTimeIntv( si ); seisres.isavg_ = false;
    seisres.execute(); 

    Wavelet* wvlt = dholder_.wvltset()[1]; 
    if ( !wvlt ) return false;

    wvlt->setName( "Estimated Wavelet" );
    int wvltsz = params_.estwvltlength_ ? params_.estwvltlength_ 
					: dholder_.wvltset()[0]->size();
    if ( datasz < wvltsz +1 )
       return false;

    wvltsz += wvltsz%2 ? 0 : 1;
    wvlt->reSize( wvltsz );
   
    Array1DImpl<float> wvltarr( datasz ), wvltvals( wvltsz );
    geocalc_->deconvolve( *seisres.vals_, *refres.vals_, wvltarr, wvltsz );

    for ( int idx=0; idx<wvltsz; idx++ )
	wvlt->samples()[idx] = wvltarr.get( datasz/2 + idx - wvltsz/2 );
    
    memcpy( wvltvals.getData(), wvlt->samples(), wvltsz*sizeof(float) );
    ArrayNDWindow window( Array1DInfoImpl(wvltsz), false, "CosTaper", .05 );
    window.apply( &wvltvals );
    memcpy( wvlt->samples(), wvltvals.getData(), wvltsz*sizeof(float) );
  
    return true;
}


bool DataPlayer::computeCrossCorrel()
{
    const StepInterval<float> si = params_.timeintvs_[2];
    const int sz = si.nrSteps();
    
    WellTie::LogResampler synres( logset_.getLog(params_.crosscorrnm_), 
	    			  *logset_.getLog(params_.synthnm_), wd() );
    synres.setTimeIntv( si ); synres.isavg_ = false;
    synres.execute(); 
    
    WellTie::LogResampler seisres( 0, *logset_.getLog(params_.seisnm_), wd() );
    seisres.setTimeIntv( si ); seisres.isavg_ = false;
    seisres.execute();

    Array1DImpl<float> tmpcrosscorr( sz );

    geocalc_->crosscorr( *synres.vals_, *seisres.vals_, tmpcrosscorr );

    dholder_.setLogVal( params_.crosscorrnm_, &tmpcrosscorr, synres.dahs_ );
    //computes cross-correl coeff
    LinStats2D ls2d;
    ls2d.use( synres.vals_->getData(), seisres.vals_->getData(), sz );

    dholder_.corrcoeff() = ls2d.corrcoeff;

    return true;
}

}; //namespace WellTie
