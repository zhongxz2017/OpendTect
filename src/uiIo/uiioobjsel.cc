/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        Bert Bril
 Date:          25/05/2000
 RCS:           $Id: uiioobjsel.cc,v 1.45 2003-03-06 15:08:56 arend Exp $
________________________________________________________________________

-*/

#include "uiioobjsel.h"
#include "iodirentry.h"
#include "uigeninput.h"
#include "uilistbox.h"
#include "uimsg.h"
#include "uimenu.h"
#include "ioman.h"
#include "ioobj.h"
#include "iolink.h"
#include "iodir.h"
#include "iopar.h"
#include "transl.h"
#include "ptrman.h"
#include "filegen.h"
#include "errh.h"


static IOObj* mkEntry( const CtxtIOObj& ctio, const char* nm )
{
    CtxtIOObj newctio( ctio );
    newctio.ioobj = 0; newctio.setName( nm );
    newctio.fillObj();
    return newctio.ioobj;
}


uiIOObjSelDlg::uiIOObjSelDlg( uiParent* p, const CtxtIOObj& c,
			      const char* seltxt, bool multisel )
	: uiIOObjRetDlg(p,c.ctxt.forread?"Input selection":"Output selection")
	, ctio(c)
	, nmfld(0)
	, ioobj(0)
	, ismultisel(multisel && ctio.ctxt.forread)
{
    BufferString nm( "Select " );
    nm += ctio.ctxt.forread ? "input " : "output ";
    nm += ctio.ctxt.trgroup->name();
    if ( ismultisel ) nm += "(s)";
    setTitleText( nm );

    IOM().to( MultiID(IOObjContext::getStdDirData(ctio.ctxt.stdseltype)->id) );
    entrylist = new IODirEntryList( IOM().dirPtr(), ctio.ctxt );
    if ( ctio.ioobj )
        entrylist->setSelected( ctio.ioobj->key() );

    entrylist->setName( seltxt );
    listfld = new uiLabeledListBox( this, entrylist->Ptr() );
    if ( ismultisel )
	listfld->box()->setMultiSelect( true );
    listfld->box()->setPrefWidthInChar( 
		listfld->box()->optimumFieldWidth(20,60) );

    if ( !ctio.ctxt.forread )
    {
	nmfld = new uiGenInput( this, "Name" );
	nmfld->attach( alignedBelow, listfld );
	nmfld->setStretch( 2, 0 );
    }

    listfld->box()->selectionChanged.notify( mCB(this,uiIOObjSelDlg,selChg) );
    listfld->box()->rightButtonClicked.notify(mCB(this,uiIOObjSelDlg,rightClk));
    listfld->box()->doubleClicked.notify( mCB(this,uiDialog,accept) );

    setOkText( "Select" );
    selChg( this );
}


uiIOObjSelDlg::~uiIOObjSelDlg()
{
    delete entrylist;
}


int uiIOObjSelDlg::nrSel() const
{
    if ( !ismultisel )
	return ioobj ? 1 : 0;

    int nr = 0;
    for ( int idx=0; idx<listfld->box()->size(); idx++ )
	if ( listfld->box()->isSelected(idx) ) nr++;
    return nr;
}


const IOObj* uiIOObjSelDlg::selected( int objnr ) const
{
    const int nrsel = nrSel();
    if ( nrsel < 2 || objnr < 1 ) return ioobj;
    if ( objnr >= nrsel ) return 0;

    for ( int idx=0; idx<listfld->box()->size(); idx++ )
    {
	if ( listfld->box()->isSelected(idx) )
	    objnr--;
	if ( objnr < 0 )
	{
	    entrylist->setCurrent( idx );
	    return entrylist->selected();
	}
    }
    BufferString msg( "Should not reach. objnr=" );
    msg += objnr; msg += " nrsel="; msg += nrsel;
    pErrMsg( msg );
    return 0;
}


void uiIOObjSelDlg::selChg( CallBacker* c )
{
    if ( ismultisel ) return;

    entrylist->setCurrent( listfld->box()->currentItem() );
    ioobj = entrylist->selected();
    if ( c && nmfld )
	nmfld->setText( ioobj ? (const char*)ioobj->name() : "" );
}


