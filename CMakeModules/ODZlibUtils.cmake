#________________________________________________________________________
#
# Copyright:    (C) 1995-2022 dGB Beheer B.V.
# License:      https://dgbes.com/licensing
#________________________________________________________________________
#

macro( OD_SETUP_ZLIB_TARGET )
    if ( UNIX )
	get_target_property( ZLIB_LOCATION ZLIB::ZLIB IMPORTED_LOCATION_RELEASE )
	if ( ZLIB_LOCATION )
	    get_filename_component( ZLIB_IMPORTED_LOCATION_RELEASE "${ZLIB_LOCATION}" REALPATH )
	    set_target_properties( ZLIB::ZLIB PROPERTIES
				   IMPORTED_LOCATION_RELEASE "${ZLIB_IMPORTED_LOCATION_RELEASE}" )
	    unset( ZLIB_IMPORTED_LOCATION_RELEASE )
	endif()
	unset( ZLIB_LOCATION )
    endif()
endmacro(OD_SETUP_ZLIB_TARGET)

macro( OD_FIND_ZLIB )

    if ( NOT ZLIB_FOUND )
	if ( Qt${QT_VERSION_MAJOR}Core_FOUND )
	    if ( QT_VERSION_MAJOR EQUAL 5 )
		find_package( Qt${QT_VERSION_MAJOR} QUIET COMPONENTS Zlib GLOBAL )
		if ( Qt${QT_VERSION_MAJOR}Zlib_FOUND )
		    set ( ZLIB_LIBRARY Qt${QT_VERSION_MAJOR}::Core )
		    set ( ZLIB_FOUND TRUE )
		endif()
	    elseif( QT_VERSION_MAJOR GREATER_EQUAL 6 )
		find_package( Qt${QT_VERSION_MAJOR} QUIET COMPONENTS ZlibPrivate GLOBAL )
		if ( Qt${QT_VERSION_MAJOR}ZlibPrivate_FOUND )
		    set ( ZLIB_LIBRARY Qt${QT_VERSION_MAJOR}::Core )
		    set ( ZLIB_FOUND TRUE )
		endif()
	    endif()
	endif()

	if ( NOT ZLIB_FOUND )
	    find_package( ZLIB QUIET GLOBAL )
	endif()
    endif()

endmacro(OD_FIND_ZLIB)

macro( OD_SETUP_ZLIB )

    if ( ZLIB_FOUND )
	if ( TARGET ZLIB::ZLIB )
	    OD_SETUP_ZLIB_TARGET()
	    list( APPEND OD_MODULE_EXTERNAL_LIBS ZLIB::ZLIB )
	elseif ( TARGET ${ZLIB_LIBRARY} AND ${ZLIB_LIBRARY} STREQUAL "Qt${QT_VERSION_MAJOR}::Core" )
	    if ( QT_VERSION_MAJOR EQUAL 5 AND Qt${QT_VERSION_MAJOR}Zlib_FOUND )
		list( APPEND OD_MODULE_EXTERNAL_LIBS "Qt${QT_VERSION_MAJOR}::Zlib" )
	    elseif ( QT_VERSION_MAJOR GREATER_EQUAL 6 AND Qt${QT_VERSION_MAJOR}ZlibPrivate_FOUND )
		list( APPEND OD_MODULE_EXTERNAL_LIBS "Qt${QT_VERSION_MAJOR}::ZlibPrivate" )
		#list( APPEND OD_MODULE_EXTERNAL_LIBS "Qt${QT_VERSION_MAJOR}::Core" )
	    endif()
	endif()

	list( APPEND OD_MODULE_COMPILE_DEFINITIONS "HAS_ZLIB" )
    else()
	set( ZLIB_ROOT "" CACHE PATH "ZLIB location" )
	message( SEND_ERROR "Cannot find/use the ZLIB installation" )
    endif()

endmacro(OD_SETUP_ZLIB)
