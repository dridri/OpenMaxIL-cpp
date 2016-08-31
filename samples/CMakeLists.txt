set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_BSD_SOURCE -D_GNU_SOURCE -DSTANDALONE -D__STDC_CONSTANT_MACROS -D__STDC_LIMIT_MACROS -DTARGET_POSIX -D_LINUX -fPIC -DPIC -D_REENTRANT -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 -U_FORTIFY_SOURCE" )
set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DHAVE_LIBOPENMAX=2 -DOMX -DOMX_SKIP64BIT -ftree-vectorize -pipe -DUSE_EXTERNAL_OMX -DHAVE_LIBBCM_HOST -DUSE_EXTERNAL_LIBBCM_HOST -DUSE_VCHIQ_ARM -Wno-psabi" )
set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wl,--unresolved-symbols=ignore-in-shared-libs -I/opt/vc/include/ -I/opt/vc/include/interface/vmcs_host -I/opt/vc/include/interface/vcos/pthreads -I/opt/vc/include/interface/vmcs_host/linux -L/opt/vc/lib/" )
set( CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} ${CMAKE_CXX_FLAGS} -mfloat-abi=hard -Wl,--unresolved-symbols=ignore-in-shared-libs" )
set( CMAKE_LD_FLAGS "${CMAKE_LD_FLAGS} -mfloat-abi=hard -Wl,--unresolved-symbols=ignore-in-shared-libs -L/opt/vc/lib/ -Wl,-rpath=/opt/vc/lib" )
set( CMAKE_LINKER_FLAGS "${CMAKE_LD_FLAGS}" )

set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wuninitialized -std=gnu11 -fgnu89-inline" )
set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wuninitialized -std=gnu++11 -Wno-pmf-conversions -Wno-unused-result" )
set( CMAKE_LD_FLAGS "${CMAKE_LD_FLAGS} -Wall -std=gnu11" )
set( CMAKE_LINKER_FLAGS "${CMAKE_LINKER_FLAGS} -Wall -std=gnu11" )

include_directories( ${CMAKE_SOURCE_DIR}/../include )

add_executable( camera_live camera_live.cpp )
target_link_libraries( camera_live "OpenMaxIL++" -lopenmaxil -lbcm_host -lvcos -lvchiq_arm -lrt -lpthread -ldl )

add_executable( camera_encode camera_encode.cpp )
target_link_libraries( camera_encode "OpenMaxIL++" -lopenmaxil -lbcm_host -lvcos -lvchiq_arm -lrt -lpthread -ldl )
