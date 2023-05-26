#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "vissurveymod.h"
#include "bufstringset.h"
#include "draw.h"
#include "emposid.h"
#include "visobject.h"
#include "vissurvobj.h"
#include "emobject.h"

class ZAxisTransform;

namespace EM { class EMManager; class EMObject; }
namespace Geometry { class Element; }
namespace visBase
{
    class DataObjectGroup;
    class DrawStyle;
    class TextureChannel2RGBA;
    class MarkerSet;
}

namespace visSurvey
{
struct EMChangeData
{
    ObjectSet<const EM::EMObjectCallbackData>	emcallbackdata_;
    Threads::Lock				lock_;

    void    addCallBackData( const EM::EMObjectCallbackData* data )
    {
	Threads::Locker locker( lock_ );
	emcallbackdata_ += data;
    }

    const EM::EMObjectCallbackData*   getCallBackData( int idx ) const
    {
	if ( idx<emcallbackdata_.size() )
	    return emcallbackdata_[idx];
	return nullptr;
    }

    void    clearData()
    {
	Threads::Locker locker( lock_ );
	deepErase( emcallbackdata_ );
    }

    int     size() const { return emcallbackdata_.size(); }
};

class MPEEditor;
class EdgeLineSetDisplay;


mExpClass(visSurvey) EMObjectDisplay : public visBase::VisualObjectImpl
				     , public visSurvey::SurveyObject
{
public:
    const mVisTrans*		getDisplayTransformation() const override;
    void			setDisplayTransformation(
						const mVisTrans*) override;
    void			setSceneEventCatcher(
					visBase::EventCatcher*) override;
    virtual bool		setZAxisTransform(ZAxisTransform*,
						  TaskRunner*) override
				{ return false; }
    const ZAxisTransform*	getZAxisTransform() const override
				{ return zaxistransform_; }

    virtual bool		setEMObject(const EM::ObjectID&,TaskRunner*);
    EM::ObjectID		getObjectID() const;
    virtual bool		updateFromEM(TaskRunner*);
    virtual void		updateFromMPE();

    void			showPosAttrib( int attr, bool yn);
				/*!<Turns position attributes (as defined in
				    EM::EMObject) to be marked with a marker. */
    bool			showsPosAttrib( int attr ) const;
				/*!<\returns wether a position attribute (as
				     defined in EM::EMObject) to be marked
				     with a marker. */
    const char*			errMsg() const override { return errmsg_.str();}

    MultiID			getMultiID() const override;
    BufferStringSet		displayedSections() const;

    void			setOnlyAtSectionsDisplay(bool yn) override;
    bool			displayedOnlyAtSections() const override;

    void			turnOnSelectionMode(bool) override;
    bool			allowMaterialEdit() const override
				{ return true; }
    const OD::LineStyle*	lineStyle() const override;
    void			setLineStyle(const OD::LineStyle&) override;

    bool			hasColor() const override;
    void			setColor(OD::Color) override;
    OD::Color			getColor() const override;

    bool			allowsPicks() const override	{ return true; }

    void			getObjectInfo(BufferString&) const override;
    void			getMousePosInfo( const visBase::EventInfo& ei,
						 IOPar& iop ) const override
				{ return SurveyObject::getMousePosInfo(ei,iop);}
    void			getMousePosInfo(const visBase::EventInfo&,
					    Coord3&,BufferString& val,
					    BufferString& info) const override;
    MPEEditor*			getEditor();
    void			enableEditing(bool yn);
    bool			isEditingEnabled() const;

    virtual EM::SectionID	getSectionID(VisID visid) const		= 0;
    EM::SectionID		getSectionID(const TypeSet<VisID>* path) const;

    EM::PosID			getPosAttribPosID(int attrib,
					 const TypeSet<VisID>& path,
					 const Coord3& clickeddisplaypos) const;

    bool			canRemoveSelection() const override
				{ return true; }
    bool			removeSelections(TaskRunner*) override;
    void			clearSelections() override;
    virtual void		updateAuxData()			{}

    bool			setChannels2RGBA(
					visBase::TextureChannel2RGBA*) override;
    visBase::TextureChannel2RGBA* getChannels2RGBA() override;

    void			fillPar(IOPar&) const override;
    bool			usePar(const IOPar&) override;

    NotifierAccess*		getMovementNotifier() override
				{ return &hasmoved; }
    Notifier<EMObjectDisplay>	changedisplay;

    void			lock(bool yn) override;
    NotifierAccess*		getLockNotifier() override
				{ return &locknotifier; }
    virtual void		doOtherObjectsMoved(
				    const ObjectSet<const SurveyObject>&,
				    VisID whichobj )	=0;
    void			setPixelDensity(float dpi) override;
    const visBase::MarkerSet*	getSeedMarkerSet() const;

    virtual bool		getOnlyAtSectionsDisplay() const
				{ return displayedOnlyAtSections(); }

protected:
				EMObjectDisplay();
				~EMObjectDisplay();
    virtual void		removeEMStuff();

    virtual void		removeSectionDisplay(const EM::SectionID&) = 0;
    virtual bool		addSection(const EM::SectionID&,TaskRunner*)=0;

    virtual EM::PosID		findClosestNode(const Coord3&) const;
    virtual void		emChangeCB(CallBacker*);
    virtual void		clickCB(CallBacker*);
    virtual void		updatePosAttrib(int attrib);
    void			polygonFinishedCB(CallBacker*);

    virtual void		updateSelections();
    void			handleEmChange(const EM::EMObjectCallbackData&);

    Notifier<EMObjectDisplay>	hasmoved;
    Notifier<EMObjectDisplay>	locknotifier;

    const mVisTrans*			transformation_		= nullptr;
    ZAxisTransform*			zaxistransform_		= nullptr;
    visBase::EventCatcher*		eventcatcher_		= nullptr;

    ObjectSet<visBase::MarkerSet>	posattribmarkers_;

    TypeSet<int>			posattribs_;
    TypeSet<int>			parposattrshown_;

    EM::EMManager&			em_;
    EM::EMObject*			emobject_		= nullptr;
    MultiID				parmid_;
    BufferStringSet			parsections_;

    MPEEditor*				editor_			= nullptr;
    visBase::TextureChannel2RGBA*	channel2rgba_		= nullptr;
					//set in usePar,
					//should be nil when given
					//to channels_.

    TypeSet<EM::SectionID>		addsectionids_;

    mutable OD::Color			nontexturecol_;
    mutable bool			nontexturecolisset_	= false;
    visBase::DrawStyle*			drawstyle_;
    bool				displayonlyatsections_	= false;
    bool				enableedit_		= false;
    bool				restoresessupdate_	= false;

    bool				burstalertison_		= false;
    bool				ctrldown_		= false;
    TypeSet<EM::SubID>			selectionids_;
    EMChangeData			emchangedata_;

    static const char*			sKeyEarthModelID();
    static const char*			sKeyResolution();
    static const char*			sKeyEdit();
    static const char*			sKeyOnlyAtSections();
    static const char*			sKeyLineStyle();
    static const char*			sKeySections();
    static const char*			sKeyPosAttrShown();

private:
    void				unSelectAll();
    void				updateLockedSeedsColor();
};

} // namespace visSurvey
