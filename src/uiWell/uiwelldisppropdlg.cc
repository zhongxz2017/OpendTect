/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bruno
 Date:		Nov 2008
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uiwelldisppropdlg.h"
#include "uiwelldispprop.h"

#include "uibutton.h"
#include "uibuttongroup.h"
#include "uicombobox.h"
#include "uiseparator.h"
#include "uitabstack.h"

#include "ioman.h"
#include "keystrs.h"
#include "objdisposer.h"
#include "od_helpids.h"
#include "welldata.h"
#include "welldisp.h"
#include "wellman.h"
#include "wellmarker.h"

#define mDispNot (is2ddisplay_? wd_->disp2dparschanged : wd_->disp3dparschanged)

uiWellDispPropDlg::uiWellDispPropDlg( uiParent* p, const Well::Data& wd,
				      OD::Color bkCol, bool is2d )
    : uiDialog(p,uiDialog::Setup(
			tr("Display properties of: %1").arg(wd.name()),
			mNoDlgTitle, mODHelpKey(mWellDispPropDlgHelpID) )
			    .savebutton(true).savechecked(false)
			    .applybutton(true).modal(true)
			    .applytext(uiStrings::sReset()))
    , applyAllReq(this)
    , applyTabReq(this)
    , resetAllReq(this)
    , is2ddisplay_(is2d)
    , bkcol_(bkCol)
{
    wd_ = Well::MGR().get( wd.multiID(),
			   Well::LoadReqs( Well::LogInfos,
					   Well::Mrkrs,
					   is2d ? Well::DispProps2D :
						  Well::DispProps3D ) );
    init();
}


uiWellDispPropDlg::uiWellDispPropDlg( uiParent* p, const Well::Data& wd,
				      bool is2d )
    : uiWellDispPropDlg(p,wd,OD::Color::NoColor(),is2d)
{}


uiWellDispPropDlg::uiWellDispPropDlg( uiParent* p, const MultiID& wid,
				      OD::Color bkCol, bool is2d )
    : uiDialog(p,uiDialog::Setup(
			tr("Display properties of: %1").arg(IOM().nameOf(wid)),
			mNoDlgTitle, mODHelpKey(mWellDispPropDlgHelpID) )
			    .savebutton(true).savechecked(false)
			    .applybutton(true).modal(true)
			    .applytext(uiStrings::sReset()))
    , applyAllReq(this)
    , applyTabReq(this)
    , resetAllReq(this)
    , is2ddisplay_(is2d)
    , bkcol_(bkCol)
{
    wd_ = Well::MGR().get( wid, Well::LoadReqs( Well::LogInfos,
						Well::Mrkrs,
						is2d ? Well::DispProps2D :
						       Well::DispProps3D ) );
    init();
}


uiWellDispPropDlg::uiWellDispPropDlg( uiParent* p, const MultiID& wid,
				      bool is2d )
    : uiWellDispPropDlg(p,wid,OD::Color::NoColor(),is2d )
{}


