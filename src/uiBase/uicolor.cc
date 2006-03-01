/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink / Bril
 Date:          22/05/2000
 RCS:           $Id: uicolor.cc,v 1.16 2006-03-01 13:45:46 cvsbert Exp $
________________________________________________________________________

-*/

#include "uicolor.h"
#include "uibutton.h"
#include "uibody.h"
#include "uilabel.h"
#include "pixmap.h"
#include "uiparentbody.h"

#ifdef __mac__
# define _machack_
# define private public // need access to some private stuff in qcolordialog
#endif
#include "qcolordialog.h"

#ifdef _machack_

static QRgb mygetRgba( QRgb initial, bool *ok,
                            QWidget *parent, const char* name )
{
    int allocContext = QColor::enterAllocContext();
    QColorDialog *dlg = new QColorDialog( parent, name, TRUE );  //modal

    Q_CHECK_PTR( dlg );
#ifndef QT_NO_WIDGET_TOPEXTRA
    dlg->setCaption( QColorDialog::tr( "Select color" ) );
#endif
    dlg->setColor( initial );
    dlg->selectColor( initial );
    dlg->setSelectedAlpha( qAlpha(initial) );
    int resultCode = dlg->exec();
    QColor::leaveAllocContext();
    QRgb result = initial;
    if ( resultCode == QDialog::Accepted ) {
        QRgb c = dlg->color().rgb();
        int alpha = dlg->selectedAlpha();
        result = qRgba( qRed(c), qGreen(c), qBlue(c), alpha );
    }
    if ( ok )
        *ok = resultCode == QDialog::Accepted;

    QColor::destroyAllocContext(allocContext);
    delete dlg;
    return result;
}


#endif

bool selectColor( Color& col, uiParent* parnt, const char* nm, bool withtransp )
{
  
    bool ok;
    QRgb rgb;

    if ( withtransp )
    {
#ifdef _machack_
	rgb = mygetRgba( (QRgb) col.rgb(), &ok, 
		      parnt ? parnt->pbody()->qwidget() : 0, nm );
#else
	rgb = QColorDialog::getRgba( (QRgb) col.rgb(), &ok, 
		      parnt ? parnt->pbody()->qwidget() : 0, nm );
#endif
    }
    else
    {
	QColor newcol = QColorDialog::getColor( QColor((QRgb) col.rgb()) , 
		      parnt ? parnt->pbody()->qwidget() : 0, nm );

	ok = newcol.isValid();
	rgb = newcol.rgb(); 
    }

    if ( ok ) col.setRgb( rgb );

    return ok;
}


uiColorInput::uiColorInput( uiParent* p, const Color& col,
			    const char* lbltxt, const char* dlgtxt )
	: uiGroup(p,"Color input")
	, dlgtxt_(dlgtxt)
	, colorchanged(this)
	, withalpha_(false)
{
    colbut_ = new uiPushButton( this, "", false );
    colbut_->activated.notify( mCB(this,uiColorInput,selCol) );
    
    if ( lbltxt && *lbltxt )
	new uiLabel( this, lbltxt, colbut_ );

    setColor( col ); 
    setHAlignObj( colbut_ );
}


void uiColorInput::selCol( CallBacker* )
{
    Color newcol = color_;
    const Color oldcol = color_;
    selectColor( newcol, this, dlgtxt_, withalpha_ );
    if ( oldcol != newcol )
    {
	setColor( newcol );
	colorchanged.trigger();
    }
}


void uiColorInput::setColor( const Color& col )
{
    color_ = col;

    ioPixmap pm( colbut_->prefHNrPics()-10, colbut_->prefVNrPics()-10 );
    pm.fill( color_ );
    colbut_->setPixmap( pm );
}
