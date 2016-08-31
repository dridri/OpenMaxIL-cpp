#include "VideoRender.h"
#include <IL/OMX_Video.h>
#include <IL/OMX_Types.h>
#include <IL/OMX_Component.h>
#include <IL/OMX_Broadcom.h>

using namespace IL;

VideoRender::VideoRender( bool verbose )
	: Component( "OMX.broadcom.video_render", { 90 }, std::vector< uint8_t >(), verbose )
{
	OMX_CONFIG_DISPLAYREGIONTYPE region;
	OMX_INIT_STRUCTURE( region );
	region.nPortIndex = 90;
	region.set = (OMX_DISPLAYSETTYPE)( OMX_DISPLAY_SET_FULLSCREEN | OMX_DISPLAY_SET_NOASPECT | OMX_DISPLAY_SET_LAYER );
	region.fullscreen = OMX_TRUE;
	region.noaspect = OMX_TRUE;
	region.layer = 0;
	SetConfig( OMX_IndexConfigDisplayRegion, &region );

	SendCommand( OMX_CommandStateSet, OMX_StateIdle, nullptr );
}


VideoRender::~VideoRender()
{
}
