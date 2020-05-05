#include <unistd.h>
#include "../include/Camera.h"
#include "../include/VideoRender.h"

using namespace IL;

int main( int ac, char** av )
{
	bcm_host_init();
	OMX_Init();

// 	Camera* camera = new Camera( 1280, 720, 0, true, 0, true );
	Camera* camera = new Camera( 1640, 1232, 0, true, 4, true );
	VideoRender* render = new VideoRender();

	camera->SetupTunnelVideo( render );
	camera->SetState( Component::StateIdle );
	render->SetState( Component::StateIdle );

	camera->SetState( Component::StateExecuting );
	render->SetState( Component::StateExecuting );
	camera->SetCapturing( true );

	printf( "Ok\n" );

	while ( 1 ) {
		usleep( 1000 * 1000 );
	}

	return 0;
}




