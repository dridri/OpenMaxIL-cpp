cmake_minimum_required(VERSION 2.6)

option( cross "Cross compile" 0 )

if ( ${cross} MATCHES "1" OR ${cross} MATCHES "true" OR ${cross} MATCHES "TRUE" )
	message( " => Cross compiling" )
	SET( CMAKE_SYSTEM_NAME Linux )
	SET( CMAKE_SYSTEM_VERSION 1 )

	SET( CROSS arm-linux-gnueabihf- )
	SET( CMAKE_C_COMPILER arm-linux-gnueabihf-gcc )
	SET( CMAKE_CXX_COMPILER arm-linux-gnueabihf-g++ )
	SET( CMAKE_ASM_COMPILER arm-linux-gnueabihf-as )

	SET( CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER )
	SET( CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY )
	SET( CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY )
endif()
project( "OpenMaxIL++" )

message( ${CMAKE_C_COMPILER} )

set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_DEFAULT_SOURCE -DSTANDALONE -D__STDC_CONSTANT_MACROS -D__STDC_LIMIT_MACROS -DTARGET_POSIX -D_LINUX -fPIC -DPIC -D_REENTRANT -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 -U_FORTIFY_SOURCE" )
set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DHAVE_LIBOPENMAX=2 -DOMX -DOMX_SKIP64BIT -ftree-vectorize -pipe -DUSE_EXTERNAL_OMX -DHAVE_LIBBCM_HOST -DUSE_EXTERNAL_LIBBCM_HOST -DUSE_VCHIQ_ARM -Wno-psabi" )
set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wl,--unresolved-symbols=ignore-in-shared-libs -I/opt/vc/include/ -I/opt/vc/include/interface/vmcs_host -I/opt/vc/include/interface/vcos/pthreads -I/opt/vc/include/interface/vmcs_host/linux -L/opt/vc/lib/" )
set( CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} ${CMAKE_CXX_FLAGS} -mfloat-abi=hard -Wl,--unresolved-symbols=ignore-in-shared-libs -I/opt/vc/include/ -I/opt/vc/include/interface/vmcs_host -I/opt/vc/include/interface/vcos/pthreads -I/opt/vc/include/interface/vmcs_host/linux -L/opt/vc/lib/ -Wl,-rpath=/opt/vc/lib" )
set( CMAKE_LD_FLAGS "${CMAKE_LD_FLAGS} -mfloat-abi=hard -Wl,--unresolved-symbols=ignore-in-shared-libs -L/opt/vc/lib/ -Wl,-rpath=/opt/vc/lib" )
set( CMAKE_LINKER_FLAGS "${CMAKE_LD_FLAGS}" )

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wuninitialized -std=gnu++11 -Wno-pmf-conversions -Wno-unused-result -Wno-unused-variable" )
set( CMAKE_LD_FLAGS "${CMAKE_LD_FLAGS} -Wall -std=gnu11" )
set( CMAKE_LINKER_FLAGS "${CMAKE_LINKER_FLAGS} -Wall -std=gnu11" )

include_directories( ${CMAKE_BINARY_DIR} )
include_directories( ${CMAKE_CURRENT_SOURCE_DIR} )
include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/include )
include_directories( /opt/vc/include )

set( SOURCES
	src/Component.cpp
	src/Clock.cpp
	src/Camera.cpp
	src/NullSink.cpp
	src/Resize.cpp
	src/ImageEncode.cpp
	src/VideoSplitter.cpp
	src/VideoRender.cpp
	src/VideoEncode.cpp
	src/VideoDecode.cpp
	src/EGLRender.cpp
)

add_library( "OpenMaxIL++" ${SOURCES} )

target_link_libraries( "OpenMaxIL++" -lopenmaxil -lbcm_host -lvcsm -lvcos -lvchiq_arm -lrt -lpthread -ldl )

add_subdirectory( samples )

set( CMAKE_INSTALL_PREFIX "/opt/vc/" )
install( TARGETS "OpenMaxIL++" LIBRARY )
install( DIRECTORY ${CMAKE_SOURCE_DIR}/include/ DESTINATION "/opt/vc/include/OpenMaxIL++" FILES_MATCHING PATTERN "*.h*" )
