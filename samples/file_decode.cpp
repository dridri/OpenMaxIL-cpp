#include <unistd.h>
#include "../include/VideoDecode.h"
#include "../include/VideoRender.h"
#include <fstream>

using namespace IL;

int main( int ac, char** av )
{
	bcm_host_init();
	OMX_Init();

	VideoDecode* decode = new VideoDecode( 30, VideoDecode::CodingAVC, true );
	VideoRender* render = new VideoRender( true );
	decode->SetupTunnel( render );
	decode->SetState( Component::StateIdle );
	render->SetState( Component::StateIdle );

	decode->SetState( Component::StateExecuting );
	render->SetState( Component::StateExecuting );

	std::ifstream file( av[1], std::ofstream::in | std::ofstream::binary );

	while ( 1 ) {
		uint8_t data[4096] = { 0 };
		uint32_t datalen = file.readsome( (char*)data, sizeof(data) );

		decode->fillInput( data, datalen );
		usleep( 1000 * 1000 / 30 );
	}

	return 0;
}
