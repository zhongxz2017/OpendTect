#ifndef flatauxdataeditor_h
#define flatauxdataeditor_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Kris
 Date:          Mar 2007
 RCS:           $Id: flatauxdataeditor.h,v 1.7 2007-08-30 14:34:13 cvskris Exp $
________________________________________________________________________

-*/

#include "flatview.h"
#include "callback.h"
#include "geometry.h"

class MouseEventHandler;

namespace FlatView
{

/*!Editor for FlatView::Annotation::AuxData. Allows the enduser to
   click-drag-release the points in data.
   Users of the class have the choice if the editor should do the changes for
   them, or if they want to do changes themself, driven by the callback. */

class AuxDataEditor : public CallBacker
{
public:
			AuxDataEditor(Viewer&,MouseEventHandler&);
    virtual		~AuxDataEditor();
    int			addAuxData(FlatView::Annotation::AuxData*,bool doedit);
    			/*!<\param doedit says whether this object
			     should change the auxdata, or if the user
			     of the objects should do it.
			    \returns an id of the new set. */
    void		removeAuxData(int id);
    void		enableEdit(int id,bool allowadd,bool allowmove,
				   bool allowdelete);
    void		setAddAuxData(int id);
    			//!<Added points will be added to this set.
    int			getAddAuxData() const;
    			//!<\returns the id of the set that new points will
			//!< 	     be added to
    
    void		setView(const Rect& wv,
	    			const Geom::Rectangle<int>& mouserect);
    			/*!<User of the class must ensure that both
			    the wv and the mouserect are up to date at
			    all times. */
    Rect		getWorldRect(int dataid) const;
    void		limitMovement(const Rect&);
    			/*!<When movement starts, the movement is unlimited.
			    Movement can be limited once the movement started
			    by calling limitMovement. */
    bool		isDragging() const { return mousedown_; }

    int				getSelPtDataID() const;
    int				getSelPtIdx() const;
    const Point&		getSelPtPos() const;
			
    Notifier<AuxDataEditor>	movementStarted;
    Notifier<AuxDataEditor>	movementFinished;
    				/*!If selPtIdx()==-1, position should be added,
				   otherwise moved. */
    Notifier<AuxDataEditor>	removeSelected;

    void			setSelectionPolygonStyle(const LineStyle&);
    const LineStyle&		getSelectionPolygonStyle() const;
    void			getPointSelections(TypeSet<int>& ids,
	    					   TypeSet<int>& idxs) const;
    				/*!<Each point within the limits of the polygons
				    will be put in the typesets.*/

    const Viewer&	viewer() const	{ return viewer_; }
    Viewer&		viewer()	{ return viewer_; }

    const TypeSet<int>&				getIds() const;
    const ObjectSet<Annotation::AuxData>&	getAuxData() const;


protected:
    void		removeSelectionPolygon();
    void		mousePressCB(CallBacker*);
    void		mouseReleaseCB(CallBacker*);
    void		mouseMoveCB(CallBacker*);

    bool		updateSelection(const Geom::Point2D<int>&);
    			//!<\returns true if something is selected

    Viewer&				viewer_;
    ObjectSet<Annotation::AuxData>	auxdata_;
    TypeSet<int>			ids_;
    BoolTypeSet				allowadd_;
    BoolTypeSet				allowmove_;
    BoolTypeSet				allowremove_;
    BoolTypeSet				doedit_;

    int					addauxdataid_;
    ObjectSet<Annotation::AuxData>	polygonsel_;
    LineStyle				polygonsellst_;
    Annotation::AuxData*		feedback_;
    Geom::Point2D<int>			prevpt_;

    Geom::PixRectangle<int>		mousearea_;
    Rect				curview_;
    MouseEventHandler&			mousehandler_;
    bool				mousedown_;
    bool				hasmoved_;

    int					seldatasetidx_;
    int					selptidx_;
    Point				selptcoord_;
    Rect				movementlimit_;
};



}; // namespace FlatView

#endif
