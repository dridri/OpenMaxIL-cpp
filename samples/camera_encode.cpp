#include <unistd.h>
#include "../include/Camera.h"
#include "../include/VideoEncode.h"
#include <fstream>

using namespace IL;

int main( int ac, char** av )
{
	bcm_host_init();
	OMX_Init();

	Camera* camera = new Camera( 1280, 720, 0, true, true );
	VideoEncode* encode = new VideoEncode( 4 * 1024, VideoEncode::CodingAVC, true );
	camera->SetupTunnel( 71, encode, 200 );

	camera->SetState( Component::StateExecuting );
	encode->SetState( Component::StateExecuting );

	std::ofstream file( "test.h264", std::ofstream::out | std::ofstream::binary );

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
