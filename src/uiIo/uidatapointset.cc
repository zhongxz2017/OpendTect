/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Feb 2008
 RCS:           $Id: uidatapointset.cc,v 1.8 2008-04-09 12:17:06 cvsbert Exp $
________________________________________________________________________

-*/

#include "uidatapointset.h"
#include "uistatsdisplaywin.h"
#include "uidatapointsetcrossplotwin.h"

#include "datapointset.h"
#include "posvecdataset.h"
#include "posvecdatasettr.h"
#include "datacoldef.h"
#include "ctxtioobj.h"
#include "iopar.h"
#include "ioobj.h"
#include "survinfo.h"
#include "statruncalc.h"
#include "unitofmeasure.h"
#include "keystrs.h"
#include "oddirs.h"
#include "unitofmeasure.h"

#include "uitable.h"
#include "uilabel.h"
#include "uispinbox.h"
#include "uitoolbar.h"
#include "uiioobjsel.h"
#include "uifileinput.h"
#include "uimsg.h"

static const int cNrPosCols = 3;


uiDataPointSet::Setup::Setup( const char* wintitl, bool ismodal )
    : uiDialog::Setup(wintitl?wintitl:"Extracted data","","0.0.0")
    , isconst_(false)
    , allowretrieve_(true)
    , initialmaxnrlines_(4000)
{
    modal_ = ismodal;
}


static CallBackSet& creationCBS()
{
    static CallBackSet* cbs = 0;
    if ( !cbs ) cbs = new CallBackSet;
    return *cbs;
}


void uiDataPointSet::createNotify( const CallBack& cb )
{ creationCBS() += cb; }
void uiDataPointSet::stopCreateNotify( CallBacker* c )
{ creationCBS().removeWith( c ); }


#define mDPM DPM(DataPackMgr::PointID)

uiDataPointSet::uiDataPointSet( uiParent* p, const DataPointSet& dps,
				const uiDataPointSet::Setup& su )
	: uiDialog(p,su)
	, dps_(*const_cast<DataPointSet*>(&dps))
	, orgdps_(*new DataPointSet(dps))
    	, setup_(su)
    	, zfac_(SI().zFactor())
    	, zunitnm_(SI().getZUnit(false))
	, tbl_(0)
    	, unsavedchgs_(false)
    	, fillingtable_(true)
    	, valueChanged(this)
{
    mDPM.obtain( dps_.id() );
    setCtrlStyle( LeaveOnly );
    runcalcs_.allowNull( true );

    const int nrcols = initVars();
    mkToolBars();

    uiLabel* titllbl = new uiLabel( this, dps.name() );
    titllbl->attach( hCentered );

    tbl_ = new uiTable( this, uiTable::Setup(size(),nrcols)
			  .rowgrow( false ).colgrow( false )
			  .selmode( uiTable::Multi )
			  .manualresize( true ) );
    tbl_->attach( ensureBelow, titllbl );
    tbl_->valueChanged.notify( mCB(this,uiDataPointSet,valChg) );

    setPrefWidth( 800 ); setPrefHeight( 600 );
    eachrow_ = -1; // force refill
    finaliseDone.notify( mCB(this,uiDataPointSet,eachChg) );
    creationCBS().doCall( this );
}


int uiDataPointSet::initVars()
{
    sortcol_ = statscol_ = xcol_ = ycol_ = y2col_ = -1;
    xplotwin_ = 0; statswin_ = 0;

    deepErase( runcalcs_ );
    const int nrcols = dps_.nrCols() + cNrPosCols;
    for ( int idx=0; idx<nrcols; idx++ )
	runcalcs_ += 0;

    eachrow_ = dps_.size() / setup_.initialmaxnrlines_;
    if ( eachrow_ < 1 ) eachrow_ = 1;

    calcIdxs();
    return nrcols;
}


uiDataPointSet::~uiDataPointSet()
{
}


