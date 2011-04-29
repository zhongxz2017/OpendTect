/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : May 1999
-*/


#include "raytrace1d.h"

#include "arrayndimpl.h"
#include "iopar.h"
#include "sorting.h"
#include "velocitycalc.h"
#include "zoeppritzcoeff.h"


bool RayTracer1D::Setup::usePar( const IOPar& par )
{
    return par.getYN( sKeyPWave(), pdown_, pup_ ) &&
	   par.get( sKeySRDepth(), sourcedepth_, receiverdepth_ ) &&
	   par.get( sKeyPSVelFac(), pvel2svelafac_, pvel2svelbfac_ );
}


void RayTracer1D::Setup::fillPar( IOPar& par ) const 
{
    par.setYN( sKeyPWave(), pdown_, pup_ );
    par.set( sKeySRDepth(), sourcedepth_, receiverdepth_ );
    par.set( sKeyPSVelFac(), pvel2svelafac_, pvel2svelbfac_ );
}


RayTracer1D::RayTracer1D( const RayTracer1D::Setup& s )
    : setup_( s )
    , receiverlayer_( 0 )
    , sourcelayer_( 0 )
    , sini_( 0 )
    , twt_(0)
    , reflectivity_( 0 )
{}					       	


RayTracer1D::~RayTracer1D()
{ delete sini_; delete twt_; delete reflectivity_; }


void RayTracer1D::setSetup( const RayTracer1D::Setup& s )
{ setup_ = s; }


void RayTracer1D::setModel( bool pwave, const AIModel& lys )
{
    if ( pwave )
	pmodel_ = lys;
    else 
	smodel_ = lys;
}


void RayTracer1D::setOffsets( const TypeSet<float>& offsets )
{ offsets_ = offsets; }


od_int64 RayTracer1D::nrIterations() const
{ return (pmodel_.size() ? pmodel_.size() : smodel_.size() )-1; }


int RayTracer1D::findLayer( const AIModel& model, float targetdepth ) const
{
    int res; float depth = setup_.sourcedepth_;
    for ( res=0; res<model.size(); res++ )
    {
	if ( targetdepth <= depth  )
	    break;
	depth += model[res].thickness_;
    }

    return res ? res-1 : 0;
}


float RayTracer1D::getLayerDepth( const AIModel& model, int layer ) const
{
    float depth = setup_.sourcedepth_;
    for ( int idx=0; idx<layer+1; idx++)
    {
	depth += model[ idx ].thickness_;
    }
    return depth;
}



bool RayTracer1D::doPrepare( int nrthreads )
{
    //See if we can find zero-offset
    bool found = false;
    for ( int idx=0; idx<offsets_.size(); idx++ )
    {
	if ( mIsZero(offsets_[idx],1e-3) )
	{
	    found = true;
	    break;
	}
    }
    if ( !found )
	offsets_ += 0;

    const int psz = pmodel_.size();
    const int ssz = smodel_.size();
    const float p2safac = setup_.pvel2svelafac_;
    const float p2sbfac = setup_.pvel2svelbfac_;

    if ( !ssz )
    {
	for ( int idx=0; idx<psz; idx++ )
	{
	    const AILayer& pl = pmodel_[idx];
	    const float vs = sqrt( p2safac*pl.vel_*pl.vel_ + p2sbfac );
	    smodel_ += AILayer( pl.thickness_, vs, pl.den_ );
	}
    }
    else if ( !psz )
    {
	for ( int idx=0; idx<ssz; idx++ )
	{
	    const AILayer& sl = smodel_[idx];
	    const float vp = sqrt( (sl.vel_*sl.vel_ - p2sbfac)/p2safac );
	    pmodel_ += AILayer( sl.thickness_, vp, sl.den_ );
	}
    }

    if ( psz && ssz && psz!=ssz )
    {
	errmsg_ = "P and S model sizes must be identical";
	return false;
    }

    const AIModel& depthmodel = psz ? pmodel_ : smodel_;

    sourcelayer_ = findLayer( depthmodel, setup_.sourcedepth_ );
    receiverlayer_ = findLayer( depthmodel, setup_.receiverdepth_ );

    const int layersize = nrIterations();
    if ( sourcelayer_>=layersize || receiverlayer_>=layersize )
    {
	errmsg_ = "Source or receiver is below lowest layer";
	return false;
    }

    const int offsetsz = offsets_.size();
    TypeSet<int> offsetidx( offsetsz, 0 );
    for ( int idx=0; idx<offsetsz; idx++ )
	offsetidx[idx] = idx;

    sort_coupled( offsets_.arr(), offsetidx.arr(), offsetsz );
    offsetpermutation_.erase();
    for ( int idx=0; idx<offsetsz; idx++ )
	offsetpermutation_ += offsetidx.indexOf( idx );

    firstlayer_ = mMIN( receiverlayer_, sourcelayer_ );

    if ( !sini_ )
	sini_ = new Array2DImpl<float>( layersize, offsetsz );
    else
	sini_->setSize( layersize, offsetsz );
    sini_->setAll( 0 );

    if ( !twt_ )
	twt_ = new Array2DImpl<float>( layersize, offsetsz );
    else
	twt_->setSize( layersize, offsetsz );
    twt_->setAll( mUdf(float) );

    if ( !reflectivity_ )
	reflectivity_ = new Array2DImpl<float_complex>( layersize, offsetsz );
    else
	reflectivity_->setSize( layersize, offsetsz );

    reflectivity_->setAll( mUdf( float_complex ) );

    return true;
}


