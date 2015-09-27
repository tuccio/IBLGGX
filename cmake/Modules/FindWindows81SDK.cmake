if (WIN32)

	set (ENV_PROGRAMFILES $ENV{ProgramFiles})
	
	set (
		Windows81SDK_INC_PATH_SUFFIXES
		um
	)
	
	set (
		Windows81SDK_LIB_PATH_SUFFIXES
		x86
		um
	)
	set (
		Windows81SDK_ROOT
		"${ENV_PROGRAMFILES}/Windows Kits/8.1/"
	)
	
	find_path ( Windows81SDK_INCLUDE_DIR NAMES d3d11.h HINTS "${Windows81SDK_ROOT}/Include" PATH_SUFFIXES ${Windows81SDK_INC_PATH_SUFFIXES} )
	
	find_library ( Windows81SDK_D3D11_LIBRARY NAMES d3d11 HINTS "${Windows81SDK_ROOT}/Lib/*/*" PATH_SUFFIXES ${Windows81SDK_LIB_PATH_SUFFIXES} )
	find_library ( Windows81SDK_D3DCOMPILER_LIBRARY NAMES d3dcompiler HINTS "${Windows81SDK_ROOT}/Lib/*/*" PATH_SUFFIXES ${Windows81SDK_LIB_PATH_SUFFIXES} )
	
	if (NOT Windows81SDK_INCLUDE_DIR)
		set ( Windows81SDK_FOUND FALSE)
	else ()
		set ( Windows81SDK_FOUND TRUE )
	endif ()
	
	if (Windows81SDK_FOUND)
		if (NOT Windows81SDK_FIND_QUIET)
			message ( STATUS "Found Windows 8.1 SDK: ${Windows81SDK_ROOT}" )
		endif ()
	elseif (Windows81SDK_FIND_REQUIRED)
		message ( FATAL_ERROR "Could not find Windows 8.1 SDK" )
	endif()

endif ()