void uiDataPointSet::mkToolBars()
{
    iotb_ = new uiToolBar( this, "I/O Tool bar" );
#define mAddButton(fnm,func,tip) \
    iotb_->addButton( fnm, mCB(this,uiDataPointSet,func), tip )
    mAddButton( "saveset.png", save, "Save data" );
    if ( setup_.allowretrieve_ )
	mAddButton( "openset.png", retrieve, "Retrieve stored data" );
#undef mAddButton

    maniptb_ = new uiToolBar( this, "Manip Tool bar" );
#define mAddButton(fnm,func,tip) \
    maniptb_->addButton( fnm, mCB(this,uiDataPointSet,func), tip )
    mAddButton( "axis-x.png", selXCol, "Set data for X" );
    mAddButton( "axis-add-y.png", selYCol, "Select as Y data" );
    mAddButton( "axis-rm-y.png", unSelCol, "UnSelect as Y data" );
    mAddButton( "delselrows.png", delSelRows, "Remove selected columns" );
    mAddButton( "axis-prev.png", colStepL, "Set Y one column left" );
    mAddButton( "axis-next.png", colStepR, "Set Y one column right" );
    mAddButton( "sortcol.png", setSortCol, "Set sorted column to current" );
#undef mAddButton

    disptb_ = new uiToolBar( this, "Display Tool bar" );

    uiGroup* grp = new uiGroup( disptb_, "Each grp" );
    eachfld_ = new uiSpinBox( grp, 0, "Each" );
    eachfld_->setValue( eachrow_ );
    eachfld_->setInterval( 1, mUdf(int), 1 );
    eachfld_->valueChanged.notify( mCB(this,uiDataPointSet,eachChg) );
    new uiLabel( grp, "Display each", eachfld_ );
    disptb_->addObject( grp->attachObj() );

#define mAddButton(fnm,func,tip,istogg) \
    disptb_->addButton( fnm, mCB(this,uiDataPointSet,func), tip, istogg )
    dispxytbid_ = mAddButton( "toggxy.png", toggleXYZ,
			      "Toggle show X and Y columns", true );
    dispztbid_ = mAddButton( "toggz.png", toggleXYZ,
			     "Toggle show Z column", true );
    xplottbid_ = mAddButton( "xplot.png", showCrossPlot,
	    		     "Show crossplot", false );
    mAddButton( "statsinfo.png", showStatsWin,
			     "Show histogram and stats for column", false );

    disptb_->turnOn( dispxytbid_, true ); disptb_->turnOn( dispztbid_, true );
}


void uiDataPointSet::updColNames()
{
    const int nrcols = tbl_->nrCols();
    const TColID zcid = 2;
    for ( TColID tid=0; tid<nrcols; tid++ )
    {
	BufferString axnm;
	if ( tid == xcol_ )
	    axnm += "X";
	else if ( tid == ycol_ )
	    axnm += "Y";
	else if ( tid == y2col_ )
	    axnm += "Y2";

	BufferString colnm( tid == sortcol_ ? "*" : "" );;
	if ( !axnm.isEmpty() )
	    { colnm += "["; colnm += axnm; colnm += "]"; }

	if ( tid == zcid )
	    colnm += BufferString("Z (",zunitnm_,")");
	else
	    colnm += userName( dColID(tid) );
	tbl_->setColumnLabel( tid, colnm );
    }
}


void uiDataPointSet::calcIdxs()
{
    const int orgtblsz = drowids_.size();
    drowids_.erase(); trowids_.erase(); sortidxs_.erase();

    const int dpssz = dps_.size();
    for ( int did=0; did<dpssz; did++ )
    {
	const bool inact = dps_.isInactive(did);
	const bool hidden = did % eachrow_;
	if ( inact || hidden )
	    trowids_ += -1;
	else
	{
	    const TRowID tid = drowids_.size();
	    sortidxs_ += tid;
	    trowids_ += tid;
	    drowids_ += did;
	}
    }

    revsortidxs_ = sortidxs_;
    if ( sortcol_ >= 0 )
	calcSortIdxs();

    const int newtblsz = drowids_.size();
    if ( tbl_ && newtblsz != orgtblsz )
	tbl_->setNrRows( newtblsz );
}


