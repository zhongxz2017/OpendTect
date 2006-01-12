#ifndef volstatsattrib_h
#define volstatsattrib_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Kristofer Tingdahl
 Date:          07-10-1999
 RCS:           $Id: volstatsattrib.h,v 1.8 2006-01-12 20:37:38 cvsnanne Exp $
________________________________________________________________________

-*/

#include "attribprovider.h"

/*!\brief Volume Statistics Attribute

VolumeStatistics stepout=1,1 shape=Rectangle|Ellipse gate=[0,0]
		 steering=

VolumeStatistics collects all samples within the timegate from all traces
within the stepout.

If steering is enabled, the timegate is taken relative to the steering.

Inputs:
0-(nrvolumes-1)         The data
nrvolumes  -            Steerings (only if steering is enabled)

Outputs:
0       Avg
1       Med
2       Variance
3       Min
4       Max
5       Sum
6       Normalized Variance

*/


namespace Attrib
{

class VolStats : public Provider
{
public:
    static void			initClass();
				VolStats(Desc&);

    static const char*		attribName()	  { return "VolumeStatistics"; }
    static const char*		nrvolumesStr()	  { return "nrvolumes"; }
    static const char*		stepoutStr()	  { return "stepout"; }
    static const char*		shapeStr()	  { return "shape"; }
    static const char*		gateStr()	  { return "gate"; }
    static const char*		absolutegateStr() { return "absolutegate"; }
    static const char*		steeringStr()	  { return "steering"; }
    static const char*		shapeTypeStr(int);
    void			initSteering();

protected:
				~VolStats();
    static Provider*		createInstance(Desc&);
    static void			updateDesc(Desc&);

    bool			allowParallelComputing() const
				{ return true; }

    bool			getInputOutput(int inp,TypeSet<int>& res) const;
    bool			getInputData(const BinID&,int zintv);
    bool			computeData(const DataHolder&,
	    				    const BinID& relpos,
					    int z0,int nrsamples) const;

    const BinID*		reqStepout(int input,int output) const;
    const Interval<float>*	reqZMargin(int input,int output) const;
    const Interval<float>*      desZMargin(int input,int output) const;

    BinID			stepout;
    int				shape;
    Interval<float>		gate;
    bool			steering;

    Interval<float>             desgate;

    TypeSet<BinID>      	positions;
    Interval<float>		absdepthgate;

    static int          	outputtypes[];

    int				dataidx_;

    ObjectSet<const DataHolder>	inputdata;
    const DataHolder*		steeringdata;
};

} // namespace Attrib


#endif
