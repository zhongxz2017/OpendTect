
#ifndef uiseislinesel_h
#define uiseislinesel_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		Nov 2008
 RCS:		$Id: uiseislinesel.h,v 1.9 2008-12-11 08:44:06 cvsumesh Exp $
________________________________________________________________________

-*/

#include "uicompoundparsel.h"
#include "uidialog.h"
#include "bufstring.h"
#include "bufstringset.h"
#include "ranges.h"

class uiLabeledSpinBox;
class uiListBox;
class uiSeisSel;
class uiSpinBox;

class CtxtIOObj;
class IOObj;


class uiSeis2DLineSubSel : public uiDialog
{
public:
    					uiSeis2DLineSubSel(uiParent*,
						           CtxtIOObj& lsctio);
					~uiSeis2DLineSubSel()	{}

    BufferString			getSummary() const;

    void				setSelLines(const BufferStringSet&);
    const BufferStringSet&		getSelLines() const { return sellines_;}
    const Interval<int>			getLineTrcRange(int idx) const;

protected:

    int					nroflines_;				
    BufferStringSet 			sellines_;
    uiSeisSel*  			linesetfld_;
    uiListBox*  			lnmsfld_;
    uiLabeledSpinBox*          	        lsb_;
    uiSpinBox*                          trc0fld_;
    uiSpinBox*                  	trc1fld_;
    CtxtIOObj&				lsctio_;

    TypeSet< Interval<int> > 		linetrcrgs_;
    TypeSet< Interval<int> >		linetrcflrgs_;

    void 				lineSetSel(CallBacker*);
    void 				lineSel(CallBacker*);
    void				trc0Changed(CallBacker*);
    void				trc1Changed(CallBacker*);

    virtual bool        		acceptOK(CallBacker*);
};


class uiSelection2DParSel : public uiCompoundParSel
{
public:
    				uiSelection2DParSel(uiParent*);
				~uiSelection2DParSel();

    BufferString                getSummary() const;
    void                        doDlg(CallBacker*);
    IOObj*                      getIOObj();
    const BufferStringSet&      getSelLines() const     { return sellines_; }
    void 			fillPar(IOPar &);

protected:

    BufferStringSet             sellines_;
    CtxtIOObj*                  lsctio_;
    uiSeis2DLineSubSel*         linesel_;
};   

#endif