void uiDataPointSet::calcSortIdxs()
{
    const DColID dcid = dColID( sortcol_ );
    TypeSet<float> vals;
    for ( int disprid=0; disprid<sortidxs_.size(); disprid++ )
    {
	const DRowID drid = drowids_[ disprid ];
	const float val = getVal( dcid, drid, false );
	vals += val;
    }

    sort_coupled( vals.arr(), sortidxs_.arr(), sortidxs_.size() );
    for ( int idx=0; idx<sortidxs_.size(); idx++ )
	revsortidxs_[ sortidxs_[idx] ] = idx;
}


uiDataPointSet::DRowID uiDataPointSet::dRowID( TRowID tid ) const
{
    if ( tid < -1 ) tid = tbl_->currentRow();
    if ( tid < 0 ) return -1;
    return drowids_[ sortidxs_[tid] ];
}


uiDataPointSet::TRowID uiDataPointSet::tRowID( DRowID did ) const
{
    if ( did < -1 ) return tbl_->currentRow();
    else if ( did < 0 ) return -1;
    const TRowID itabrow = trowids_[did];
    if ( itabrow < 0 ) return -1;
    return revsortidxs_[ itabrow ];
}


uiDataPointSet::DColID uiDataPointSet::dColID( TColID tid ) const
{
    if ( tid < -1 ) tid = tbl_->currentCol();
    return tid - cNrPosCols;
}


uiDataPointSet::TColID uiDataPointSet::tColID( DColID did ) const
{
    if ( did < -1-cNrPosCols ) return tbl_->currentCol();

    int ret = did + cNrPosCols;
    if ( ret < 0 ) ret = -1;
    return ret;
}


void uiDataPointSet::fillPos( TRowID tid )
{
    fillingtable_ = true;
    const DataPointSet::Pos pos( dps_.pos(dRowID(tid)) );
    RowCol rc( tid, 0 );
    const Coord c( pos.coord() );
    tbl_->setValue( rc, c.x ); rc.c()++;
    tbl_->setValue( rc, c.y ); rc.c()++;
    float fz = zfac_ * pos.z_ * 100;
    int iz = mNINT(fz);
    tbl_->setValue( rc, iz * 0.01 );
    BufferString rownm;
    if ( is2D() )
	rownm += pos.nr_;
    else
	{ rownm += pos.binid_.inl; rownm += "/"; rownm += pos.binid_.crl; }
    tbl_->setRowLabel( tid, rownm );

    fillingtable_ = false;
}


void uiDataPointSet::fillData( TRowID tid )
{
    RowCol rc( tid, cNrPosCols );
    const DRowID drid = dRowID(tid);
    fillingtable_ = true;
    for ( DColID dcid=0; dcid<dps_.nrCols(); dcid++ )
	{ tbl_->setValue( rc, getVal(dcid,drid,true) ); rc.c()++; }
    fillingtable_ = false;
}


void uiDataPointSet::handleAxisColChg()
{
    updColNames();
    if ( xplotwin_ )
	xplotwin_->plotter().setCols( dColID(xcol_), dColID(ycol_),
					dColID(y2col_) );
    if ( ycol_ >= 0 && statswin_ )
	showStats( dColID(ycol_) );
}


void uiDataPointSet::selXCol( CallBacker* )
{
    const TColID tid = tColID(); if ( tid < 0 ) return;
    if ( xcol_ != tid )
    {
	xcol_ = tid;
	handleAxisColChg();
    }
}


void uiDataPointSet::selYCol( CallBacker* )
{
    const TColID tid = tColID(); if ( tid < 0 ) return;

    const TColID prevy = ycol_; const TColID prevy2 = y2col_;
    if ( ycol_ == -1 )
	ycol_ = tid;
    else
	y2col_ = tid;

    if ( prevy != ycol_ || prevy2 != y2col_ )
	handleAxisColChg();
}


void uiDataPointSet::unSelCol( CallBacker* )
{
    const TColID tid = tColID(); if ( tid < 0 ) return;

    if ( tid == ycol_ )
	{ ycol_ = y2col_; y2col_ = -1; }
    else if ( tid == y2col_ )
	y2col_ = -1;
    else
	return;

    handleAxisColChg();
}


