#ifndef uiattribcrossplot_h
#define uiattribcrossplot_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert Bril
 Date:          June 2005
 RCS:           $Id: uiwellattribxplot.h,v 1.3 2008-04-22 16:20:04 cvsbert Exp $
________________________________________________________________________

-*/


#include "uidialog.h"
class IOObj;
class uiListBox;
class uiComboBox;
class uiGenInput;
class DataPointSet;
class BufferStringSet;
class uiPosFilterSetSel;
namespace Attrib { class DescSet; }


class uiWellAttribCrossPlot : public uiDialog
{
public:
			uiWellAttribCrossPlot(uiParent*,const Attrib::DescSet&);
			~uiWellAttribCrossPlot();

    void		setDescSet(const Attrib::DescSet&);

protected:

    const Attrib::DescSet& ads_;
    ObjectSet<IOObj>	wellobjs_;

    uiListBox*		attrsfld_;
    uiListBox*		wellsfld_;
    uiListBox*		logsfld_;
    uiComboBox*		topmarkfld_;
    uiComboBox*		botmarkfld_;
    uiGenInput*		radiusfld_;
    uiGenInput*		abovefld_;
    uiGenInput*		belowfld_;
    uiGenInput*		logresamplfld_;
    uiPosFilterSetSel*		posfiltfld_;

    void		adsChg();
    bool		extractWellData(const BufferStringSet&,
	    				const BufferStringSet&,
					ObjectSet<DataPointSet>&);
    bool		extractAttribData(DataPointSet&);

    void		initWin(CallBacker*);

    bool		acceptOK(CallBacker*);
};


#endif