void uiWellDispPropDlg::init()
{
    bool is2d = is2ddisplay_;

    setCtrlStyle( OkAndCancel );
    setOkText( uiStrings::sSave() );
    setCancelText( uiStrings::sClose() );
    setButtonSensitive( APPLY, false );

    Well::DisplayProperties& props = wd_->displayProperties( is2ddisplay_ );
    if ( bkcol_!=OD::Color::NoColor() )
	props.ensureColorContrastWith( bkcol_ );

    ts_ = new uiTabStack( this, "Well display properties tab stack" );
    ObjectSet<uiGroup> tgs;
    tgs += new uiGroup( ts_->tabGroup(),"Left log properties" );
    tgs += new uiGroup( ts_->tabGroup(),"Center log properties" );
    tgs += new uiGroup( ts_->tabGroup(),"Right log properties" );
    tgs += new uiGroup( ts_->tabGroup(), "Marker properties" );
    if ( !is2d )
	tgs += new uiGroup( ts_->tabGroup(), "Track properties" );

    uiWellLogDispProperties* wlp1 = new uiWellLogDispProperties( tgs[LeftLog],
	uiWellDispProperties::Setup( tr("Line thickness"), tr("Line color"))
	.onlyfor2ddisplay(is2d), props.logs_[0]->left_, &(wd_->logs()) );
    uiWellLogDispProperties* wlp2 = new uiWellLogDispProperties( tgs[CenterLog],
	uiWellDispProperties::Setup( tr("Line thickness"), tr("Line color"))
	.onlyfor2ddisplay(is2d), props.logs_[0]->center_, &(wd_->logs()) );
    uiWellLogDispProperties* wlp3 = new uiWellLogDispProperties( tgs[RightLog],
	uiWellDispProperties::Setup( tr("Line thickness"), tr("Line color"))
	.onlyfor2ddisplay(is2d), props.logs_[0]->right_, &(wd_->logs()) );

    propflds_ += wlp1;
    propflds_ += wlp2;
    propflds_ += wlp3;

    BufferStringSet markernms;
    wd_->markers().getNames( markernms );
    TypeSet<OD::Color> markercols;
    wd_->markers().getColors( markercols );

    uiWellDispProperties::Setup propsu =
	uiWellDispProperties::Setup(tr("Marker size"),tr("Marker color"))
	.onlyfor2ddisplay(is2d);
    uiWellMarkersDispProperties* wellprops = new uiWellMarkersDispProperties(
	tgs[Marker], propsu, props.markers_, markernms );
    wellprops->setAllMarkerNames( markernms, markercols );
    propflds_ += wellprops;

    if ( !is2d )
	propflds_ += new uiWellTrackDispProperties( tgs[Track],
			    uiWellDispProperties::Setup(), props.track_ );

    for ( int idx=0; idx<propflds_.size(); idx++ )
    {
	mAttachCB( propflds_[idx]->propChanged, uiWellDispPropDlg::propChg );
	if ( sKey::Log() == propflds_[idx]->props().subjectName() )
	{
	    if ( idx==LeftLog )
		ts_->addTab( tgs[idx], is2d ? tr("Log 1")
					    : tr("Left Log") );
	    else if ( idx==CenterLog )
		ts_->addTab( tgs[idx], tr("Center Log") );
	    else if ( idx==RightLog )
		ts_->addTab( tgs[idx], is2d ? tr("Log 2")
					    : tr("Right Log") );
	}
	else
	    ts_->addTab( tgs[idx], toUiString(
				      propflds_[idx]->props().subjectName()) );
    }

    uiButtonGroup* bgrp = new uiButtonGroup( this, "", OD::Horizontal );
    new uiPushButton( bgrp, tr("Apply Current to all wells"),
			mCB(this,uiWellDispPropDlg,applyTabPush), true );
    new uiPushButton( bgrp, tr("Apply All to all wells"),
			mCB(this,uiWellDispPropDlg,applyAllPush), true );
    new uiPushButton( bgrp, tr("Reset All"),
			mCB(this,uiWellDispPropDlg,resetAllPush), true );
    bgrp->attach( centeredBelow, ts_ );

    ts_->setCurrentPage( 1 );
    mAttachCB(ts_->selChange(), uiWellDispPropDlg::tabSel);

    setWDNotifiers( true );
    mAttachCB( applyPushed, uiWellDispPropDlg::resetCB );

    tabSel( 0 );
    mDispNot.trigger();
}


uiWellDispPropDlg::~uiWellDispPropDlg()
{
    detachAllNotifiers();
}


void uiWellDispPropDlg::tabSel(CallBacker*)
{
    for ( int idx=0; idx<propflds_.size(); idx++ )
    {
	int curpageid = ts_->currentPageId();
	mDynamicCastGet(
	    uiWellLogDispProperties*, curwelllogproperty,propflds_[curpageid]);
	if ( curwelllogproperty )
	   propflds_[idx]->curwelllogproperty_ =  curwelllogproperty;
	else
	   propflds_[idx]->curwelllogproperty_ = 0;
    }
}


void uiWellDispPropDlg::updateLogs()
{
    const Well::LogSet& wls = wd_->logs();
    for ( int idx=0; idx<propflds_.size(); idx++ )
    {
	mDynamicCastGet(
	    uiWellLogDispProperties*, curwelllogproperty,propflds_[idx]);
	if ( curwelllogproperty )
	    curwelllogproperty->setLogSet( &wls );
    }
}


