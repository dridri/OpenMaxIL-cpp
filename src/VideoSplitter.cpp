#include "VideoSplitter.h"
#include <IL/OMX_Video.h>
#include <IL/OMX_Types.h>
#include <IL/OMX_Component.h>
#include <IL/OMX_Broadcom.h>

using namespace IL;


VideoSplitter::VideoSplitter( bool verbose )
	: Component( "OMX.broadcom.video_splitter", { PortInit( 250, Video ) }, { PortInit( 251, Video ), PortInit( 252, Video ), PortInit( 253, Video ), PortInit( 254, Video ) }, verbose )
{
	SendCommand( OMX_CommandStateSet, OMX_StateIdle, nullptr );
}


VideoSplitter::~VideoSplitter()
{
}


OMX_ERRORTYPE VideoSplitter::SetupTunnel( Component* next, uint8_t port_input )
{
	for ( uint32_t i = 0; i < 4; i++ ) {
		if ( mOutputPorts[i].bTunneled == false ) {
			return Component::SetupTunnel( mOutputPorts[i].nPort, next, port_input );
		}
	}
	return OMX_ErrorNoMore;
}


OMX_ERRORTYPE VideoSplitter::EventHandler( OMX_EVENTTYPE event, OMX_U32 data1, OMX_U32 data2, OMX_PTR eventdata )
{
	return Component::EventHandler( event, data1, data2, eventdata );
}