void uiDataPointSet::colStepL( CallBacker* )
{
    ycol_ = ycol_ < 1 ? tbl_->nrCols()-1 : ycol_ - 1;
    if ( ycol_ == 2 && !isDisp(false) )
	ycol_--;

    handleAxisColChg();
}


void uiDataPointSet::colStepR( CallBacker* )
{
    ycol_ = ycol_ >= tbl_->nrCols()-1 ? 0 : ycol_ + 1;
    if ( ycol_ == 2 && !isDisp(false) )
	ycol_++;
    if ( ycol_ >= tbl_->nrCols() )
	ycol_ = 0;

    handleAxisColChg();
}


void uiDataPointSet::rowSel( CallBacker* cb )
{
    mCBCapsuleUnpack(int,trid,cb);
    selrows_ += trid;
    handleSelRows();
    setStatsMarker( dRowID(trid) );
}


void uiDataPointSet::handleSelRows()
{
    bool havechgs = false;
    for ( int idx=0; idx<selrows_.size(); idx++ )
    {
	const TRowID trid = selrows_[idx];
	const DRowID drid = dRowID( trid );
	const bool dpssel = dps_.isSelected( drid );
	const bool tblsel = tbl_->isRowSelected( trid );
	if ( dpssel != tblsel )
	{
	    havechgs = true;
	    dps_.setSelected( drid, tblsel );
	}
    }
    if ( havechgs && xplotwin_ )
	xplotwin_->plotter().update();
}


bool uiDataPointSet::isDisp( bool xy ) const
{
    return disptb_->isOn( xy ? dispxytbid_ : dispztbid_ );
}


void uiDataPointSet::toggleXYZ( CallBacker* )
{
    const bool havexy = disptb_->isOn( dispxytbid_ );
    const bool havez = disptb_->isOn( dispztbid_ );
    tbl_->hideColumn( 0, !havexy );
    tbl_->hideColumn( 1, !havexy );
    tbl_->hideColumn( 2, !havez );
    redoAll();
}


void uiDataPointSet::showCrossPlot( CallBacker* )
{
    if ( !xplotwin_ )
    {
	xplotwin_ = new uiDataPointSetCrossPlotWin( *this );
	uiDataPointSetCrossPlotter& xpl = xplotwin_->plotter();
	xpl.selectionChanged.notify( mCB(this,uiDataPointSet,xplotSelChg) );
	xpl.removeRequest.notify( mCB(this,uiDataPointSet,xplotRemReq) );
	xplotwin_->windowClosed.notify( mCB(this,uiDataPointSet,xplotClose) );
    }

    disptb_->setSensitive( xplottbid_, false );
    handleAxisColChg();
    xplotwin_->show();
}


void uiDataPointSet::getXplotPos( uiDataPointSet::DColID& dcid,
				  uiDataPointSet::DRowID& drid ) const
{
    drid = -1; dcid = -cNrPosCols-1;
    if ( !xplotwin_ ) return;
    const uiDataPointSetCrossPlotter& xpl = xplotwin_->plotter();
    drid = xpl.selRow();
    TColID tcid = xpl.isY2() ? y2col_ : ycol_;
    if ( tcid >= 0 )
	dcid = dColID( tcid );
}


void uiDataPointSet::setCurrent( uiDataPointSet::DColID dcid,
				 uiDataPointSet::DRowID drid )
{
    RowCol rc( tRowID(drid), tColID(dcid) );
    if ( rc.c() >= 0 && rc.r() >= 0 )
	tbl_->setCurrentCell( rc );
}


void uiDataPointSet::setCurrent( const DataPointSet::Pos& pos,
				 uiDataPointSet::DColID dcid )
{
    setCurrent( dps_.find(pos), dcid );
}


void uiDataPointSet::xplotSelChg( CallBacker* )
{
    int dcid, drid; getXplotPos( dcid, drid );
    if ( drid < 0 || dcid < -cNrPosCols ) return;

    setCurrent( dcid, drid );
    setStatsMarker( drid );
}


