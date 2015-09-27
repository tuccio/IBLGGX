cmake_minimum_required ( VERSION 3.0 )

project ( IBLGGX )

set ( CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/" )
set ( EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin )
set ( LIBRARY_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/lib )

find_package ( Windows81SDK REQUIRED )
find_package ( DXUT         REQUIRED )
find_package ( DirectXTex   REQUIRED )
find_package ( DirectXMesh  REQUIRED )
find_package ( DirectXTK    REQUIRED )
find_package ( FX11         REQUIRED )
find_package ( AntTweakBar )
find_package ( OpenMP )

if ( OPENMP_FOUND )

	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")

endif ()

include_directories ( ${Windows81SDK_INCLUDE_DIR} )
include_directories ( ${DXUT_INCLUDE_DIR} )
include_directories ( ${DIRECTXTEX_INCLUDE_DIR} ${DIRECTXMESH_INCLUDE_DIR} ${DIRECTXTK_INCLUDE_DIR} ${FX11_INCLUDE_DIR} )

add_definitions ( -DUNICODE -D_UNICODE )

add_executable ( iblggx WIN32 iblggx.cpp iblinteg.hpp iblinteg.cpp rgbe.h rgbe.c dxutils.hpp dxutils.cpp )

target_link_libraries ( iblggx ${Windows81SDK_D3D11_LIBRARY} ${Windows81SDK_D3DCOMPILER_LIBRARY} ${DXUT_LIBRARY} comctl32 winmm )
target_link_libraries ( iblggx ${DXUT_LIBRARY} comctl32 winmm )
target_link_libraries ( iblggx ${DIRECTXTEX_LIBRARY} ${DIRECTXMESH_LIBRARY} ${DIRECTXTK_LIBRARY} ${FX11_LIBRARY} )

if ( ANTTWEAKBAR_FOUND )

	add_definitions ( -DIBL_USE_ANTTWEAKBAR )
	include_directories ( ${ANTTWEAKBAR_INCLUDE_DIR} )
	target_link_libraries ( iblggx ${ANTTWEAKBAR_LIBRARY} )

endif ()