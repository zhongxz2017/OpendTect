/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne and Kristofer
 Date:          December 2007
________________________________________________________________________

-*/
static const char* rcsID = "$Id: initseis.cc,v 1.10 2011-08-23 06:54:11 cvsbert Exp $";

#include "initseis.h"
#include "timedepthconv.h"
#include "seisseqio.h"
#include "segytr.h"
#include "seiscbvs.h"
#include "seis2dlineio.h"
#include "seispscubetr.h"
#include "segydirecttr.h"

#define sKeySeisTrcTranslatorGroup "Seismic Data"
defineTranslatorGroup(SeisTrc,sKeySeisTrcTranslatorGroup);
defineTranslator(CBVS,SeisTrc,"CBVS");
defineTranslator(SEGY,SeisTrc,"SEG-Y");
defineTranslator(TwoD,SeisTrc,"2D");
defineTranslator(SEGYDirect,SeisTrc,"SEGYDirect");
defineTranslator(SeisPSCube,SeisTrc,"PS Cube");

mDefSimpleTranslatorSelector(SeisTrc,sKeySeisTrcTranslatorGroup)
mDefSimpleTranslatorioContext(SeisTrc,Seis)

void Seis::initStdClasses()
{
    mIfNotFirstTime( return );

    LinearT2DTransform::initClass();
    LinearD2TTransform::initClass();
    Time2DepthStretcher::initClass();
    Depth2TimeStretcher::initClass();
    Seis::ODSeqInp::initClass();
    Seis::ODSeqOut::initClass();
}
