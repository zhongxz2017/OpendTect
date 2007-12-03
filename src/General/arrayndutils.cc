/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id: arrayndutils.cc,v 1.21 2007-12-03 15:00:47 cvsnanne Exp $";

#include "arrayndutils.h"

#include "windowfunction.h"


DefineEnumNames( ArrayNDWindow, WindowType, 0, "Windowing type")
{ "Box", "Hamming", "Hanning", "Blackman", "Bartlett",
  "CosTaper5", "CosTaper10", "CosTaper20", 0 };


ArrayNDWindow::ArrayNDWindow( const ArrayNDInfo& sz, bool rectangular,
			      ArrayNDWindow::WindowType type )
    : size_(sz)
    , rectangular_(rectangular)
    , window_(0)
    , paramval_(0)
{
    setType( type );
}


ArrayNDWindow::ArrayNDWindow( const ArrayNDInfo& sz, bool rectangular,
			      const char* wintypenm, float paramval )
    : size_(sz)
    , rectangular_(rectangular)
    , windowtypename_(wintypenm)
    , paramval_(paramval)
    , window_(0)
{
    buildWindow( wintypenm, paramval );
}


ArrayNDWindow::~ArrayNDWindow()
{
    delete [] window_;
}


bool ArrayNDWindow::setType( ArrayNDWindow::WindowType wintype )
{
    BufferString winnm = ArrayNDWindow::WindowTypeNames[wintype];
    float paramval = mUdf(float);

    switch( wintype )
    {
    case ArrayNDWindow::CosTaper5:
        winnm = "CosTaper";
	paramval = 0.95;
    break;
    case ArrayNDWindow::CosTaper10:
        winnm = "CosTaper";
	paramval = 0.90;
    break;
    case ArrayNDWindow::CosTaper20:
        winnm = "CosTaper";
	paramval = 0.80;
    break;
    }

    return setType( winnm, paramval );
}


bool ArrayNDWindow::setType( const char* winnm, float val ) 
{
    if ( !buildWindow(winnm,val) )
	return false;

    windowtypename_ = winnm;
    paramval_ = val;
    return true;
}


bool ArrayNDWindow::buildWindow( const char* winnm, float val )
{
    unsigned long totalsz = size_.getTotalSz();
    window_ = new float[totalsz];  
    const int ndim = size_.getNDim();
    ArrayNDIter position( size_ );

    WindowFunction* windowfunc = WinFuncs().create( winnm );
    if ( !windowfunc ) { delete [] window_; window_ = 0; return false; }

    if ( windowfunc->hasVariable() && !windowfunc->setVariable(val) )
    { delete [] window_; window_ = 0; delete windowfunc; return false; }

    if ( !rectangular_ )
    {
	for ( unsigned long off=0; off<totalsz; off++ )
	{
	    float dist = 0;    

	    for ( int idx=0; idx<ndim; idx++ )
	    {
		int sz =  size_.getSize(idx);
		int halfsz = sz / 2;
		float distval = (halfsz==0) ? 0 :
		    		( (float) (position[idx] - halfsz) / halfsz );
		dist += distval * distval;
	    }

	    dist = sqrt( dist );

	    window_[off] = windowfunc->getValue( dist );
	    position.next();
	}	
    }
    else
    {
	for ( unsigned long off=0; off<totalsz; off++ )
	{
	    float windowval = 1;

	    for ( int idx=0; idx<ndim; idx++ )
	    {
		int sz =  size_.getSize(idx);
		int halfsz = sz / 2;
		float distval = ((float) (position[idx] - halfsz) / halfsz);
		windowval *= windowfunc->getValue( distval );
	    }

	    window_[off] = windowval;
	    position.next();
	}	
    }
	

    delete windowfunc;
    return true;
}
