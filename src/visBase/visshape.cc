/*
___________________________________________________________________

 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Jan 2003
___________________________________________________________________

-*/

static const char* rcsID = "$Id: visshape.cc,v 1.16 2004-09-09 12:45:53 nanne Exp $";

#include "visshape.h"

#include "errh.h"
#include "iopar.h"
#include "viscoord.h"
#include "visdataman.h"
#include "visdetail.h"
#include "visevent.h"
#include "vismaterial.h"
#include "visnormals.h"
#include "vistexture2.h"
#include "vistexture3.h"
#include "vistexturecoords.h"

#include "Inventor/nodes/SoIndexedShape.h"
#include "Inventor/nodes/SoMaterialBinding.h"
#include "Inventor/nodes/SoNormalBinding.h"
#include "Inventor/nodes/SoSeparator.h"
#include "Inventor/nodes/SoShapeHints.h"
#include "Inventor/nodes/SoSwitch.h"


const char* visBase::Shape::onoffstr = "Is on";
const char* visBase::Shape::texturestr = "Texture";
const char* visBase::Shape::materialstr = "Material";

visBase::Shape::Shape( SoNode* shape_ )
    : shape( shape_ )
    , onoff( new SoSwitch )
    , texture2( 0 )
    , texture3( 0 )
    , material( 0 )
    , root( new SoSeparator )
    , materialbinding( 0 )
{
    onoff->ref();
    onoff->addChild( root );
    onoff->whichChild = 0;
    insertNode( shape );
}


visBase::Shape::~Shape()
{
    if ( texture2 ) texture2->unRef();
    if ( texture3 ) texture3->unRef();
    if ( material ) material->unRef();

    getInventorNode()->unref();
}


void visBase::Shape::turnOn(bool n)
{
    if ( onoff ) onoff->whichChild = n ? 0 : SO_SWITCH_NONE;
    else if ( !n )
    {
	pErrMsg( "Turning off object without switch");
    }
}


bool visBase::Shape::isOn() const
{
    return !onoff || !onoff->whichChild.getValue();
}


void visBase::Shape::removeSwitch()
{
    root->ref();
    onoff->unref();
    onoff = 0;
}


void visBase::Shape::setRenderCache(int mode)
{
    if ( !mode )
	root->renderCaching = SoSeparator::OFF;
    else if ( mode==1 )
	root->renderCaching = SoSeparator::ON;
    else
	root->renderCaching = SoSeparator::AUTO;
}


int visBase::Shape::getRenderCache() const
{
    if ( root->renderCaching.getValue()==SoSeparator::OFF )
	return 0;

    if ( root->renderCaching.getValue()==SoSeparator::ON )
	return 1;
    
    return 2;
}


#define setGetItem(ownclass, clssname, variable) \
void visBase::ownclass::set##clssname( visBase::clssname* newitem ) \
{ \
    if ( variable ) \
    { \
	removeNode( variable->getInventorNode() ); \
	variable->unRef(); \
	variable = 0; \
    } \
 \
    if ( newitem ) \
    { \
	variable = newitem; \
	variable->ref(); \
	insertNode( variable->getInventorNode() ); \
    } \
} \
 \
 \
visBase::clssname* visBase::ownclass::get##clssname() \
{ \
    return variable; \
}


setGetItem( Shape, Texture2, texture2 );
setGetItem( Shape, Texture3, texture3 );
setGetItem( Shape, Material, material );


void visBase::Shape::setMaterialBinding( int nv )
{
    bool isindexed = dynamic_cast<IndexedShape*>( this );

    if ( !materialbinding )
    {
	materialbinding = new SoMaterialBinding;
	insertNode( materialbinding );
    }
    if ( !nv )
    {
	materialbinding->value = SoMaterialBinding::OVERALL;
    }
    else if ( nv==1 )
    {
	materialbinding->value = isindexed ?
	    SoMaterialBinding::PER_FACE_INDEXED : SoMaterialBinding::PER_FACE;
    }
    else
    {
	materialbinding->value = isindexed
	    ? SoMaterialBinding::PER_VERTEX_INDEXED
	    : SoMaterialBinding::PER_VERTEX;
    }
}


int visBase::Shape::getMaterialBinding() const
{
    if ( !materialbinding ) return 0;
    return materialbinding->value.getValue()==SoMaterialBinding::PER_FACE ||
	   materialbinding->value.getValue()==
	   			SoMaterialBinding::PER_FACE_INDEXED ? 1 : 2;
}





void visBase::Shape::fillPar( IOPar& iopar, TypeSet<int>& saveids ) const
{
    VisualObject::fillPar( iopar, saveids );

    if ( material )
	iopar.set( materialstr, material->id() );

    int textureindex = -1;
    if ( texture2 )
	textureindex = texture2->id();
    else if ( texture3 )
	textureindex = texture3->id();

    if ( textureindex!=-1 )
    {
	iopar.set( texturestr, textureindex );
	if ( saveids.indexOf( textureindex ) != -1 )
	    saveids += textureindex;
    }

    iopar.setYN( onoffstr, isOn() );
}


int visBase::Shape::usePar( const IOPar& par )
{
    int res = VisualObject::usePar( par );
    if ( res!=1 ) return res;

    bool ison;
    if ( par.getYN( onoffstr, ison ) )
	turnOn( ison );

    int textureindex;
    if ( par.get( texturestr, textureindex ) && textureindex!=-1 )
    {
	if ( !DM().getObj( textureindex ) )
	    return 0;

	Texture2* t2 = dynamic_cast<Texture2*>(DM().getObj(textureindex));
	Texture3* t3 = dynamic_cast<Texture3*>(DM().getObj(textureindex));

	if ( t2 ) setTexture2( t2 );
	else if ( t3 ) setTexture3( t3 );
	else return -1;
    }

    return 1;
}

	
SoNode* visBase::Shape::getInventorNode()
{ return onoff ? (SoNode*) onoff : (SoNode*) root; }


