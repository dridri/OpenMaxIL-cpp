#include <unistd.h>
#include "../include/Camera.h"
#include "../include/VideoRender.h"

using namespace IL;

int main( int ac, char** av )
{
	bcm_host_init();
	OMX_Init();

	Camera* camera = new Camera( 1920, 1080, 0, true, true );
	VideoRender* render = new VideoRender( true );
	camera->SetupTunnel( 71, render, 90 );

	camera->SetState( Component::StateExecuting );
	render->SetState( Component::StateExecuting );

	while ( 1 ) {
		usleep( 1000 * 1000 );
	}

	return 0;
}