void uiDataPointSet::setStatsMarker( uiDataPointSet::DRowID drid )
{
    if ( !statswin_ || statscol_ < 0 ) return;

    const float val = getVal( dColID(statscol_), drid, false );
    statswin_->setMarkValue( val, true );
}


void uiDataPointSet::xplotRemReq( CallBacker* )
{
    int drid, dcid; getXplotPos( dcid, drid );
    if ( drid < 0 ) return;
    dps_.setInactive( drid, true );
    const TRowID trid = tRowID( drid );
    if ( trid >= 0 )
	redoAll();
}


void uiDataPointSet::redoAll()
{
    calcIdxs();

    const int nrrows = tbl_->nrRows();
    for ( TRowID tid=0; tid<nrrows; tid++ )
    {
	fillPos( tid );
	fillData( tid );
    }

    updColNames();

    if ( statswin_ )
	showStats( dColID(statscol_) );
    if ( xplotwin_ )
	showCrossPlot( 0 );
}


void uiDataPointSet::xplotClose( CallBacker* )
{
    delete xplotwin_; xplotwin_ = 0;
    disptb_->setSensitive( xplottbid_, true );
}


void uiDataPointSet::statsClose( CallBacker* )
{
    delete statswin_; statswin_ = 0;
}


const char* uiDataPointSet::userName( uiDataPointSet::DColID did ) const
{
    if ( did >= 0 )
	return dps_.colName( did );
    else if ( did == -1 )
	return "Z";
    else
	return did == -3 ? "X-Coord" : "Y-Coord";
}


Stats::RunCalc<float>& uiDataPointSet::getRunCalc(
				uiDataPointSet::DColID dcid ) const
{
    static Stats::RunCalc<float> empty( Stats::RunCalcSetup(false) );
    if ( dcid < -cNrPosCols ) return empty;

    int rcidx = dcid;
    if ( rcidx < 0 )
	rcidx = dps_.nrCols() - 1 - dcid;
    Stats::RunCalc<float>* rc = runcalcs_[rcidx];
    if ( !rc )
    {
	Stats::RunCalcSetup su( false );
#	define mReq(typ) require(Stats::typ)
	su.mReq(Count).mReq(Average).mReq(Median).mReq(StdDev);
	rc = new Stats::RunCalc<float>( su.mReq(Min).mReq(Max).mReq(RMS) );
	for ( DRowID drid=0; drid<dps_.size(); drid++ )
	    rc->addValue( getVal( dcid, drid, true ) );
	runcalcs_.replace( rcidx, rc );
    }

    return *rc;


}


void uiDataPointSet::showStatsWin( CallBacker* )
{
    const DColID dcid = dColID();
    showStats( dcid );
}


void uiDataPointSet::showStats( uiDataPointSet::DColID dcid )
{
    int newcol = tColID( dcid );
    if ( newcol < 0 ) return;
    statscol_ = newcol;

    BufferString txt( "Column: " );
    txt += userName( dcid );
    if ( statscol_ >= 0 )
    {
	const DataColDef& dcd = dps_.colDef( dcid );
	if ( dcd.unit_ )
	    { txt += " ("; txt += dcd.unit_->name(); txt += ")"; }
	if ( dcd.ref_ != dcd.name_ )
	    { txt += "\n"; txt += dcd.ref_; }
    }

    const Stats::RunCalc<float>& rc = getRunCalc( dcid );
    if ( !statswin_ )
    {
	statswin_ = new uiStatsDisplayWin( this, uiStatsDisplay::Setup() );
	statswin_->windowClosed.notify( mCB(this,uiDataPointSet,statsClose) );
    }
    statswin_->setData( rc );
    statswin_->setDataName( txt );
    statswin_->show();
}


float uiDataPointSet::getVal( DColID dcid, DRowID drid, bool foruser ) const
{
    if ( dcid >= 0 )
    {
	const float val = dps_.value( dcid, drid );
	if ( !foruser ) return val;
	const UnitOfMeasure* mu = dps_.colDef( dcid ).unit_;
	return mu ? mu->userValue(val) : val;
    }
    else if ( dcid == -1 )
    {
	const float val = dps_.z( drid );
	if ( !foruser ) return val;
	return val * zfac_;
    }

    return dcid == -3 ? dps_.coord(drid).x : dps_.coord(drid).y;
}


