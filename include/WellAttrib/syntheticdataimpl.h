#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bruno
 Date:		July 2011
________________________________________________________________________

-*/

#include "wellattribmod.h"

#include "prestackgather.h"
#include "syntheticdata.h"

class SeisTrcBufDataPack;
class PropertyRef;


mExpClass(WellAttrib) PostStackSyntheticData : public SyntheticData
{
public:
				PostStackSyntheticData(const SynthGenParams&,
					       const Seis::SynthGenDataPack&,
					       SeisTrcBufDataPack&);
				~PostStackSyntheticData();

    DataPack::FullID		fullID() const override;
    bool			isPS() const override	   { return false; }
    bool			hasOffset() const override { return false; }
    SynthGenParams::SynthType	synthType() const override
				{ return SynthGenParams::ZeroOffset; }

    const SeisTrc*		getTrace(int trcnr) const override;
    int				nrPositions() const override;
    TrcKey			getTrcKey(int trcnr) const override;
    ZSampling			zRange() const override;

    const SeisTrcBufDataPack&	postStackPack() const;
    SeisTrcBufDataPack&		postStackPack();

    const FlatDataPack*		getTrcDP() const override;
    const FlatDataPack*		getFlattenedTrcDP(const TypeSet<float>& zvals,
						  bool istime) const override;

    static const char*		sDataPackCategory();

private:

    static DataPack::MgrID	groupID();

};


mExpClass(WellAttrib) PostStackSyntheticDataWithInput
				: public PostStackSyntheticData
{
public:
				PostStackSyntheticDataWithInput(
				    const SynthGenParams&,
				    const Seis::SynthGenDataPack&,
				    SeisTrcBufDataPack&);
				~PostStackSyntheticDataWithInput();

    void			useGenParams(const SynthGenParams&);
    void			fillGenParams(SynthGenParams&) const;

protected:
    BufferString		inpsynthnm_;
};


mExpClass(WellAttrib) InstAttributeSyntheticData
		: public PostStackSyntheticDataWithInput
{
public:
				InstAttributeSyntheticData(
				    const SynthGenParams& sgp,
				    const Seis::SynthGenDataPack& synthdp,
				    SeisTrcBufDataPack& sbufdp );

    bool			isAttribute() const override	{ return true; }
    SynthGenParams::SynthType	synthType() const override
				{ return SynthGenParams::InstAttrib; }

    void			useGenParams(const SynthGenParams&);
    void			fillGenParams(SynthGenParams&) const;

protected:
    Attrib::Instantaneous::OutType	attribtype_;
};


mExpClass(WellAttrib) PSBasedPostStackSyntheticData
				: public PostStackSyntheticDataWithInput
{
public:
				PSBasedPostStackSyntheticData(
				    const SynthGenParams&,
				    const Seis::SynthGenDataPack&,
				    SeisTrcBufDataPack&);
				~PSBasedPostStackSyntheticData();

    void			useGenParams(const SynthGenParams&);
    void			fillGenParams(SynthGenParams&) const;

protected:
    Interval<float>		anglerg_;
};


mExpClass(WellAttrib) AVOGradSyntheticData
		: public PSBasedPostStackSyntheticData
{
public:
				AVOGradSyntheticData(
					const SynthGenParams& sgp,
					const Seis::SynthGenDataPack& synthdp,
					SeisTrcBufDataPack& sbufdp )
				    : PSBasedPostStackSyntheticData(sgp,synthdp,
								    sbufdp)
				{}

    bool			isAVOGradient() const override	{ return true; }
    SynthGenParams::SynthType	synthType() const override
				{ return SynthGenParams::AVOGradient; }
protected:
};


mExpClass(WellAttrib) AngleStackSyntheticData
		: public PSBasedPostStackSyntheticData
{
public:
				AngleStackSyntheticData(
					const SynthGenParams& sgp,
					const Seis::SynthGenDataPack& synthdp,
					SeisTrcBufDataPack& sbufdp )
				    : PSBasedPostStackSyntheticData(sgp,synthdp,
								    sbufdp)
				{}

    bool			isAngleStack() const override	{ return true; }
    SynthGenParams::SynthType	synthType() const override
				{ return SynthGenParams::AngleStack; }
protected:
};


mExpClass(WellAttrib) PreStackSyntheticData : public SyntheticData
{
public:
				PreStackSyntheticData(const SynthGenParams&,
						const Seis::SynthGenDataPack&,
						PreStack::GatherSetDataPack&);
				~PreStackSyntheticData();

    DataPack::FullID			fullID() const override;

    void				setName(const char*) override;
    bool				isPS() const override  { return true; }
    bool				isNMOCorrected() const;
    bool				hasOffset() const override;
    Interval<float>			offsetRange() const;
    float				offsetRangeStep() const;
    SynthGenParams::SynthType		synthType() const override
					{ return SynthGenParams::PreStack; }
    int					nrPositions() const override;
    TrcKey				getTrcKey(int trcnr) const override;
    DataPack::ID			getGatherIDByIdx(int trcnr,
						      bool angles=false) const;
    ZSampling				zRange() const override;

    void				setAngleData(
					    const PreStack::GatherSetDataPack*);
    const SeisTrc*			getTrace(int trcnr) const override
					{ return getTrace(trcnr,nullptr); }
    const SeisTrc*			getTrace(int trcnr,int* offset) const;
    SeisTrcBuf*				getTrcBuf(float startoffset,
						  const Interval<float>* of
								=nullptr) const;

    PreStack::GatherSetDataPack&	preStackPack();
    const PreStack::GatherSetDataPack&	preStackPack() const;
    const PreStack::GatherSetDataPack&	angleData() const { return *angledp_; }
    bool			hasAngles() const	{ return angledp_; }
    void			obtainGathers();
				/*!< Make all gathers available in the
				     FlatDataPack Mgr */

    ConstRefMan<PreStack::Gather> getGather(int trcnr,bool angles=false) const;

    const FlatDataPack*		getTrcDP() const override
				{ return getTrcDPAtOffset(0); }
    const FlatDataPack*		getTrcDPAtOffset(int offsidx) const;
    const FlatDataPack*		getFlattenedTrcDP(const TypeSet<float>& zvals,
						  bool istime) const override
				{ return getFlattenedTrcDP(zvals,istime,0); }
    const FlatDataPack*		getFlattenedTrcDP(const TypeSet<float>& zvals,
						 bool istime,int offsidx) const;

private:

    ConstRefMan<PreStack::GatherSetDataPack> angledp_;
    void				convertAngleDataToDegrees(
						PreStack::Gather&) const;
    static DataPack::MgrID		groupID();
};


mExpClass(WellAttrib) StratPropSyntheticData : public PostStackSyntheticData
{
public:
				StratPropSyntheticData(const SynthGenParams&,
						const Seis::SynthGenDataPack&,
						SeisTrcBufDataPack&,
						const PropertyRef&);

    bool			isStratProp() const override	{ return true; }

    const PropertyRef&		propRef() const { return prop_; }
    SynthGenParams::SynthType	synthType() const override
				{ return SynthGenParams::StratProp; }

protected:
    const PropertyRef&		prop_;
};


