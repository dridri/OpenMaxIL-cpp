#include <unistd.h>
#include "../include/Camera.h"
#include "../include/VideoEncode.h"
#include <fstream>
#include <iostream>

using namespace IL;

int main( int ac, char** av )
{
	bcm_host_init();
	OMX_Init();

	Camera* camera = new Camera( 1280, 720, 0, true, 0, true );
	VideoEncode* encode = new VideoEncode( 4 * 1024, VideoEncode::CodingAVC, true );
	camera->SetupTunnelVideo( encode );
	camera->SetState( Component::StateIdle );
	encode->SetState( Component::StateIdle );

	camera->SetState( Component::StateExecuting );
	encode->SetState( Component::StateExecuting );
	camera->SetCapturing( true );

	std::ofstream file( av[1], std::ofstream::out | std::ofstream::binary );

	while ( 1 ) {
		uint8_t data[65536] = { 0 };
		uint32_t datalen = encode->getOutputData( data );

		if ( datalen > 0 ) {
			file.write( (char*)data, datalen );
			file.flush();
		}
	}

	return 0;
}
