#ifndef uiseisrandto2dline_h
#define uiseisrandto2dline_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Raman Singh
 Date:		May 2008
 RCS:		$Id$
________________________________________________________________________

-*/

#include "uiseismod.h"
#include "uidialog.h"
#include "uigroup.h"

class IOObj;
class uiGenInput;
class uiIOObjSel;
class uiSeisSel;

namespace Geometry { class RandomLine; class RandomLineSet; }

mExpClass(uiSeis) uiSeisRandTo2DBase : public uiGroup
{
public:
    			uiSeisRandTo2DBase(uiParent*,bool rdlsel);
			~uiSeisRandTo2DBase();

    const IOObj*	getInputIOObj() const;
    const IOObj*	getOutputIOObj() const;

    bool		getRandomLineGeom(Geometry::RandomLineSet&) const;

    virtual bool	checkInputs();

    Notifier<uiSeisRandTo2DBase> change;

protected:

    uiIOObjSel*		rdlfld_;
    uiSeisSel*		inpfld_;
    uiSeisSel*          outpfld_;

    void		selCB(CallBacker*);

};


mExpClass(uiSeis) uiSeisRandTo2DLineDlg : public uiDialog
{
public:
    			uiSeisRandTo2DLineDlg(uiParent*,
					      const Geometry::RandomLine*);

protected:

    uiSeisRandTo2DBase*	basegrp_;
    uiGenInput*		linenmfld_;
    uiGenInput*		trcnrfld_;
    const Geometry::RandomLine*	rdlgeom_;

    bool		acceptOK(CallBacker*);
};

#endif