bool RayTracer1D::doWork( od_int64 start, od_int64 stop, int nrthreads )
{
    const int offsz = offsets_.size();
    const AIModel& model = setup_.pdown_ ? pmodel_ : smodel_;

    for ( int layer=start; layer<=stop; layer++ )
    {
	const AILayer& ailayer = model[layer];
	const float depth = getLayerDepth(model,layer);
	for ( int osidx=0; osidx<offsz; osidx++ )
	{
	    const float offset = offsets_[osidx];
	    const float angle = depth ? atan( offset / depth ) : 0;
	    const float rayparam = sin(angle) / ailayer.vel_;

	    if ( !compute( layer+1, osidx, rayparam ) )
	    { 
		BufferString msg( "Can not compute reflectivity " );
		msg += toString( layer+1 ); 
		msg += "\n most probably the velocity is negative or null";
		errmsg_ = msg; return false; 
	    }
	}
    }
    return true;
}



#define mRetFalseIfZero(vel) \
    if ( mIsZero(vel,mDefEps) ) return false;

#define mAddInterTime(mod) \
{ \
    vel = mod[lidx].vel_; \
    mRetFalseIfZero(vel) ; \
    depth = getLayerDepth( mod, lidx ); \
    angle = depth ? atan( offsets_[offsetidx] / depth ) : 0; \
    dist = mod[lidx].thickness_ / cos( angle ); \
    twt += dist / vel; \
}

bool RayTracer1D::compute( int layer, int offsetidx, float rayparam )
{
    const AIModel& dlayers = setup_.pdown_ ? pmodel_ : smodel_;
    const AIModel& ulayers = setup_.pup_ ? pmodel_ : smodel_;

    const float sini = dlayers[layer-1].vel_ * rayparam;
    sini_->set( layer-1, offsetidx, sini );

    mAllocVarLenArr( ZoeppritzCoeff, coefs, layer );

    for ( int idx=firstlayer_; idx<layer; idx++ )
	coefs[idx].setInterface( rayparam, pmodel_[idx], pmodel_[idx+1],
				smodel_[idx], smodel_[idx+1] );
    int lidx = sourcelayer_;
    float_complex reflectivity =
	coefs[lidx].getCoeff( true, lidx!=layer-1, setup_.pdown_,
			    lidx==layer-1? setup_.pup_ : setup_.pdown_ ); 

    float vel, dist, twt, angle, depth; twt = 0;
    mAddInterTime( dlayers );

    lidx++;

    while ( lidx < layer )
    {
	reflectivity *= coefs[lidx].getCoeff( true, lidx!=layer-1,
		setup_.pdown_, lidx==layer-1? setup_.pup_ : setup_.pdown_ );
	mAddInterTime( dlayers );
	lidx++;
    }

    for ( lidx=layer-1; lidx>=receiverlayer_; lidx--)
    {
	if ( lidx>receiverlayer_  )
	    reflectivity *=
		coefs[lidx-1].getCoeff(false,false,setup_.pup_,setup_.pup_);
	mAddInterTime( ulayers );
    }

    /*
    reflectivity = getFastCoeff( rayparam, 
				    pmodel_[layer-1], pmodel_[layer], 
				    smodel_[layer-1], smodel_[layer] );*/ 

    twt_->set( layer-1, offsetidx, twt );
    reflectivity_->set( layer-1, offsetidx, reflectivity );

    return true;
}


float RayTracer1D::getSinAngle( int layer, int offset ) const
{
    const int offsetidx = offsetpermutation_[offset];

    if ( !sini_ || layer<0 || layer>=sini_->info().getSize(0) || 
	 offsetidx<0 || offsetidx>=sini_->info().getSize(1) )
	return mUdf(float);
    
    return sini_->get( layer, offsetidx );
}



bool RayTracer1D::getReflectivity( int offset, ReflectivityModel& model ) const
{
    const int offsetidx = offsetpermutation_[offset];
    if ( offsetidx<0 || offsetidx>=reflectivity_->info().getSize(1) )
	return false;

    const int nrlayers = reflectivity_->info().getSize(0);

    model.erase();
    model.setCapacity( nrlayers );
    ReflectivitySpike spike;

    const AIModel& layers = pmodel_.size() ? pmodel_ : smodel_;
    for ( int idx=0; idx<nrlayers; idx++ )
    {
	spike.reflectivity_ = reflectivity_->get( idx, offsetidx );
	spike.depth_ = getLayerDepth( layers, idx);
	spike.time_ = twt_->get( idx, offsetidx );
	spike.correctedtime_ = twt_->get( idx, 0 );
	model += spike;
    }
    return true;
}


bool RayTracer1D::getTWT( int offset, TimeDepthModel& d2tm ) const
{
    const int offsetidx = offsetpermutation_[offset];

    if ( !twt_ || offsetidx<0 || offsetidx>=twt_->info().getSize(1) )
	return false;

    const AIModel& layers = pmodel_.size() ? pmodel_ : smodel_;
    const int nrtimes = twt_->info().getSize(0);
    const int nrlayers = layers.size();

    TypeSet<float> times, depths;

    depths += setup_.sourcedepth_;
    times += 0;
    for ( int idx=0; idx<nrlayers; idx++ )
    {
	depths += getLayerDepth( layers, idx );
	times += idx < nrtimes ? twt_->get( idx, offsetidx ) : 
	    times[times.size()-1] + 2*layers[idx].thickness_/layers[idx].vel_;
    }
    return d2tm.setModel( depths.arr(), times.arr(), nrlayers+1 ); 
}

