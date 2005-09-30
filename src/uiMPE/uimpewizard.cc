/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          March 2004
 RCS:           $Id: uimpewizard.cc,v 1.27 2005-09-30 19:55:18 cvskris Exp $
________________________________________________________________________

-*/


#include "uimpewizard.h"

#include "ctxtioobj.h"
#include "emmanager.h"
#include "emobject.h"
#include "emseedpicker.h"
#include "executor.h"
#include "ioman.h"
#include "ioobj.h"
#include "mpeengine.h"
#include "ptrman.h"
#include "survinfo.h"
#include "uicolor.h"
#include "uicursor.h"
#include "uigeninput.h"
#include "uigroup.h"
#include "uiioobjsel.h"
#include "uilabel.h"
#include "uilistbox.h"
#include "uimsg.h"
#include "uimpepartserv.h"
#include "uimpesetup.h"
#include "uiseparator.h"
#include "uispinbox.h"


namespace MPE {

int Wizard::defcolnr = 0;

const int Wizard::sNamePage		= 0;
const int Wizard::sSeedSetupPage	= 1;
const int Wizard::sFinalizePage	= 2;


Wizard::Wizard( uiParent* p, uiMPEPartServer* mps )
    : uiWizard(p,uiDialog::Setup("Tracking Wizard","XXXXXXX tracking","")
				.modal(false))
    , mpeserv(mps)
    , sid( -1 )
    , currentobject(-1)
    , objectcreated(false)
    , trackercreated(false)
    , ioparentrycreated(false)
    , ispicking(false)
    , typefld( 0 )
{
    objselgrp = createNamePage();
    addPage( objselgrp );
    addPage( createSeedSetupPage() );
    addPage( createFinalizePage() );

    seedbox.setEmpty();
    setHelpID( "108.0.0" );
}


Wizard::~Wizard()
{
}


uiIOObjSelGrp* Wizard::createNamePage()
{
    const IOObjContext* ctxttemplate = EM::EMM().getContext(trackertype);
    if ( !ctxttemplate )
    {
	ctxttemplate = EM::EMM().getContext("Fault");
    }

    PtrMan<IOObjContext> ctxt = new IOObjContext(*ctxttemplate);
    ctxt->forread = false;

    const CtxtIOObj ctio( *ctxt );
    return new uiIOObjSelGrp( this, ctio );
}


uiGroup* Wizard::createSeedSetupPage()
{
    uiGroup* grp = new uiGroup( this, "Page 2" );

    BufferString str( "In this step you'll have to create your seedpoints.\n"
		      "First select tracking setup, then\n"
		      "create a seedpoint by clicking on a slice.\n"
		      "You can remove a seedpoint by ctrl-click." );
    uiLabel* lbl = new uiLabel( grp, str );
    
    uiSeparator* sep1 = new uiSeparator( grp, "Separator 1" );
    sep1->attach( stretchedBelow, lbl );

    setupgrp = new uiSetupSel( grp, mpeserv->attrset );
    setupgrp->attach( alignedBelow, lbl );
    setupgrp->attach( ensureBelow, sep1 );
    setupgrp->setupchg.notify( mCB(this,Wizard,setupChange) );

    uiSeparator* sep2 = new uiSeparator( grp, "Separator 2" );
    sep2->attach( stretchedBelow, setupgrp );

    colorfld = new uiColorInput( grp, Color::drawDef(defcolnr++),
	    			 "Object color" );
    colorfld->colorchanged.notify( mCB(this,Wizard,colorChangeCB) );
    colorfld->attach( alignedBelow, setupgrp );
    colorfld->attach( ensureBelow, sep2 );
    
    return grp;
}


uiGroup* Wizard::createFinalizePage()
{
    uiGroup* grp = new uiGroup( this, "Page 4" );
    BufferStringSet trackernames;
    engine().getAvaliableTrackerTypes( trackernames );

    BufferString str("Do you want to track another ");
    str += trackernames.size()>1 ? "surface" : (const char*) trackernames[0];
    str += "?";
    uiLabel* lbl = new uiLabel( grp, str );

    anotherfld = new uiGenInput( grp, "", BoolInpSpec() );
    anotherfld->attach( alignedBelow, lbl );
    anotherfld->valuechanged.notify( mCB(this,Wizard,anotherSel) );

    if ( trackernames.size()>1 )
    {
	typefld = new uiGenInput( grp, "Type",
				  StringListInpSpec(trackernames) );
	typefld->attach( alignedBelow, anotherfld );
    }

    anotherSel(0);
    return grp;
}


bool Wizard::prepareNamePage()
{
    const IOObjContext* ctxttemplate = EM::EMM().getContext(trackertype);
    if ( !ctxttemplate )
    {
	pErrMsg("Cannot find context");
	return false;
    }

    PtrMan<IOObjContext> ctxt = new IOObjContext(*ctxttemplate);
    ctxt->forread = false;

    const CtxtIOObj ctio( *ctxt );
    objselgrp->setContext( ctio );

    if ( objselgrp->getListField()->box()->size() )
	objselgrp->getListField()->box()->selectAll(false);

    objselgrp->getNameField()->setFocus();

    return true;
}

#define mErrRet(msg) { uiMSG().error(msg); return false; }

bool Wizard::leaveNamePage( bool process )
{
    if ( !process ) return true;

    bool didexist = true;
    const char* newobjnm = objselgrp->getNameField()->text(); 
    if ( *newobjnm )
    {
	PtrMan<IOObj> ioobj = IOM().getLocal( newobjnm );
	if ( !ioobj ) didexist = false;
	
    }

    if ( !objselgrp->processInput() )
    {
	pErrMsg("Could not process input");
	return false;
    }

    ioparentrycreated = !didexist;

    const IOObj* ioobj = objselgrp->selected(0);
    if ( !ioobj )
    {
	pErrMsg( "Could not get ioobj");
	return false;
    }

    const bool isimpl = ioobj->implExists(false);
    const bool isreadonly = isimpl && ioobj->implReadOnly();

    EM::ObjectID objid = EM::EMM().multiID2ObjectID( ioobj->key() );
    EM::EMObject* emobj = EM::EMM().getObject( objid );
    if ( emobj )
    {
	uiMSG().error("An object with this name exist and is currently\n"
		      "loaded. Please select another name or quit the\n"
		      "wizard and remove the object with this name from\n"
		      "the tree.");
	return false;
    }
    else if ( isreadonly )
    {
	uiMSG().error("This object is marked as read-only. Please select\n"
		      "another object or make it writable." );
	return false;
    }
    else if ( didexist )
    {
	if ( !uiMSG().askGoOn("An object with that name does already exist."
			      " Overwrite?",true) )
	    return false;
    }
    else
	currentobject = -1;

    return true;
}


bool Wizard::prepareSeedSetupPage()
{
    if ( !(currentobject==-1) )
    {
	const EM::ObjectID objid = EM::EMM().multiID2ObjectID( currentobject );
	EM::EMObject* emobj = EM::EMM().getObject( objid );
	colorfld->setColor( emobj->preferredColor() );
    }
    else
    {
	colorfld->setColor( Color::drawDef(defcolnr) );
    }

    if ( !createTracker() )
	return false;

    const int trackerid = mpeserv->getTrackerID( currentobject );

    const EMTracker* tracker = MPE::engine().getTracker( trackerid );
    const EM::ObjectID objid = tracker->objectID();
    EM::EMObject* emobj = EM::EMM().getObject(objid);
    if ( sid==-1 )
        sid = emobj->sectionID( emobj->nrSections()-1 );

    displayPage( sNamePage, false );
    
    setupgrp->setType( objid, sid );

    colorChangeCB(0);
    allowpicking = true;
    updatePickingStatus();

    return true;
}


bool Wizard::leaveSeedSetupPage( bool process )
{
    allowpicking = false;
    updatePickingStatus();

    if ( !process ) return true;

    const int trackerid = mpeserv->getTrackerID( currentobject );
    const EMTracker* tracker = MPE::engine().getTracker( trackerid );
    const EM::ObjectID objid = tracker->objectID();
    EM::EMObject* emobj = EM::EMM().getObject(objid);
    const TypeSet<EM::PosID>* pids =
	emobj->getPosAttribList( EM::EMObject::sSeedNode );

    if ( !pids || !pids->size() )
    {
	allowpicking = true;
	updatePickingStatus();
	mErrRet( "You did not create any seedpoints" );
    }

    if ( !setupgrp->isSetToValidSetup() )
	mErrRet( "Please select Tracking Setup" );

    displayPage( sSeedSetupPage, false );

    return true;
}


bool Wizard::prepareFinalizePage()
{
    if ( typefld ) typefld->setText(trackertype);
    return true;
}


bool Wizard::leaveFinalizePage(bool process)
{
    if ( !process ) return true;

    anotherSel(0);
    if ( anotherfld->getBoolValue() )
	setTrackingType( typefld ? typefld->text() : (const char*)trackertype );

    const EM::ObjectID objid = EM::EMM().multiID2ObjectID( currentobject );
    EM::EMObject* emobj = EM::EMM().getObject( objid );
    PtrMan<Executor> saver = emobj->saver();

    if ( saver ) saver->execute();
    adjustSeedBox();

    return true;
}


void Wizard::isStarting()
{
    seedbox.setEmpty();
}


bool Wizard::isClosing( bool iscancel )
{
    if ( iscancel )
    {
	//This must come before tracker is removed since
	//applman needs tracker to know what to remove.
	if ( objectcreated )
	{
	    mpeserv->sendEvent( ::uiMPEPartServer::evRemoveTreeObject );
	    objectcreated = false; //Avoids increment of defcol when reseting
	}

	if ( ioparentrycreated )
	{
	    IOM().to( currentobject );
	    PtrMan<IOObj> ioobj = IOM().get(currentobject);

	    if ( !ioobj || !fullImplRemove(*ioobj) ||
	         !IOM().permRemove(currentobject) )
	    {
		pErrMsg( "Could not remove object" );
	    }
	}


	if ( trackercreated )
	{
	    const int trackerid = mpeserv->getTrackerID( currentobject );
	    MPE::engine().removeTracker( trackerid );
	}
    }

    if ( !seedbox.isEmpty() )
    {
	mpeserv->expandActiveArea(seedbox);
    }


    return true;
}


void Wizard::anotherSel( CallBacker* )
{
    const bool cont = anotherfld->getBoolValue();
    typefld->display( cont );
    setRotateMode( cont );
}


void Wizard::colorChangeCB( CallBacker* )
{
    if ( !(currentobject==-1) )
    {
	const EM::ObjectID objid = EM::EMM().multiID2ObjectID( currentobject );
	EM::EMObject* emobj = EM::EMM().getObject( objid );
	emobj->setPreferredColor( colorfld->color() );
    }
}


bool Wizard::preparePage( int page )
{
    switch ( page )
    {
	case 0: return prepareNamePage();
	case 1: return prepareSeedSetupPage();
	case 3: return prepareFinalizePage();
    }

    return true;
}


bool Wizard::leavePage( int page, bool process )
{
    switch ( page )
    {
	case 0: return leaveNamePage(process);
	case 1: return leaveSeedSetupPage(process);
	case 3: return leaveFinalizePage(process);
    }

    return true;
}


void Wizard::adjustSeedBox()
{
    const int trackerid = mpeserv->getTrackerID( currentobject );

    const EMTracker* tracker = MPE::engine().getTracker( trackerid );
    const EM::ObjectID objid = tracker->objectID();
    EM::EMObject* emobj = EM::EMM().getObject(objid);

    PtrMan<EM::EMObjectIterator> iterator = emobj->createIterator(sid);

    while ( true )
    {
	const EM::PosID pid = iterator->next();
	if ( pid.objectID()==-1 )
	    break;

	const Coord3 pos = emobj->getPos(pid);
	if ( !pos.isDefined() )
	    continue;

	const BinID bid = SI().transform(pos);
	if ( seedbox.isEmpty() )
	{
	    seedbox.hrg.start = seedbox.hrg.stop = bid;
	    seedbox.zrg.start = seedbox.zrg.stop = pos.z;
	}
	else
	{
	    seedbox.hrg.include(bid);
	    seedbox.zrg.include(pos.z);
	}
    }
}


void Wizard::reset()
{
    if ( objectcreated )
	defcolnr++;

    sid = -1;
    objectcreated = false;
    ioparentrycreated = false;
    trackercreated = false;
    currentobject = -1;

    for ( int idx=0; idx<nrPages(); idx++ )
	displayPage( idx, true );
}


void Wizard::setTrackingType( const char* tp )
{
    trackertype = tp;
    updateDialogTitle();
}


void Wizard::setObject( const MultiID& mid, const EM::SectionID& sectionid )
{
    currentobject = mid;
    sid = sectionid;
    const EM::ObjectID objid = EM::EMM().multiID2ObjectID( mid );
    const EM::EMObject* emobj = EM::EMM().getObject( objid );
    if ( emobj )
	setTrackingType( emobj->getTypeStr() );
}


void Wizard::updateDialogTitle()
{
    BufferString str( trackertype ); str += " Tracking";
    setTitleText( str );
}


bool Wizard::createTracker()
{
    if ( currentobject==-1 )
    {
	EM::ObjectID objid = EM::EMM().createObject(trackertype,
		objselgrp->getNameField()->text());
	EM::EMObject* emobj = EM::EMM().getObject(objid);
	const int id = engine().addTracker( emobj );
	if ( id==-1 )
	{
	    pErrMsg( "Could not create tracker" );
	    return false;
	}

	EMTracker* tracker = engine().getTracker(id);

	if ( !engine().getEditor(objid,false) )
	    engine().getEditor(objid,true);

	mpeserv->activetrackerid = id;
	if ( !mpeserv->sendEvent( ::uiMPEPartServer::evAddTreeObject ) )
	{
	    pErrMsg("Could not add treeitem");
	    engine().removeTracker( id );
	    emobj->ref(); emobj->unRef();
	    return false;
	}

	currentobject = emobj->multiID();
	objectcreated = true;
	trackercreated = true;
    }
    else if ( mpeserv->getTrackerID(currentobject)<0 )
    {
	const EM::ObjectID objid = EM::EMM().multiID2ObjectID( currentobject );
	EM::EMObject* emobj = EM::EMM().getObject( objid );

	if ( MPE::engine().addTracker(emobj)<0 )
	{
	    pErrMsg( "Could not create tracker" );
	    return false;
	}

	trackercreated = true;
    }

    return true;
}


void Wizard::updatePickingStatus()
{
    const bool shouldbeon = allowpicking && setupgrp->isSetToValidSetup();
    if ( shouldbeon==ispicking )
	return;

    if ( shouldbeon )
    {
	mpeserv->sendEvent( uiMPEPartServer::evStartSeedPick );
	ispicking = true;
    }
    else
    {
	mpeserv->blockdataloading = true;
	mpeserv->sendEvent( uiMPEPartServer::evEndSeedPick );
	mpeserv->blockdataloading = false;
	ispicking = false;
    }
}
	


void Wizard::setupChange( CallBacker* )
{
    updatePickingStatus();

    const int trackerid = mpeserv->getTrackerID( currentobject );
    MPE::EMTracker* tracker = MPE::engine().getTracker( trackerid );
    MPE::EMSeedPicker* seedpicker = tracker->getSeedPicker(false);
    if ( !seedpicker )
	return;

    uiCursor::setOverride( uiCursor::Wait );
    seedpicker->reTrack();
    uiCursor::restoreOverride();
}

}; // namespace MPE