void visBase::Shape::insertNode( SoNode*  node )
{
    root->insertChild( node, 0 );
}


void visBase::Shape::removeNode( SoNode* node )
{
    while ( root->findChild( node ) != -1 )
	root->removeChild( node );
}


visBase::VertexShape::VertexShape( SoVertexShape* shape_ )
    : Shape( shape_ )
    , normals( 0 )
    , coords( 0 )
    , texturecoords( 0 )
    , normalbinding( 0 )
    , shapehints( 0 )
{
    setCoordinates( visBase::Coordinates::create() );
}


visBase::VertexShape::~VertexShape()
{
    setCoordinates( 0 );
    setTextureCoords( 0 );
    setNormals( 0 );
}


void visBase::VertexShape::setTransformation( Transformation* tr )
{ coords->setTransformation( tr ); }


visBase::Transformation* visBase::VertexShape::getTransformation()
{ return  coords->getTransformation(); }


setGetItem( VertexShape, Coordinates, coords );
setGetItem( VertexShape, Normals, normals );
setGetItem( VertexShape, TextureCoords, texturecoords );


const visBase::Coordinates* visBase::VertexShape::getCoordinates() const
{
    return const_cast<VertexShape*>(this)->getCoordinates();
}


void visBase::VertexShape::setNormalPerFaceBinding( bool nv )
{
    bool isindexed = dynamic_cast<IndexedShape*>( this );

    if ( !normalbinding )
    {
	normalbinding = new SoNormalBinding;
	insertNode( normalbinding );
    }
    if ( nv )
    {
	normalbinding->value = isindexed ?
	    SoNormalBinding::PER_FACE_INDEXED : SoNormalBinding::PER_FACE;
    }
    else
    {
	normalbinding->value = isindexed
	    ? SoNormalBinding::PER_VERTEX_INDEXED
	    : SoNormalBinding::PER_VERTEX;
    }
}


bool visBase::VertexShape::getNormalPerFaceBinding() const
{
    if ( !normalbinding ) return true;
    return normalbinding->value.getValue()==SoNormalBinding::PER_FACE ||
	   normalbinding->value.getValue()==SoNormalBinding::PER_FACE_INDEXED;
}


#define mCheckCreateShapeHints() \
    if ( !shapehints ) \
    { \
	shapehints = new SoShapeHints; \
	insertNode( shapehints ); \
    }

void visBase::VertexShape::setVertexOrdering( int nv )
{
    mCheckCreateShapeHints()
    if ( !nv )
	shapehints->vertexOrdering = SoShapeHints::CLOCKWISE;
    else if ( nv==1 )
	shapehints->vertexOrdering = SoShapeHints::COUNTERCLOCKWISE;
    else
	shapehints->vertexOrdering = SoShapeHints::UNKNOWN_ORDERING;
}


int visBase::VertexShape::getVertexOrdering() const
{
    if ( !shapehints ) return 2;

    if ( shapehints->vertexOrdering.getValue()==SoShapeHints::CLOCKWISE )
	return 0;
    if ( shapehints->vertexOrdering.getValue()==SoShapeHints::COUNTERCLOCKWISE )
	return 1;

    return 2;
}


void visBase::VertexShape::setFaceType( int ft )
{
    mCheckCreateShapeHints()
    shapehints->faceType = !ft ? SoShapeHints::UNKNOWN_FACE_TYPE
			       : SoShapeHints::CONVEX;
}


int visBase::VertexShape::getFaceType() const
{
    return shapehints && 
	shapehints->faceType.getValue() == SoShapeHints::CONVEX ? 1 : 0;
}


void visBase::VertexShape::setShapeType( int st )
{
    mCheckCreateShapeHints()
    shapehints->shapeType = !st ? SoShapeHints::UNKNOWN_SHAPE_TYPE
			        : SoShapeHints::SOLID;
}


int visBase::VertexShape::getShapeType() const
{
    return shapehints && 
	shapehints->shapeType.getValue() == SoShapeHints::SOLID ? 1 : 0;
}


visBase::IndexedShape::IndexedShape( SoIndexedShape* shape_ )
    : VertexShape( shape_ )
    , indexedshape( shape_ )
{}


#define setGetIndex( resourcename, fieldname )  \
int visBase::IndexedShape::nr##resourcename() const \
{ return indexedshape->fieldname.getNum(); } \
 \
 \
void visBase::IndexedShape::set##resourcename( int pos, int idx ) \
{ indexedshape->fieldname.set1Value( pos, idx ); } \
 \
 \
void visBase::IndexedShape::remove##resourcename##After(int pos) \
{  \
    if ( indexedshape->fieldname.getNum()>pos+1 ) \
	indexedshape->fieldname.deleteValues(pos+1); \
} \
 \
 \
int visBase::IndexedShape::get##resourcename( int pos ) const \
{ return indexedshape->fieldname[pos]; } \


setGetIndex( CoordIndex, coordIndex );
setGetIndex( TextureCoordIndex, textureCoordIndex );
setGetIndex( NormalIndex, normalIndex );
setGetIndex( MaterialIndex, materialIndex );


int visBase::IndexedShape::getClosestCoordIndex(const EventInfo& ei) const
{
    mDynamicCastGet(  FaceDetail * const, facedetail, ei.detail );
    if ( !facedetail ) return -1;

    return facedetail->getClosestIdx( getCoordinates(), ei.localpickedpos );
}