void uiWellDispPropDlg::setWDNotifiers( bool yn )
{
    if ( !wd_ ) return;

    if ( yn )
	mAttachCB( mDispNot, uiWellDispPropDlg::wdChg );
    else
	mDetachCB( mDispNot, uiWellDispPropDlg::wdChg );

    mAttachCB( wd_->markerschanged, uiWellDispPropDlg::markersChgd );
}


void uiWellDispPropDlg::getFromScreen()
{
    for ( int idx=0; idx<propflds_.size(); idx++ )
	propflds_[idx]->getFromScreen();
}


void uiWellDispPropDlg::putToScreen()
{
    for ( int idx=0; idx<propflds_.size(); idx++ )
	propflds_[idx]->putToScreen();
}


void uiWellDispPropDlg::markersChgd( CallBacker* )
{
    BufferStringSet markernms;
    wd_->markers().getNames( markernms );
    TypeSet<OD::Color> markercols;
    wd_->markers().getColors( markercols );

    for ( int idx=0; idx<propflds_.size(); idx++ )
    {
	mDynamicCastGet(uiWellMarkersDispProperties*,mrkrfld,propflds_[idx])
	if ( !mrkrfld )
	    continue;

	mrkrfld->setAllMarkerNames( markernms, markercols );
	return;
    }
}


void uiWellDispPropDlg::wdChg( CallBacker* )
{
    NotifyStopper ns( mDispNot );
    putToScreen();
}


void uiWellDispPropDlg::propChg( CallBacker* )
{
    setButtonSensitive( APPLY, true );
    getFromScreen();
    mDispNot.trigger();
}


void uiWellDispPropDlg::applyAllPush( CallBacker* )
{
    getFromScreen();
    applyAllReq.trigger();
}


void uiWellDispPropDlg::applyTabPush( CallBacker* )
{
    getFromScreen();
    applyTabReq.trigger();
}


void uiWellDispPropDlg::resetAllPush( CallBacker* )
{
    resetAllReq.trigger();
    putToScreen();
}


void uiWellDispPropDlg::welldataDelNotify( CallBacker* )
{
    windowClosed.trigger();
    wd_ = 0;
    OBJDISP()->go( this );
}


bool uiWellDispPropDlg::acceptOK( CallBacker* )
{
    return true;
}


void uiWellDispPropDlg::resetCB( CallBacker* )
{
    if ( wd_ )
    {
	Well::MGR().reloadDispPars( wd_->multiID(), is2ddisplay_ );
	putToScreen();
    }
}


bool uiWellDispPropDlg::is2D() const
{
    return is2ddisplay_;
}

uiWellDispPropDlg::TabType uiWellDispPropDlg::currentTab() const
{
    return uiWellDispPropDlg::TabType( ts_->currentPageId() );
}


//uiMultiWellDispPropDlg
uiMultiWellDispPropDlg::uiMultiWellDispPropDlg( uiParent* p,
					const ObjectSet<Well::Data>& wds,
					bool is2ddisplay )
	: uiWellDispPropDlg(p,*wds[0],is2ddisplay)
	, wds_(wds)
	, wellselfld_(0)
{
    if ( wds_.size()>1 )
    {
	BufferStringSet wellnames;
	for ( int idx=0; idx<wds_.size(); idx++ )
	    wellnames.addIfNew( wds_[idx]->name() );

	wellselfld_ = new uiLabeledComboBox( this, tr("Select Well") );
	wellselfld_->box()->addItems( wellnames );
	mAttachCB( wellselfld_->box()->selectionChanged,
		   uiMultiWellDispPropDlg::wellSelChg );
	wellselfld_->attach( hCentered );
	ts_->attach( ensureBelow, wellselfld_ );
    }
    deepRef( wds_ );
}


uiMultiWellDispPropDlg::~uiMultiWellDispPropDlg()
{
    deepUnRef( wds_ );
}


