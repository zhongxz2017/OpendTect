#ifndef vishorizonsection_h
#define vishorizonsection_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		March 2009
 RCS:		$Id: vishorizonsection.h,v 1.28 2009-06-12 21:12:31 cvskris Exp $
________________________________________________________________________


-*/

#include "arrayndimpl.h"
#include "position.h"
#include "rowcol.h"
#include "visobject.h"
#include "geomelement.h"

class BinIDValueSet;
class Color;
class SbBox3f;
class SbVec2f;
class SbVec3f;
class SoAction;
class SoCallback;
class SoGetBoundingBoxAction;
class SoGroup;
class SoShapeHints;
class SoState;
class SoIndexedLineSet;
class SoDGBIndexedPointSet;
class SoIndexedTriangleStripSet;
class SoNormal;
class SoTextureComposer;
class SoTextureCoordinate2;
class ZAxisTransform;

namespace Geometry { class BinIDSurface; }
namespace ColTab { class Sequence; struct MapperSetup; }

#define mHorSectNrRes		6

namespace visBase
{
class VisColorTab;    
class TextureChannel2RGBA;    
class Coordinates;
class HorizonSectionTile;
class TextureChannels;
class Texture2;
class TileResolutionTesselator;

/*!Horizon geometry is divided into 64*64 pixel tiles. Each tile has it's own 
  glue edge to merge into it's neighbors in case of different resolutions. Each
  tile has it's own coordinates and normals, but they all share the same texture  coordinates since the have the same size. Each tile holds its wireframe. It 
  would only turn on wireframe or lines and points depends if you use wireframe
  or not. */

mClass HorizonSection : public VisualObjectImpl
{
public:
    static HorizonSection*	create() mCreateDataObj(HorizonSection);

    void			setDisplayTransformation(Transformation*);
    Transformation*		getDisplayTransformation();
    void			setZAxisTransform(ZAxisTransform*);

    void			setRightHandSystem(bool);

    void                        useChannel(bool);
    int                         nrChannels() const;
    void                        addChannel();
    void                        removeChannel(int);
    void                        swapChannels(int,int);

    int                         nrVersions(int channel) const;
    void                        setNrVersions(int channel,int);
    int                         activeVersion(int channel) const;
    void                        selectActiveVersion(int channel,int);

    void			setSurface(Geometry::BinIDSurface*,bool conn,
	    				   TaskRunner*);
    Geometry::BinIDSurface*	getSurface() const	{ return geometry_; }

    void			useWireframe(bool);
    bool			usesWireframe() const;
    
    char			nrResolutions() const { return mHorSectNrRes; }
    char			currentResolution() const;
    void			setResolution(int,TaskRunner*);

    void			setWireframeColor(Color col);
    void			setColTabSequence(int channel,
	    					  const ColTab::Sequence&);
    const ColTab::Sequence*	getColTabSequence(int channel) const;
    void			setColTabMapperSetup(int channel,
						const ColTab::MapperSetup&,
						TaskRunner*);
    const ColTab::MapperSetup*	getColTabMapperSetup(int channel) const;

    void			setTransparency(int ch,unsigned char yn);
    unsigned char		getTransparency(int ch) const;

    void			getDataPositions(BinIDValueSet&,float) const;
    void			setTextureData(int channel,
	    				       const BinIDValueSet*);
    const BinIDValueSet*	getCache(int channel) const;
    void			inValidateCache(int channel);

    void			setChannel2RGBA(TextureChannel2RGBA*);
    TextureChannel2RGBA*	getChannel2RGBA();
    const TextureChannel2RGBA*	getChannel2RGBA() const;

protected:
    				~HorizonSection();
    friend class		HorizonSectionTile;			
    friend class		HorizonSectionTileUpdater;			
    void			surfaceChangeCB(CallBacker*);
    void			surfaceChange(const TypeSet<GeomPosID>*,
	    				      TaskRunner*);
    void			removeZTransform();
    void			updateZAxisVOI();

    void			updateTexture(int channel);
    void			updateAutoResolution(SoState*,TaskRunner*);
    static void			updateAutoResolution(void*,SoAction*);
    void			turnOnWireframe(int res,TaskRunner*);
    void			updateTileTextureOrigin(const RowCol& texorig);
    void			updateTileArray(const StepInterval<int>& rrg,
	    					const StepInterval<int>& crg);
    void			updateBBox(SoGetBoundingBoxAction* action);
    HorizonSectionTile*		createTile(int rowidx,int colidx);

    Geometry::BinIDSurface*	geometry_;
    Threads::Mutex		geometrylock_;
    ObjectSet<BinIDValueSet>	cache_;

    TextureChannels*		channels_;
    TextureChannel2RGBA*	channel2rgba_;
    SoTextureCoordinate2*	texturecrds_;

    SoCallback*			callbacker_;
    SoShapeHints*		shapehints_;

    Array2DImpl<HorizonSectionTile*> tiles_;
    bool			usewireframe_;

    Transformation*		transformation_;
    ZAxisTransform*		zaxistransform_;
    int				zaxistransformvoi_; 
    				//-1 not needed by zaxistransform, -2 not set
				
    int				desiredresolution_;
    double			cosanglexinl_;
    double			sinanglexinl_;
    float			rowdistance_;
    float			coldistance_;

    RowCol			origin_;
    RowCol			step_;
    
    static ArrPtrMan<SbVec2f>	texturecoordptr_;				
    static int			normalstartidx_[mHorSectNrRes];
    static int			normalsidesize_[mHorSectNrRes];
};



mClass HorizonSectionTile
{
public:
				HorizonSectionTile();
				~HorizonSectionTile();
    void			setResolution(int);
    				/*!<Resolution -1 means it is automatic. */
    int				getActualResolution() const;
    void			updateAutoResolution(SoState*);
    				/*<Update only when the resolutionis -1. */
    void			setNeighbor(int,HorizonSectionTile*);
    void			setPos(int row,int col,const Coord3&);
    void			setDisplayTransformation(Transformation*);

    void			setTextureSize(int rowsz,int colsz);
    void			setTextureOrigin(int globrow,int globcol);

    void			setNormal(int idx,const SbVec3f& normal);
    int				getNormalIdx(int crdidx,int res) const;
    void			reverseNormals();

    void			resetResolutionChangeFlag();
    void			resetGlueNeedsUpdateFlag();

    void			tesselateActualResolution();
    void			updateGlue();

    void			useWireframe(bool);
    void			turnOnWireframe(int res);
    void			setWireframeMaterial(Material*);
    void			setWireframeColor(Color col);
    
    SbBox3f			getBBox() const; 
    SoLockableSeparator*	getNodeRoot() const	{ return root_; }

    bool			allNormalsInvalid(int res) const;
    void			setAllNormalsInvalid(int res,bool yn);
    void			removeInvalidNormals(int res);

protected:

    friend class		HorizonSectionTileUpdater;			
    void			setActualResolution(int);
    int				getAutoResolution(SoState*);
    void			tesselateGlue();
    void			tesselateResolution(int);
    void			updateBBox();
    void			setWireframe(int res);
    void			setInvalidNormals(int row,int col);
    void			setNormal(HorizonSection& section,int normidx,
	    				 int res,int tilerowidx,int tilecolidx);
    void			updateNormals(HorizonSection& section,int res,
					      int tilerowidx,int tilecolidx);

    bool			usewireframe_;
    bool			wireframeneedsupdate_[mHorSectNrRes];
    Material*			wireframematerial_;

    HorizonSectionTile*		neighbors_[9];

    Coord3			bboxstart_;	//Display space
    Coord3			bboxstop_;	//Display space
    bool			needsupdatebbox_;

    SoLockableSeparator*	root_;
    visBase::Coordinates*	coords_;
    SoTextureComposer*		texture_;
    SoSwitch*			resswitch_;
    SoNormal*			normals_;
    Threads::Mutex		normlock_;

    int				desiredresolution_;
    bool			resolutionhaschanged_;

    bool			needsretesselation_[mHorSectNrRes];

    TypeSet<int>		invalidnormals_[mHorSectNrRes];
    bool			allnormalsinvalid_[mHorSectNrRes];

    SoGroup*			resolutions_[mHorSectNrRes];
    SoIndexedTriangleStripSet*	triangles_[mHorSectNrRes];
    SoIndexedLineSet*		lines_[mHorSectNrRes];
    SoIndexedLineSet*		wireframes_[mHorSectNrRes];
    SoDGBIndexedPointSet*	points_[mHorSectNrRes];
    SoSwitch*			wireframeswitch_[mHorSectNrRes];
    SoSeparator*		wireframeseparator_[mHorSectNrRes];
    Texture2*			wireframetexture_;

    SoIndexedTriangleStripSet*	gluetriangles_;
    SoSwitch*			gluelowdimswitch_;
    SoIndexedLineSet*		gluelines_;
    SoDGBIndexedPointSet*	gluepoints_;
    bool			glueneedsretesselation_;

    static int			normalstartidx_[mHorSectNrRes];
    static int			normalsidesize_[mHorSectNrRes];
    static int			spacing_[mHorSectNrRes];
    static int			nrcells_[mHorSectNrRes];
};

};


#endif