#define mRetErr fillPos( TRowID(drid) ); return

void uiDataPointSet::valChg( CallBacker* )
{
    if ( fillingtable_ ) return;

    const RowCol& cell = tbl_->notifiedCell();
    if ( cell.row < 0 || cell.col < 0 ) return;

    const DColID dcid( dColID(cell.col) );
    const DRowID drid( dRowID(cell.row) );

    afterchgdr_ = beforechgdr_ = dps_.dataRow( drid );
    bool poschgd = false;

    if ( dcid >= 0 )
    {
	float val = tbl_->getfValue( cell );
	const UnitOfMeasure* mu = dps_.colDef( dcid ).unit_;
	afterchgdr_.data_[dcid] = mu ? mu->internalValue(val) : val;
    }
    else
    {
	const char* txt = tbl_->text( cell );
	if ( !txt || !*txt )
	{
	    uiMSG().error( "Positioning values cannot be undefined" );
	    mRetErr;
	}
	DataPointSet::Pos& pos( afterchgdr_.pos_ );

	if ( dcid == -1 )
	{
	    if ( !isDisp(false) ) { pErrMsg("Huh"); mRetErr; }
	    pos.z_ = tbl_->getfValue( cell ) / zfac_;
	    poschgd = !mIsZero(pos.z_-beforechgdr_.pos_.z_,1e-6);
	}
	else
	{
	    if ( !isDisp(true) ) { pErrMsg("Huh"); mRetErr; }
	    Coord crd( pos.coord() );
	    (dcid == -cNrPosCols ? crd.x : crd.y) = tbl_->getValue( cell );
	    pos.set( SI().transform(crd), crd );
	    poschgd = pos.binid_ != beforechgdr_.pos_.binid_;
	}
    }

    if ( poschgd )
	dps_.bivSet().remove( dps_.bvsPos(drid) );

    if ( dps_.setRow(afterchgdr_) )
    {
	dps_.dataChanged();
	redoAll(); setCurrent( afterchgdr_.pos_, dcid );
    }

    unsavedchgs_ = true;
    valueChanged.trigger();
}


void uiDataPointSet::eachChg( CallBacker* )
{
    int neweachrow = eachfld_->getValue();
    if ( neweachrow < 1 ) neweachrow = 1;
    if ( neweachrow != eachrow_ )
    {
	eachrow_ = neweachrow;
	redoAll();
	setCurrent( 0, 0 );
    }
}


void uiDataPointSet::setSortCol( CallBacker* )
{
    TColID tid = tColID();
    if ( sortcol_ != tid )
    {
	sortcol_ = tColID();
	DRowID drid = dRowID();
	redoAll();
	setCurrent( dColID(sortcol_), drid );
    }
}


bool uiDataPointSet::is2D() const
{
    return dps_.is2D();
}


bool uiDataPointSet::saveOK()
{
    if ( !unsavedchgs_ )
	return true;

    int res = uiMSG().askGoOnAfter( "There are unsaved changes.\n"
				    "Do you want to save the data?" );
    if ( res == 2 )
	return false;
    else if ( res == 1 )
	return true;

    return doSave();
}


bool uiDataPointSet::rejectOK( CallBacker* )
{
    if ( !saveOK() )
	return false;
    dps_ = orgdps_;
    return acceptOK( 0 );
}


bool uiDataPointSet::acceptOK( CallBacker* )
{
    mDPM.release( dps_.id() );
    delete xplotwin_; delete statswin_;
    return true;
}


