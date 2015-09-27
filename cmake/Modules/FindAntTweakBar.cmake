find_path( ANTTWEAKBAR_ROOT NAMES include/AntTweakBar.h )
    
find_library ( ANTTWEAKBAR_LIBRARY NAMES AntTweakBar PATHS ${ANTTWEAKBAR_ROOT}/lib )
    
if ( ANTTWEAKBAR_LIBRARY )
	set ( ANTTWEAKBAR_FOUND TRUE )
else ()
	set ( ANTTWEAKBAR_FOUND FALSE )
endif ()
    
if ( ANTTWEAKBAR_FOUND )

    mark_as_advanced ( ANTTWEAKBAR_ROOT ANTTWEAKBAR_LIBRARY )
	
    set ( ANTTWEAKBAR_INCLUDE_DIR ${ANTTWEAKBAR_ROOT}/include CACHE STRING "" )
	
elseif (ANTTWEAKBAR_FIND_REQUIRED)

	message ( FATAL_ERROR "Could not find AntTweakBar." )
	
endif()