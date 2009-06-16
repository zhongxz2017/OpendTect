#ifndef uiodapplmgraux_h
#define uiodapplmgraux_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          Mar 2009
 RCS:           $Id: uiodapplmgraux.h,v 1.6 2009-06-16 02:08:15 cvskris Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
#include "uiapplserv.h"
class uiVelSel;
class CtxtIOObj;
class uiODApplMgr;
class DataPointSet;
class uiConvertPos;
class ZAxisTransform;
class VelocityStretcher;


/*!\brief uiApplService for OD */

mClass uiODApplService : public uiApplService
{
public:
    			uiODApplService( uiParent* p, uiODApplMgr& am )
			    : par_(p), applman_(am)	{}
    uiParent*		parent() const;
    bool		eventOccurred(const uiApplPartServer*,int);
    void*		getObject(const uiApplPartServer*, int);

    uiODApplMgr&	applman_;
    uiParent*		par_;
};


/*!\brief Dispatches work for Appl Mgr */

mClass uiODApplMgrDispatcher : public CallBacker
{
    friend class	uiODApplMgr;

    			uiODApplMgrDispatcher( uiODApplMgr& a, uiParent* p )
			    : am_(a), par_(p), convposdlg_(0)	{}
    void		survChg(bool);

    void		doOperation(int,int,int);
    void		manPreLoad(int);
    void		posConversion();
    int			createMapDataPack(const DataPointSet&,int);

    void		processPreStack();
    void		reStartProc();
    void		batchProgs();
    void		pluginMan();
    void		manageShortcuts();
    void		setFonts();

    void		posDlgClose(CallBacker*);

    uiConvertPos*	convposdlg_;
    uiODApplMgr&	am_;
    uiParent*		par_;
};


/*!\brief Does visualisation-related work for uiODApplMgr */

mClass uiODApplMgrAttrVisHandler : public CallBacker
{
    friend class	uiODApplMgr;

    			uiODApplMgrAttrVisHandler( uiODApplMgr& a, uiParent* p )
			    : am_(a), par_(p)		{}
    void		survChg(bool);

    bool		editNLA(bool);
    void		createHorOutput(int,bool);
    void		createVol(bool);
    void		doXPlot();
    void		crossPlot();
    void		setZStretch();
    bool		selectAttrib(int,int);
    void		setHistogram(int,int);
    void		colMapperChg();
    void		createAndSetMapDataPack(int,int,
	    					const DataPointSet&,int);
    void		pageUpDownPressed(bool);
    void		updateColorTable(int,int);
    void		colSeqChg();
    NotifierAccess*	colorTableSeqChange();
    void		useDefColTab(int,int);
    void		saveDefColTab(int,int);

    uiODApplMgr&	am_;
    uiParent*		par_;
};


/*!\brief Velocity model for depth scene */

mClass uiODApplMgrVelSel : public uiDialog
{
public:

			uiODApplMgrVelSel(uiParent*);
			~uiODApplMgrVelSel();

    bool		acceptOK(CallBacker*);
    ZAxisTransform*	transform();
    float		zScale() const		{ return zscale_; }
    const char*		selName() const;

protected:    
    uiVelSel*		velsel_;
    VelocityStretcher*	trans_;
    float		zscale_;

};


#endif
