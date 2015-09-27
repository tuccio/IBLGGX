if (CMAKE_CL_64)
    set ( FX11_ARCH "x64" )
else()
    set ( FX11_ARCH "Win32" )
endif()

find_path( FX11_ROOT NAMES inc/d3dx11effect.h )
    
find_library ( FX11_LIBRARY_RELEASE NAMES Effects11 PATHS ${FX11_ROOT}/Bin/*/${FX11_ARCH}/Release )
find_library ( FX11_LIBRARY_DEBUG NAMES Effects11 PATHS ${FX11_ROOT}/Bin/*/${FX11_ARCH}/Debug )
    
if (FX11_LIBRARY_RELEASE AND FX11_LIBRARY_DEBUG)
	set ( FX11_FOUND TRUE )
else ()
	set ( FX11_FOUND FALSE )
endif ()
    
if (FX11_FOUND)

    mark_as_advanced ( FX11_ROOT FX11_LIBRARY_RELEASE FX11_LIBRARY_DEBUG )
	
    set ( FX11_INCLUDE_DIR ${FX11_ROOT}/inc CACHE STRING "" )
    set ( FX11_LIBRARY optimized ${FX11_LIBRARY_RELEASE} debug ${FX11_LIBRARY_DEBUG} CACHE STRING "")
	
elseif (FX11_FIND_REQUIRED)

	message ( FATAL_ERROR "Could not find FX11" )
	
endif()