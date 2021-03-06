CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
SET(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/modules")

SET( SANE_INCLUDE_DIR "/home/john/local/include" )
SET( SANE_LIBRARY_DIR "/home/john/local/lib" )
SET( SANE_LIBS "sane" )

PROJECT( gsoc )

SET( CMAKE_BUILD_TYPE "Debug" )

FIND_PACKAGE(LCMS REQUIRED)
FIND_PACKAGE(LibRaw REQUIRED)
#Oyranos Flags############
EXECUTE_PROCESS(	COMMAND oyranos-config --ldflags
						COMMAND tr '\n' " "
						OUTPUT_VARIABLE OY_LDFLAGS )
EXECUTE_PROCESS(	COMMAND oyranos-config --cflags
						COMMAND tr '\n' " "
						OUTPUT_VARIABLE OY_CFLAGS )

INCLUDE_DIRECTORIES( ${LIBRAW_INCLUDE_DIR} ${SANE_INCLUDE_DIR} ${LCMS_INCLUDE_DIR} )
LINK_DIRECTORIES( ${SANE_LIBRARY_DIR} )

##################TARGETS##################

#raw2tiff##################
ADD_EXECUTABLE( raw2tiff "RAW.cc" "raw2tiff.cc")
TARGET_LINK_LIBRARIES( raw2tiff ${LIBRAW_LIBRARIES} "exiv2" "tiff" )
SET_TARGET_PROPERTIES( raw2tiff  PROPERTIES
                                 COMPILE_FLAGS "${OY_CFLAGS}"
                                 LINK_FLAGS "${OY_LDFLAGS}" )
#scan##################
ADD_EXECUTABLE( scan "scan.c")
TARGET_LINK_LIBRARIES( scan ${SANE_LIBS} "tiff" )
SET_TARGET_PROPERTIES( scan	PROPERTIES
										COMPILE_FLAGS "${OY_CFLAGS}"
										LINK_FLAGS "${OY_LDFLAGS}" )
#oytest##################
ADD_EXECUTABLE( oytest "oyTest.cc")
SET_TARGET_PROPERTIES( oytest	PROPERTIES
										COMPILE_FLAGS "${OY_CFLAGS}"
										LINK_FLAGS "${OY_LDFLAGS}" )
#oyranos backends##################
ADD_LIBRARY( oyranos_SANE_cmm_module SHARED oyranos_cmm_SANE.c SANE_help.c )
TARGET_LINK_LIBRARIES( oyranos_SANE_cmm_module ${SANE_LIBS} ${LCMS_LIBRARIES} "oyranos" )
ADD_LIBRARY( oyranos_oyRE_cmm_module SHARED oyranos_cmm_oyRE.cc oyRE_help.c )
TARGET_LINK_LIBRARIES( oyranos_oyRE_cmm_module ${LIBRAW_LIBRARIES} "exiv2" "oyranos" )

add_subdirectory( doc )

##################INSTALLATION##################
MESSAGE( "Info: Files will be installed in ${CMAKE_INSTALL_PREFIX}" )
INSTALL( TARGETS raw2tiff scan
	RUNTIME
	DESTINATION "bin/" )
INSTALL( TARGETS oyranos_SANE_cmm_module oyranos_oyRE_cmm_module
	DESTINATION "lib/colour/modules/" )