void uiDataPointSet::retrieve( CallBacker* )
{
    if ( !saveOK() ) return;

    CtxtIOObj ctio( PosVecDataSetTranslatorGroup::ioContext() );
    ctio.ctxt.forread = true;
    uiIOObjSelDlg seldlg( this, ctio );
    if ( !seldlg.go() || !seldlg.ioObj() ) return;

    PosVecDataSet pvds;
    BufferString errmsg;
    bool rv = pvds.getFrom(seldlg.ioObj()->fullUserExpr(true),errmsg);
    if ( !rv )
	{ uiMSG().error( errmsg ); return; }
    if ( pvds.data().isEmpty() )
	{ uiMSG().error("Selected data set is empty"); return; }
    DataPointSet* newdps = new DataPointSet( pvds, dps_.is2D(),
	    				     dps_.isMinimal() );
    if ( newdps->isEmpty() )
	{ delete newdps; uiMSG().error("Data set is not suitable"); return; }

    rejectOK( 0 );

    TypeSet<int> cols;
    for ( int idx=0; idx<tbl_->nrCols(); idx++ )
	cols += idx;
    tbl_->removeColumns( cols );

    const int nrcols = initVars();
    tbl_->setNrRows( size() );
    tbl_->setNrCols( nrcols );
    eachfld_->setValue( eachrow_ );

    redoAll();
}


class uiDataPointSetSave : public uiDialog
{
public:

uiDataPointSetSave( uiParent* p )
    : uiDialog(p,uiDialog::Setup("Create output","Specify output","0.0.0"))
    , ctio_(PosVecDataSetTranslatorGroup::ioContext())
{
    ctio_.ctxt.forread = false;
    const CallBack tccb( mCB(this,uiDataPointSetSave,outTypChg) );

    tabfld_ = new uiGenInput( this, "Output to",
	    		BoolInpSpec(true,"Text file","OpendTect object") );
    tabfld_->valuechanged.notify( tccb );
    uiFileInput::Setup su;
    su.defseldir(GetDataDir()).forread(false).filter("*.txt");
    txtfld_ = new uiFileInput( this, "Output file", su );
    txtfld_->attach( alignedBelow, tabfld_ );
    selgrp_ = new uiIOObjSelGrp( this, ctio_ );
    selgrp_->attach( alignedBelow, tabfld_ );

    finaliseDone.notify( tccb );
}

~uiDataPointSetSave()
{
    delete ctio_.ioobj;
}

void outTypChg( CallBacker* )
{
    istab_ = tabfld_->getBoolValue();
    txtfld_->display( istab_ );
    selgrp_->display( !istab_ );
}

#define mErrRet(s) { uiMSG().error(s); return false; }
bool acceptOK( CallBacker* )
{
    istab_ = tabfld_->getBoolValue();
    if ( istab_ )
    {
	fname_ = txtfld_->fileName();
	if ( fname_.isEmpty() )
	    mErrRet("Please select the output file name")
    }
    else
    {
	if ( !selgrp_->processInput() )
	    mErrRet("Please enter a name for the output")
	fname_ = selgrp_->getCtxtIOObj().ioobj->fullUserExpr(false);
    }

    return true;
}

    CtxtIOObj		ctio_;
    BufferString	fname_;
    uiGenInput*		tabfld_;
    uiFileInput*	txtfld_;
    uiIOObjSelGrp*	selgrp_;
    bool		istab_;
};


void uiDataPointSet::save( CallBacker* )
{
    doSave();
}


bool uiDataPointSet::doSave()
{
    if ( dps_.nrActive() < 1 ) return true;

    uiDataPointSetSave uidpss( this );
    if ( !uidpss.go() ) return false;

    dps_.dataSet().pars() = storepars_;
    BufferString errmsg;
    if ( !dps_.dataSet().putTo(uidpss.fname_,errmsg,uidpss.istab_) )
	{ uiMSG().error( errmsg ); return false; }

    unsavedchgs_ = false;
    return true;
}


void uiDataPointSet::delSelRows( CallBacker* )
{
    int nrrem = 0;
    for ( int irow=0; irow<tbl_->nrRows(); irow++ )
    {
	if ( tbl_->isRowSelected(irow) )
	{
	    nrrem++;
	    dps_.setInactive( dRowID(irow), true );
	}
    }
    if ( nrrem > 0 )
	{ redoAll(); return; }

    uiMSG().message( "Please select the row(s) you want to remove."
		     "\nby clicking on the row label(s)."
		     "\nYou can select multiple rows by dragging,"
		     "\nor by holding down the shift key when clicking." );
}
