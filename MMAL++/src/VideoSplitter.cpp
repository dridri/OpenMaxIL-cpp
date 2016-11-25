#include "VideoSplitter.h"

using namespace MMAL;


VideoSplitter::VideoSplitter( bool verbose )
	: Component( "vc.ril.video_splitter", { 0 }, { 0, 1, 2, 3 }, verbose )
{
}


VideoSplitter::~VideoSplitter()
{
}


int VideoSplitter::SetupTunnel( Component* next, uint8_t port_input )
{
	for ( uint32_t i = 0; i < 4; i++ ) {
		if ( mOutputPorts[i].bTunneled == false ) {
			return Component::SetupTunnel( mOutputPorts[i].nPort, next, port_input );
		}
	}
	return -1;
}
