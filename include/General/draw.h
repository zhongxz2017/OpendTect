#ifndef draw_h
#define draw_h
/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Bril
 Date:          26/07/2000
 RCS:           $Id: draw.h,v 1.3 2000-07-28 13:37:05 arend Exp $
________________________________________________________________________

-*/

#include "enums.h"
#include "color.h"
#include "fontdata.h"


class Alignment
{
public:
    enum Pos		{ Start, Middle, Stop };
			DeclareEnumUtils(Pos)

			Alignment( Pos h=Start, Pos v=Start )
			: hor(h), ver(v)	{}

    Pos			hor;
    Pos			ver;

};


class MarkerStyle
{
public:

    enum Type		{ None, Square, Circle, Cross };
			DeclareEnumUtils(Type)

			MarkerStyle( Type t=Square, Color c=Color::Black,
				     FontData fd=FontData() )
			: type(t), color(c), fontdata(fd)	{}

    Type		type;
    Color		color;
    FontData		fontdata;

};


class LineStyle
{
public:

    enum Type		{ None, Solid, Dash, Dot, DashDot, DashDotDot };
			// This enum should not be changed: it is cast
			// directly to a UI enum.
			DeclareEnumUtils(Type)

			LineStyle( Type t=Solid,int w=1,Color c=Color::Black )
			: type(t), width(w), color(c)	{}

    Type		type;
    int			width;
    Color		color;

};


#endif
