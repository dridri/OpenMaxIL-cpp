#include <unistd.h>
#include "../include/Camera.h"
#include "../include/VideoRender.h"

using namespace MMAL;

int main( int ac, char** av )
{
	bcm_host_init();

	Camera* camera = new Camera( 1920, 1080, 0, true, true );
	VideoRender* render = new VideoRender( true );
	camera->SetupTunnelVideo( render );

	camera->SetState( Component::StateExecuting );
	render->SetState( Component::StateExecuting );

	while ( 1 ) {
		usleep( 1000 * 1000 );
	}

	return 0;
}
