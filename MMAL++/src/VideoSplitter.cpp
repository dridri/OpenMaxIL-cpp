#include "MMAL++/VideoSplitter.h"

using namespace MMAL;


VideoSplitter::VideoSplitter( bool verbose )
	: Component( "vc.ril.video_splitter", { PortInit( 0, Video ) }, { PortInit( 0, Video ), PortInit( 1, Video ), PortInit( 2, Video ), PortInit( 3, Video ) }, verbose )
{
}


VideoSplitter::~VideoSplitter()
{
}


int VideoSplitter::SetupTunnel( Component* next, uint8_t port_input )
{
	for ( uint32_t i = 0; i < 4; i++ ) {
		if ( mOutputPorts[i].bTunneled == false ) {
			return Component::SetupTunnel( i, next, port_input );
		}
	}
	return -1;
}
