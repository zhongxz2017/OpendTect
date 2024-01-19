/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiexpfault.h"

#include "bufstring.h"
#include "ctxtioobj.h"
#include "emfault3d.h"
#include "emfaultset3d.h"
#include "emfaultstickset.h"
#include "emioobjinfo.h"
#include "lmkemfaulttransl.h"
#include "emmanager.h"
#include "executor.h"
#include "file.h"
#include "filepath.h"
#include "ioman.h"
#include "ioobj.h"
#include "oddirs.h"
#include "od_helpids.h"
#include "od_ostream.h"
#include "ptrman.h"
#include "strmprov.h"
#include "survinfo.h"
#include "unitofmeasure.h"
#include "zaxistransform.h"

#include "uibutton.h"
#include "uichecklist.h"
#include "uifileinput.h"
#include "uiioobjsel.h"
#include "uiioobjselgrp.h"
#include "uimsg.h"
#include "uimultisurfaceread.h"
#include "uistrings.h"
#include "uitaskrunner.h"
#include "uit2dconvsel.h"
#include "uiunitsel.h"


#define mGetObjNr \
    isbulk ? mPlural : 1 \

#define mGet( tp, fss, f3d, fset ) \
    StringView(tp) == EMFaultStickSetTranslatorGroup::sGroupName() ? fss : \
    (StringView(tp) == EMFaultSet3DTranslatorGroup::sGroupName() ? fset : f3d)

#define mGetCtxt(tp) \
    mGet( tp, mIOObjContext(EMFaultStickSet), mIOObjContext(EMFault3D), \
	mIOObjContext(EMFaultSet3D) )

#define mGetTitle(tp) \
    mGet( tp, uiStrings::phrExport( uiStrings::sFaultStickSet(mGetObjNr) ), \
	      uiStrings::phrExport( uiStrings::sFault(mGetObjNr) ), \
	      uiStrings::phrExport( uiStrings::sFaultSet(mGetObjNr) ) )

#define mGetLbl(tp) \
    mGet( tp, uiStrings::phrInput( uiStrings::sFaultStickSet(mGetObjNr) ), \
	      uiStrings::phrInput( uiStrings::sFault(mGetObjNr) ), \
	      uiStrings::phrInput( uiStrings::sFaultSet(mGetObjNr) ) )


