#ifndef datapackimpl_h
#define datapackimpl_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Nanne Hemstra and Helene Huck
 Date:		January 2007
 RCS:		$Id: attribdatapack.h,v 1.4 2007-01-24 16:50:15 cvsnanne Exp $
________________________________________________________________________

-*/

#include "flatdispdatapack.h"
#include "cubesampling.h"

template <class T> class Array2D;
template <class T> class Array2DSlice;
namespace Attrib { class DataCubes; }
namespace FlatDisp { class PosData; }
class Coord3;
class IOPar;


/*!\brief A cube data packet: contains sampled data + positioning */ 

class CubeDataPack : public FlatDispDataPack
{
public:
    				CubeDataPack(Attrib::DataCubes*);
    				~CubeDataPack();

    const CubeSampling		sampling() const;
    const Attrib::DataCubes&	cube() const		{ return *cube_; }
    Attrib::DataCubes&		cube()			{ return *cube_; }

    Array2D<float>&		data();
    const Array2D<float>&	data() const;
    void			positioning(FlatDisp::PosData&);

    float			nrKBytes() const	{ return 0; }
    void			dumpInfo(IOPar&) const	{}

protected:

    Attrib::DataCubes*		cube_;
    Array2DSlice<float>*	arr2dsl_;
};


/*!\brief A line data packet: contains sampled data + positioning */

class VertPolyLineDataPack : public DataPack
{
public:

				VertPolyLineDataPack( Array2D<float>* arr,
						      ObjectSet<Coord3>* pos )
				    : arr_(arr)
				    , pos_(pos)		{}
				~VertPolyLineDataPack();

    const ObjectSet<Coord3>&	positions() const	{ return *pos_; }
    ObjectSet<Coord3>&		positions()		{ return *pos_; }
    const Array2D<float>&	cube() const		{ return *arr_; }
    Array2D<float>&		cube()			{ return *arr_; }

    float			nrKBytes() const	{ return 0; }	
    void			dumpInfo(IOPar&) const	{}

    virtual bool		getInfoAtPos(int,IOPar&) const
				    { return false; }

protected:

    ObjectSet<Coord3>*	pos_;
    Array2D<float>*	arr_;
};

#endif
