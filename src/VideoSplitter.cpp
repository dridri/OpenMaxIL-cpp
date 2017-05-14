#include "VideoSplitter.h"
#include <IL/OMX_Video.h>
#include <IL/OMX_Types.h>
#include <IL/OMX_Component.h>
#include <IL/OMX_Broadcom.h>

using namespace IL;


VideoSplitter::VideoSplitter( bool verbose )
	: Component( "OMX.broadcom.video_splitter", { PortInit( 250, Video ) }, { PortInit( 251, Video ), PortInit( 252, Video ), PortInit( 253, Video ), PortInit( 254, Video ) }, verbose )
{

	OMX_CONFIG_BRCMUSEPROPRIETARYCALLBACKTYPE propCb;
	OMX_INIT_STRUCTURE( propCb );
	propCb.nPortIndex = 250;
	propCb.bEnable = OMX_TRUE;
	SetConfig( OMX_IndexConfigBrcmUseProprietaryCallback, &propCb );

	OMX_PARAM_BRCMDISABLEPROPRIETARYTUNNELSTYPE tunnels;
	OMX_INIT_STRUCTURE( tunnels );
	tunnels.bUseBuffers = OMX_TRUE;
	tunnels.nPortIndex = 251;
	SetParameter( OMX_IndexParamBrcmDisableProprietaryTunnels, &tunnels );
	tunnels.nPortIndex = 252;
	SetParameter( OMX_IndexParamBrcmDisableProprietaryTunnels, &tunnels );
	tunnels.nPortIndex = 253;
	SetParameter( OMX_IndexParamBrcmDisableProprietaryTunnels, &tunnels );
	tunnels.nPortIndex = 254;
	SetParameter( OMX_IndexParamBrcmDisableProprietaryTunnels, &tunnels );
/*
	OMX_PARAM_IMAGEPOOLSIZETYPE pool;
	OMX_INIT_STRUCTURE( pool );
	pool.num_pages = 4;
	SetParameter( OMX_IndexParamImagePoolSize, &pool );
*/
	for ( uint32_t i = 250; i <= 254; i++ ) {
		OMX_PARAM_PORTDEFINITIONTYPE def;
		OMX_INIT_STRUCTURE( def );
		def.nPortIndex = i;
		GetParameter( OMX_IndexParamPortDefinition, &def );
		def.format.video.nFrameWidth = 1280;
		def.format.video.nFrameHeight = 720;
		def.format.video.nStride = ( def.format.video.nFrameWidth + def.nBufferAlignment - 1 ) & ( ~(def.nBufferAlignment - 1) );
		def.format.video.nSliceHeight = 720;
		def.format.video.xFramerate = 30 << 16;
// 		def.format.video.eColorFormat = OMX_COLOR_FormatYUV420PackedPlanar;
		SetParameter( OMX_IndexParamPortDefinition, &def );
	}

// 	SendCommand( OMX_CommandStateSet, OMX_StateIdle, nullptr );
}


VideoSplitter::~VideoSplitter()
{
}


OMX_ERRORTYPE VideoSplitter::SetupTunnel( Component* next, uint8_t port_input )
{
	for ( uint32_t i = 251; i <= 254; i++ ) {
		if ( mOutputPorts[i].bTunneled == false ) {
			return Component::SetupTunnel( mOutputPorts[i].nPort, next, port_input/*, &mInputPorts[250].pTunnel->outputPorts()[mInputPorts[250].nTunnelPort]*/ );
		}
	}
	return OMX_ErrorNoMore;
}


OMX_ERRORTYPE VideoSplitter::EventHandler( OMX_EVENTTYPE event, OMX_U32 data1, OMX_U32 data2, OMX_PTR eventdata )
{
	if ( data1 == 251 and event == OMX_EventPortSettingsChanged and state() == StateIdle ) {
		SetState( StateExecuting );
	}
	return Component::EventHandler( event, data1, data2, eventdata );
}