uiExportFault::uiExportFault( uiParent* p, const char* typ, bool isbulk )
    : uiDialog(p,uiDialog::Setup(mGetTitle(typ),mNoDlgTitle,
				 mGet(typ,mODHelpKey(mExportFaultStickHelpID),
				 mODHelpKey(mExportFaultHelpID),mTODOHelpKey)))
    , isbulk_(isbulk)
    , typ_(typ)
{
    setModal( false );
    setDeleteOnClose( false );
    setOkCancelText( uiStrings::sExport(), uiStrings::sClose() );

    coordfld_ = new uiGenInput( this, tr("Write coordinates as"),
				BoolInpSpec(true,tr("X/Y"),tr("Inl/Crl")) );
    mAttachCB( coordfld_->valueChanged, uiExportFault::exportCoordSysChgCB );

    IOObjContext ctxt = mGetCtxt( typ );
    uiIOObjSelGrp::Setup su; su.choicemode_ = OD::ChooseAtLeastOne;
    if ( isbulk_ )
    {
	zdomypefld_ = new uiGenInput( this, tr("Depth Domain"),
	    BoolInpSpec(SI().zIsTime(),
	    uiStrings::sTime(), uiStrings::sDepth()) );
	mAttachCB( zdomypefld_->valueChanged, uiExportFault::zDomainTypeChg );

	auto* multigrp = new uiGroup( this, "Multi Surface Read" );
	multigrp->attach( alignedBelow, zdomypefld_ );
	const ZDomain::Info& depthinfo = SI().depthsInFeet() ?
	    ZDomain::DepthFeet() : ZDomain::DepthMeter();
	multisurfdepthread_ = new uiMultiSurfaceRead( multigrp, typ,
								&depthinfo );
	multisurftimeread_ = new uiMultiSurfaceRead( multigrp, typ,
							&ZDomain::TWT() );
	coordfld_->attach( alignedBelow, multigrp );
    }
    else
    {
	infld_ = new uiIOObjSel( this, ctxt, mGetLbl(typ) );
	mAttachCB(infld_->selectionDone,uiExportFault::inpSelChg);
	coordfld_->attach( alignedBelow, infld_ );
    }

    uiObject* attachobj = coordfld_->attachObj();
    if ( SI().hasProjection() )
    {
	coordsysselfld_ = new Coords::uiCoordSystemSel( this );
	coordsysselfld_->attach( alignedBelow, attachobj );
	attachobj = coordsysselfld_->attachObj();
    }

    zfld_ = new uiGenInput( this, uiStrings::phrOutput( toUiString("Z") ),
			    BoolInpSpec(true) );
    mAttachCB( zfld_->valueChanged, uiExportFault::addZChg );
    zfld_->attach( alignedBelow, attachobj );

    zunitsel_ = new uiUnitSel( this, uiUnitSel::Setup(tr("Z Unit")) );
    zunitsel_->setUnit( UnitOfMeasure::surveyDefZUnit() );
    zunitsel_->attach( alignedBelow, zfld_ );

    stickidsfld_ = new uiCheckList( this, uiCheckList::ChainAll,
				    OD::Horizontal );
    stickidsfld_->setLabel( uiStrings::sWrite() );
    stickidsfld_->addItem( tr("Stick index") ).addItem( tr("Node index" ));
    stickidsfld_->setChecked( 0, true ); stickidsfld_->setChecked( 1, true );
    stickidsfld_->attach( alignedBelow, zunitsel_ );

    if ( mGet(typ,true,false,false) )
    {
	linenmfld_ = new uiCheckBox( this,
				     tr("Write line name if picked on 2D") );
	linenmfld_->setChecked( true );
	linenmfld_->attach( alignedBelow, stickidsfld_ );
    }

    outfld_ = new uiASCIIFileInput( this, false );
    if ( linenmfld_ )
	outfld_->attach( alignedBelow, linenmfld_ );
    else
	outfld_->attach( alignedBelow, stickidsfld_ );

    exportCoordSysChgCB(nullptr);

    dispstr_ = EMFaultStickSetTranslatorGroup::sGroupName() == typ
	? uiStrings::sFaultStickSet(mGetObjNr)
	: (EMFaultSet3DTranslatorGroup::sGroupName() == typ
	    ? uiStrings::sFaultSet(mGetObjNr) : uiStrings::sFault(mGetObjNr) );

    mAttachCB( postFinalize(), uiExportFault::initGrpCB );
}


uiExportFault::~uiExportFault()
{
    detachAllNotifiers();
}


void uiExportFault::initGrpCB( CallBacker* )
{
    if ( isbulk_ )
	zDomainTypeChg( nullptr );
    else
	inpSelChg( nullptr );
}


void uiExportFault::zDomainTypeChg(CallBacker*)
{
    const bool istime = zdomypefld_->getBoolValue();
    multisurfdepthread_->display( !istime );
    multisurftimeread_->display( istime );
    const ZDomain::Info& zinfo( istime ? ZDomain::Time() : ZDomain::Depth() );
    zunitsel_->setUnit( UnitOfMeasure::zUnit(zinfo) );
}


void uiExportFault::inpSelChg( CallBacker* )
{
    const IOObj* ioobj = infld_ ? infld_->ioobj() : nullptr;
    if ( ioobj )
    {
	const EM::IOObjInfo info( ioobj->key() );
	zunitsel_->setUnit( info.getZUoM() );
    }
}


bool uiExportFault::getInputMIDs( TypeSet<MultiID>& midset )
{
    if ( isbulk_ )
    {
	const bool istime = zdomypefld_->getBoolValue();
	if ( istime )
	    multisurftimeread_->getSurfaceIds( midset );
	else
	    multisurfdepthread_->getSurfaceIds( midset );
    }
    else
    {
	const IOObj* ioobj = infld_->ioobj( false );
	if ( !ioobj )
	    return false;

	MultiID mid = ioobj->key();
	midset.add(mid);
    }

    return true;
}


