#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiearthmodelmod.h"
#include "uidialog.h"
#include "emposid.h"
#include "multiid.h"

class BufferStringSet;
class Horizon2DScanner;
class SurfaceInfo;

class uiComboBox;
class uiFileInput;
class uiGenInput;
class uiListBox;
class uiPushButton;
class uiTableImpDataSel;
namespace Table { class FormatDesc; }

/*! \brief Dialog for Horizon Import */

mExpClass(uiEarthModel) uiImportHorizon2D : public uiDialog
{ mODTextTranslationClass(uiImportHorizon2D);
public:
			uiImportHorizon2D(uiParent*);
			~uiImportHorizon2D();

    void		getEMObjIDs(TypeSet<EM::ObjectID>&) const;
    Notifier<uiImportHorizon2D>	readyForDisplay;

protected:

    uiFileInput*	inpfld_;
    uiPushButton*       scanbut_;
    uiListBox*		horselfld_;
    uiTableImpDataSel*  dataselfld_;
    uiGenInput*		udftreatfld_;

    bool		acceptOK(CallBacker*) override;
    void                descChg(CallBacker*);
    void		setSel(CallBacker*);
    void		addHor(CallBacker*);
    void		formatSel(CallBacker*);
    void		scanPush(CallBacker*);

    bool		getFileNames(BufferStringSet&) const;
    bool		checkInpFlds();
    bool		doImport();

    Table::FormatDesc&  fd_;
    Horizon2DScanner*	scanner_;
    BufferStringSet&	linesetnms_;
    TypeSet<MultiID>	setids_;
    TypeSet<EM::ObjectID> emobjids_;

    ObjectSet<SurfaceInfo>	horinfos_;
};