void uiIOObjSelDlg::rightClk( CallBacker* c )
{
    int prevcur = listfld->box()->currentItem();
    entrylist->setCurrent( listfld->box()->lastClicked() );
    ioobj = entrylist->selected();
    bool chgd = false;
    if ( ioobj )
    {
	PtrMan<Translator> tr = ioobj->getTranslator();
	bool rmabl = tr ? tr->implRemovable(ioobj) : ioobj->implRemovable();
	uiPopupMenu* mnu = new uiPopupMenu( this, "Action" );
	BufferString mnutxt( "Remove" );
	if ( rmabl ) mnutxt += " ...";
	uiMenuItem* rmit = new uiMenuItem( mnutxt );
	mnu->insertItem( rmit );

	int ret = mnu->exec();
	if ( ret != -1 )
	{
	    if ( ret == rmit->id() )
		chgd = rmEntry( tr, rmabl );
	}
    }
    if ( !chgd ) return;

    if ( prevcur >= entrylist->size() ) prevcur--;
    entrylist->setCurrent( prevcur );
    ioobj = entrylist->selected();
    listfld->box()->empty();
    listfld->box()->addItems( entrylist->Ptr() );
}


bool uiIOObjSelDlg::rmEntry( Translator* tr, bool rmabl )
{
    if ( rmabl )
    {
	BufferString mess( "Remove '" );
	if ( !ioobj->isLink() )
	    { mess += ioobj->fullUserExpr(YES); mess += "'?"; }
	else
	{
	    FileNameString fullexpr( ioobj->fullUserExpr(YES) );
	    mess += File_getFileName(fullexpr);
	    mess += "'\n- and everything in it! - ?";
	}
	if ( !uiMSG().askGoOn(mess) )
	    return false;

	bool rmd = tr ? tr->implRemove(ioobj) : ioobj->implRemove();
	if ( !rmd )
	{
	    mess = "Could not remove '";
	    mess += ioobj->fullUserExpr(YES); mess += "'";
	    uiMSG().warning( mess );
	}
    }

    entrylist->curRemoved();
    IOM().permRemove( ioobj->key() );
    entrylist->fill( IOM().dirPtr() );
    return true;
}


void uiIOObjSelDlg::fillPar( IOPar& iopar ) const
{
    iopar.set( "ID", ioobj ? (const char*)ioobj->key() : "" );
}


void uiIOObjSelDlg::usePar( const IOPar& iopar )
{
    const char* res = iopar.find( "ID" );
    if ( res && *res )
    {
	MultiID key( res );
	if ( entrylist->selected() && entrylist->selected()->key() != key )
	{
	    entrylist->setSelected( MultiID(res) );
	    listfld->box()->setCurrentItem( entrylist->selected()->name() );
	    const_cast<uiIOObjSelDlg*>( this )->selChg(0);
	}
    }
}


bool uiIOObjSelDlg::acceptOK( CallBacker* )
{
    selChg( 0 );
    if ( !nmfld )
    {
	if ( ismultisel )
	{
	    for ( int idx=0; idx<listfld->box()->size(); idx++ )
	    {
		if ( listfld->box()->isSelected(idx) )
		{
		    entrylist->setCurrent( idx );
		    ioobj = entrylist->selected();
		    break;
		}
	    }
	    if ( !ioobj )
	    {
		uiMSG().error( "Please select at least one, or press Cancel" );
		return false;
	    }
	}
	mDynamicCastGet(IOLink*,iol,ioobj)
	if ( !ioobj || iol )
	{
	    IOM().to( iol );
	    entrylist->fill( IOM().dirPtr() );
	    listfld->box()->empty();
	    listfld->box()->addItems( entrylist->Ptr() );
	    return false;
	}
	return true;
    }

    const char* seltxt = nmfld->text();
    if ( ioobj && ioobj->name() == seltxt ) return true;

    int selidx = entrylist->indexOf( seltxt );
    if ( selidx >= 0 )
    {
	entrylist->setCurrent( selidx );
	ioobj = entrylist->selected();
	return true;
    }

    return createEntry( seltxt );
}