#define mErrRet(s) { uiMSG().error(s); return false; }

bool uiExportFault::writeAscii()
{
    TypeSet<MultiID> midset;
    if ( !getInputMIDs(midset) )
	mErrRet(tr("Cannot find object in database"))

    const BufferString fname = outfld_->fileName();
    od_ostream ostrm( fname );
    if ( !ostrm.isOK() )
	return false;

    uiTaskRunner taskrunner( this );
    PtrMan<Executor> objloader = EM::EMM().objectLoader( midset );

    if ( objloader && !TaskRunner::execute(&taskrunner, *objloader) )
	return false;

    for ( int idx=0; idx<midset.size(); idx++ )
    {
	EM::ObjectID objid = EM::EMM().getObjectID( midset[idx] );
	if ( !objid.isValid() )
	    continue;

	RefMan<EM::EMObject> emobj = EM::EMM().getObject(objid);
	mDynamicCastGet(EM::Fault3D*,f3d,emobj.ptr())
	mDynamicCastGet(EM::FaultStickSet*,fss,emobj.ptr())
	mDynamicCastGet(EM::FaultSet3D*,fset,emobj.ptr())
	if ( !f3d && !fss && !fset )
	    return false;

	const int nrobjs = fset ? fset->nrFaults() : 1;
	for ( int oidx=0; oidx<nrobjs; oidx++ )
	{
	    RefMan<EM::EMObject> fltobj = emobj;
	    BufferString objnm = fltobj->name();

	    if ( fset )
	    {
		EM::FaultID fltid = fset->getFaultID( oidx );
		fltobj = fset->getFault3D( fltid );
		objnm = fset->name();
		objnm.add("_").add( fltid.asInt() );
	    }
	}
    }

    ostrm.close();
    return true;
}


void uiExportFault::addZChg( CallBacker* )
{
    const bool displayunit = zfld_->getIntValue()!=1;
    zunitsel_->display( displayunit );
}


void uiExportFault::exportCoordSysChgCB( CallBacker* )
{
    if ( coordsysselfld_ )
	coordsysselfld_->display( coordfld_->getBoolValue() );
}


bool uiExportFault::acceptOK( CallBacker* )
{
    bool isobjsel = true;
    if ( !isbulk_ )
	isobjsel = infld_->ioobj( false );
    else
    {
	TypeSet<MultiID> midset;
	getInputMIDs( midset );
	if ( midset.isEmpty() )
	    isobjsel = false;
    }

    if ( !isobjsel )
	mErrRet( uiStrings::phrSelect(tr("the input fault")) )

    const BufferString outfnm( outfld_->fileName() );
    if ( outfnm.isEmpty() )
	mErrRet( uiStrings::sSelOutpFile() )

    if ( File::exists(outfnm) &&
	 !uiMSG().askOverwrite(uiStrings::sOutputFileExistsOverwrite()) )
	return false;

    if ( !writeAscii() )
    {
	uiMSG().error(uiStrings::phrCannotCreateDBEntryFor(
						    tr("selected faults")));
	return false;
    }

    uiString msg = tr("%1 successfully exported.").arg( dispstr_ );
    msg.addNewLine();
    if ( typ_ == EMFaultSet3DTranslatorGroup::sGroupName() )
    {
	msg.append( tr("Do you want to continue exporting FaultSet"), true );
    }
    else
    {
	msg.append( tr("Do you want to export more %2?").arg(
	    typ_ == EMFaultStickSetTranslatorGroup::sGroupName() ?
	    uiStrings::sFaultStickSet(mPlural) : uiStrings::sFault(mPlural)),
	    true );
    }

    const bool ret = uiMSG().askGoOn( msg, uiStrings::sYes(),
				    tr("No, close window") );

    return !ret;
}
