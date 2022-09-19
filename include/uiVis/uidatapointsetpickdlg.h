#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uivismod.h"
#include "uidialog.h"
#include "emobject.h"
#include "sets.h"

class uiTable;
class uiToolBar;
class Array2DInterpol;
class DataPointSet;

namespace Pick { class SetMgr; }
namespace visSurvey { class PickSetDisplay; }

mExpClass(uiVis) uiDataPointSetPickDlg : public uiDialog
{
mODTextTranslationClass(uiDataPointSetPickDlg)
public:
    virtual		~uiDataPointSetPickDlg();

protected:
			uiDataPointSetPickDlg(uiParent*,SceneID sceneid);

    void		initPickSet();
    void		updateDPS();
    void		updateTable();
    void		updateButtons();
    virtual void	cleanUp();

    void		valChgCB(CallBacker*);
    void		rowClickCB(CallBacker*);
    void		pickModeCB(CallBacker*);
    void		openCB(CallBacker*);
    void		saveCB(CallBacker*);
    void		saveasCB(CallBacker*);
    void		doSave(bool saveas);
    void		pickCB(CallBacker*);
    void		locChgCB(CallBacker*);
    bool		acceptOK(CallBacker*);
    void		winCloseCB(CallBacker*);
    void		objSelCB(CallBacker*);

    VisID		sceneid_;
    uiTable*		table_;
    uiToolBar*		tb_;
    DataPointSet&	dps_;
    TypeSet<float>	values_;
    visSurvey::PickSetDisplay* psd_;
    Pick::SetMgr&	picksetmgr_;
    int			pickbutid_;
    int			savebutid_;
    bool		changed_;
};


mExpClass(uiVis) uiEMDataPointSetPickDlg : public uiDataPointSetPickDlg
{
mODTextTranslationClass(uiEMDataPointSetPickDlg)
public:
			uiEMDataPointSetPickDlg(uiParent*,SceneID sceneid,
						EM::ObjectID);
			~uiEMDataPointSetPickDlg();

    const DataPointSet&	getData() const		{ return emdps_; }
    Notifier<uiEMDataPointSetPickDlg> readyForDisplay;

protected:

    DataPointSet&	emdps_;
    EM::ObjectID	emid_;
    Array2DInterpol*	interpol_;

    int			addSurfaceData();
    int			dataidx_;

    virtual void	cleanUp();
    void		interpolateCB(CallBacker*);
    void		settCB(CallBacker*);

    TrcKeySampling		tks_;
};