#ifndef volstatsattrib_h
#define volstatsattrib_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Kristofer Tingdahl
 Date:          07-10-1999
 RCS:           $Id: volstatsattrib.h,v 1.17 2007-11-09 16:53:51 cvshelene Exp $
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
7	Most Frequent
8	RMS
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
    static const char*		nrtrcsStr()	  { return "nrtrcs"; }
    static const char*		steeringStr()	  { return "steering"; }
    static const char*		shapeTypeStr(int);
    void			initSteering();

    void			prepPriorToBoundsCalc();

protected:
				~VolStats();
    static Provider*		createInstance(Desc&);
    static void			updateDesc(Desc&);

    bool			allowParallelComputation() const
				{ return true; }

    bool			getInputOutput(int inp,TypeSet<int>& res) const;
    bool			getInputData(const BinID&,int zintv);
    bool			computeData(const DataHolder&,
	    				    const BinID& relpos,
					    int z0,int nrsamples,
					    int threadid) const;

    const BinID*		desStepout(int input,int output) const;
    const Interval<float>*	reqZMargin(int input,int output) const;
    const Interval<float>*	desZMargin(int input,int output) const;

    BinID			stepout_;
    int				shape_;
    Interval<float>		gate_;
    int				minnrtrcs_;
    bool			dosteer_;
    Interval<float>             desgate_;
    Interval<float>             reqgate_;

    TypeSet<BinID>      	positions_;
    int				dataidx_;
    TypeSet<int>		steerindexes_;

    ObjectSet<const DataHolder>	inputdata_;
    const DataHolder*		steeringdata_;
};

} // namespace Attrib


#endif
