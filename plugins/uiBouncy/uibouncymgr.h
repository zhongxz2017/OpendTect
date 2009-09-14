#ifndef uibouncymgr_h
#define uibouncymgr_h
/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Karthika
 * DATE     : Aug 2009
 * ID       : $Id: uibouncymgr.h,v 1.3 2009-09-14 22:51:34 cvskarthika Exp $
-*/

#include "callback.h"

class uiODMain;
namespace Bouncy { class BouncyController; }

namespace uiBouncy
{
class uiBouncyMain;
class uiBouncySettingsDlg;
class BouncyDisplay;

mClass uiBouncyMgr : public CallBacker
{
public:

    				uiBouncyMgr(uiODMain*);
				~uiBouncyMgr();
    void			doWork(CallBacker*);

protected:

    void			createBouncy();
    void			destroyBouncy();
    void			destroyAllBounciesCB(CallBacker*);

    void			startGame();
    void			stopGame();
    void			pauseGame(bool);

    // callbacker to show preview of settings change
    void			propertyChangeCB(CallBacker*);
    // callbacker to update ball's position and orientation
    void			newPosAvailableCB(CallBacker*);
    // callbacker to handle events
    void			neweventCB(CallBacker*);

    void                        sessionSaveCB(CallBacker*);
    void                        sessionRestoreCB(CallBacker*);
    void                        surveyToBeChangedCB(CallBacker*);
    void                        surveyChangeCB(CallBacker*);
    void                        shutdownCB(CallBacker*);

    BouncyDisplay*		bouncydisp_;
    uiODMain*			appl_;
    uiBouncyMain*		maindlg_;
    uiBouncySettingsDlg*        settingsdlg_;
    Bouncy::BouncyController*	gamecontroller_;
};

}; // namespace

#endif
