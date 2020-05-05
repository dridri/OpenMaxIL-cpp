#include <unistd.h>
#include "../include/Camera.h"
#include "../include/VideoEncode.h"
#include "../include/VideoDecode.h"
#include "../include/VideoRender.h"
#include <iostream>

using namespace IL;

int main( int ac, char** av )
{
	bcm_host_init();
	OMX_Init();

	Camera* camera = new Camera( 1280, 720, 0, true, 0, true );
	VideoEncode* encode = new VideoEncode( 2048, VideoEncode::CodingAVC, true );
	camera->SetupTunnelVideo( encode );
	camera->SetState( Component::StateIdle );
	encode->SetState( Component::StateIdle );

	VideoDecode* decode = new VideoDecode( 0, VideoDecode::CodingAVC, true );
	VideoRender* render = new VideoRender();
	decode->SetupTunnel( render );
	decode->SetState( Component::StateIdle );
	render->SetState( Component::StateIdle );

	camera->SetState( Component::StateExecuting );
	encode->SetState( Component::StateExecuting );
	decode->SetState( Component::StateExecuting );
	render->SetState( Component::StateExecuting );
	camera->SetCapturing( true );

	while ( 1 ) {
		uint8_t data[65536] = { 0 };
		uint32_t datalen = encode->getOutputData( data );

		if ( datalen > 0 ) {
			decode->fillInput( data, datalen );
		}
		usleep( 0 );
	}

	return 0;
}
