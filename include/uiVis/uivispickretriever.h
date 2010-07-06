#ifndef uivispickretriever_h
#define uivispickretriever_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          Mar 2002
 RCS:           $Id: uivispickretriever.h,v 1.6 2010-07-06 17:37:50 cvsnanne Exp $
________________________________________________________________________

-*/

#include "pickretriever.h"
#include "position.h"

namespace visSurvey { class Scene; }
class uiVisPartServer;

mClass uiVisPickRetriever : public PickRetriever
{
public:
    			uiVisPickRetriever(uiVisPartServer*);
    bool		enable(const TypeSet<int>* allowedscenes);
    NotifierAccess*	finished()		{ return &finished_; }

    void		reset();
    bool		success() const		{ return status_==Success; }
    bool		waiting() const		{ return status_==Waiting; }
    const Coord3&	getPos() const		{ return pickedpos_; }
    int			getSceneID() const	{ return pickedscene_; }
    			
    void		addScene(visSurvey::Scene*);
    void		removeScene(visSurvey::Scene*);

protected:
				~uiVisPickRetriever();
    void			pickCB(CallBacker*);

    ObjectSet<visSurvey::Scene>	scenes_;
    TypeSet<int>		allowedscenes_;

    enum Status			{ Idle, Waiting, Failed, Success } status_;
    Coord3			pickedpos_;
    int				pickedscene_;
    Notifier<uiVisPickRetriever> finished_;
    uiVisPartServer*            visserv_;
};

#endif