void uiMultiWellDispPropDlg::resetProps( int wellidx, int logidx )
{
    if ( !wds_.validIdx( wellidx ) ) return;
    RefMan<Well::Data> wd = wds_[wellidx];
    Well::DisplayProperties& prop = wd->displayProperties( is2ddisplay_ );
    for ( int idx=0; idx<propflds_.size(); idx++ )
    {
	mDynamicCastGet( uiWellTrackDispProperties*,trckfld,propflds_[idx] );
	mDynamicCastGet( uiWellMarkersDispProperties*,mrkfld,propflds_[idx] );
	mDynamicCastGet( uiWellLogDispProperties*,logfld,propflds_[idx] );
	if ( logfld )
	{
	    if ( !prop.logs_.isEmpty() )
	    {
		if ( idx==LeftLog )
		    logfld->resetProps( prop.logs_[logidx]->left_ );
		else if ( idx==CenterLog )
		    logfld->resetProps( prop.logs_[logidx]->center_ );
		else if ( idx==RightLog )
		    logfld->resetProps( prop.logs_[logidx]->right_ );
	    }
	}
	else if ( trckfld )
	    trckfld->resetProps( prop.track_ );
	else if ( mrkfld )
	{
	    BufferStringSet markernms;
	    TypeSet<OD::Color> markercols;
	    wd->markers().getNames( markernms );
	    wd->markers().getColors( markercols );

	    mrkfld->setAllMarkerNames( markernms, markercols );
	    mrkfld->resetProps( prop.markers_ );
	}
    }
    putToScreen();
}


void uiMultiWellDispPropDlg::wellSelChg( CallBacker* )
{
    const int selidx = wellselfld_ ? wellselfld_->box()->currentItem() : 0;
    uiWellDispPropDlg::setWDNotifiers( false );
    wd_ = wds_[selidx];

    uiWellDispPropDlg::setWDNotifiers( true );
    resetProps( selidx, 0 );
}


void uiMultiWellDispPropDlg::setWDNotifiers( bool yn )
{
    Well::Data* curwd = wd_;
    for ( int idx=0; idx<wds_.size(); idx++ )
    {
	wd_ = wds_[idx];
	if ( yn )
	    mAttachCB( mDispNot, uiMultiWellDispPropDlg::wdChg );
	else
	    mDetachCB( mDispNot, uiMultiWellDispPropDlg::wdChg );
    }

    wd_ = curwd;
}


void uiMultiWellDispPropDlg::onClose( CallBacker* )
{
}


void uiMultiWellDispPropDlg::applyTabPush( CallBacker* )
{
    getFromScreen();
    const TabType pageid = currentTab();
    Well::Data* curwd = wd_;
    const Well::DisplayProperties& edprops = curwd->displayProperties();
    for ( int idx=0; idx<wds_.size(); idx++ )
    {
	wd_ = wds_[idx];
	if ( wd_ && wd_!=curwd )
	{
	    Well::DisplayProperties& wdprops = wd_->displayProperties(is2D());
	    switch ( pageid )
	    {
		case uiWellDispPropDlg::Track :
		    wdprops.track_ = edprops.track_;
		    break;
		case uiWellDispPropDlg::Marker :
		    wdprops.markers_ = edprops.markers_;
		    break;
		case uiWellDispPropDlg::LeftLog :
		    wdprops.logs_[0]->left_.setTo(wd_, edprops.logs_[0]->left_);
		    break;
		case uiWellDispPropDlg::CenterLog :
		    wdprops.logs_[0]->center_.setTo(wd_,
						    edprops.logs_[0]->center_);
		    break;
		case uiWellDispPropDlg::RightLog :
		    wdprops.logs_[0]->right_.setTo(wd_,
						   edprops.logs_[0]->right_);
	    }
	    mDispNot.trigger();
	}
    }
    wd_ = curwd;
}


void uiMultiWellDispPropDlg::resetAllPush( CallBacker* )
{
    Well::Data* curwd = wd_;
    for ( int idx=0; idx<wds_.size(); idx++ )
    {
	wd_ = wds_[idx];
	resetCB( nullptr );
	putToScreen();
    }
    wd_ = curwd;
}
