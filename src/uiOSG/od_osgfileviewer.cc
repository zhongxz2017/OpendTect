/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "prog.h"
#include "uimain.h"

#include "commandlineparser.h"
#include "file.h"
#include "moddepmgr.h"
#include "odgraphicswindow.h"

#include <QApplication>
#include <QFileDialog>

#include <osgViewer/Viewer>
#include <osgGA/TrackballManipulator>

#include <osg/Version>
#include <osg/ShapeDrawable>
#include <osg/MatrixTransform>
#include <osgManipulator/TabBoxDragger>
#include <osgDB/ReadFile>

int mProgMainFnName( int argc, char** argv )
{
    mInitProg( OD::UiProgCtxt )
    SetProgramArgs( argc, argv, false );
    uiMain::preInitForOpenGL();
    uiMain app( argc, argv );

    OD::ModDeps().ensureLoaded( "uiTools" );

    PIM().loadAuto( false );

    CommandLineParser clp( argc, argv );
    BufferStringSet files;
    clp.getNormalArguments( files );
    BufferString file;
    if ( !files.isEmpty() )
	file = files.first();

#if OSG_VERSION_LESS_THAN( 3, 5, 0 )
    initQtWindowingSystem();
#endif

    while ( !File::exists(file) )
    {
	file = QFileDialog::getOpenFileName();
	if ( file.isEmpty() )
	    { od_cout() << "Please select an osg file.\n" ; return 1; }
    }

    osg::Node* root = osgDB::readNodeFile( file.buf() );
    if ( !root )
	return 1;

    osg::ref_ptr<osgViewer::Viewer> viewer = new osgViewer::Viewer;
    viewer->setSceneData( root );
    viewer->setCameraManipulator( new osgGA::TrackballManipulator );
    setViewer( viewer.get() );

    OD::ModDeps().ensureLoaded( "uiOSG" );
    PtrMan<ODGLWidget> glw = new ODGLWidget;
    PtrMan<ODGraphicsWindow> graphicswin = new ODGraphicsWindow( glw );
    viewer->getCamera()->setViewport(
		    new osg::Viewport(0, 0, glw->width(), glw->height() ) );
    viewer->getCamera()->setGraphicsContext( graphicswin );
    PIM().loadAuto( true );
    glw->show();

    return app.exec();
}