bool uiIOObjSelDlg::createEntry( const char* seltxt )
{
    ioobj = mkEntry( ctio, seltxt );
    if ( !ioobj )
    {
	uiMSG().error( "Cannot create object with this name" );
	return false;
    }

    return true;
}


void uiIOObjSelDlg::setInitOutputName( const char* nm )
{
    if ( nmfld ) 
	nmfld->setText( nm );
    listfld->box()->setCurrentItem( nm );
}


uiIOObjSel::uiIOObjSel( uiParent* p, CtxtIOObj& c, const char* txt,
			bool wclr, const char* st, const char* buttxt )
	: uiIOSelect( p, mCB(this,uiIOObjSel,doObjSel),
		      txt ? txt : (const char*)c.ctxt.trgroup->name(), 
		      wclr, buttxt )
	, ctio(c)
	, forread(c.ctxt.forread)
	, seltxt(st)
{
    updateInput();
}


uiIOObjSel::~uiIOObjSel()
{
}


bool uiIOObjSel::fillPar( IOPar& iopar ) const
{
    iopar.set( "ID", ctio.ioobj ? ctio.ioobj->key() : MultiID("") );
    return true;
}


void uiIOObjSel::usePar( const IOPar& iopar )
{
    const char* res = iopar.find( "ID" );
    if ( res && *res )
	setInput( res );
}


void uiIOObjSel::updateInput()
{
    if ( ctio.ioobj )
	setInput( (const char*)ctio.ioobj->key() );
}


const char* uiIOObjSel::userNameFromKey( const char* ky ) const
{
    static BufferString nm;
    nm = "";
    if ( ky && *ky )
	nm = IOM().nameOf( ky, false );
    return (const char*)nm;
}


void uiIOObjSel::processInput()
{
    const char* inp = getInput();
    if ( specialitems.findKeyFor(inp) )
    {
	ctio.setObj( 0 );
	return;
    }

    int selidx = getCurrentItem();
    if ( selidx >= 0 )
    {
	const char* itemusrnm = userNameFromKey( getItem(selidx) );
	if ( !strcmp(inp,itemusrnm) && ctio.ioobj ) return;
    }

    IOM().to( ctio.ctxt.stdSelKey() );
    const IOObj* ioobj = (*IOM().dirPtr())[inp];
    ctio.setObj( ioobj && ctio.ctxt.validIOObj(*ioobj) ? ioobj->clone() : 0 );
    updateInput();
}


bool uiIOObjSel::existingTyped() const
{
    const char* inp = getInput();
    IOM().to( ctio.ctxt.stdSelKey() );
    return (*IOM().dirPtr())[inp];
}


bool uiIOObjSel::commitInput( bool mknew )
{
    const char* inp = getInput();
    if ( specialitems.findKeyFor(inp) )
    {
	ctio.setObj( 0 );
	return true;
    }

    processInput();
    if ( existingTyped() )
    {
       if ( ctio.ioobj ) return true;
       BufferString msg( getInput() );
       msg += ": Please enter another name.";
       uiMSG().error( msg );
       return false;
    }
    if ( !mknew ) return false;

    ctio.setObj( createEntry( getInput() ) );
    return ctio.ioobj;
}


void uiIOObjSel::doObjSel( CallBacker* )
{
    ctio.ctxt.forread = forread;
    uiIOObjRetDlg* dlg = mkDlg();
    if ( dlg && dlg->go() && dlg->ioObj() )
    {
	ctio.setObj( dlg->ioObj()->clone() );
	updateInput();
	newSelection( dlg );
	selok_ = true;
    }
    delete dlg;
}


void uiIOObjSel::objSel()
{
    const char* key = getKey();
    if ( specialitems.find(key) )
	ctio.setObj( 0 );
    else
	ctio.setObj( IOM().get(getKey()) );
}


uiIOObjRetDlg* uiIOObjSel::mkDlg()
{
    return new uiIOObjSelDlg( this, ctio, seltxt );
}


IOObj* uiIOObjSel::createEntry( const char* nm )
{
    return mkEntry( ctio, nm );
}
