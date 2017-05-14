#include "VideoRender.h"
#include <IL/OMX_Video.h>
#include <IL/OMX_Types.h>
#include <IL/OMX_Component.h>
#include <IL/OMX_Broadcom.h>

using namespace IL;

VideoRender::VideoRender( uint32_t offset_x, uint32_t offset_y, uint32_t width, uint32_t height, bool verbose )
	: Component( "OMX.broadcom.video_render", { PortInit( 90, Video ) }, std::vector< PortInit >(), verbose )
{
	OMX_CONFIG_DISPLAYREGIONTYPE region;
	OMX_INIT_STRUCTURE( region );
	region.nPortIndex = 90;
	region.set = (OMX_DISPLAYSETTYPE)( OMX_DISPLAY_SET_FULLSCREEN | OMX_DISPLAY_SET_NOASPECT | OMX_DISPLAY_SET_LAYER | ( width != 0 and height != 0 ? OMX_DISPLAY_SET_DEST_RECT : 0 ) );
	region.fullscreen = (OMX_BOOL)( width == 0 and height == 0 );
	if ( width != 0 and height != 0 ) {
		region.dest_rect.x_offset = offset_x;
		region.dest_rect.y_offset = offset_y;
		region.dest_rect.width = width;
		region.dest_rect.height = height;
	}
	region.noaspect = OMX_TRUE;
	region.layer = 0;
	SetConfig( OMX_IndexConfigDisplayRegion, &region );

// 	SendCommand( OMX_CommandStateSet, OMX_StateIdle, nullptr );
}


VideoRender::~VideoRender()
{
}


OMX_ERRORTYPE VideoRender::setMirror( bool hrzn, bool vert )
{
	OMX_MIRRORTYPE eMirror = OMX_MirrorNone;
	if ( hrzn and not vert ) {
		eMirror = OMX_MirrorHorizontal;
	} else if ( vert and not hrzn ) {
		eMirror = OMX_MirrorVertical;
	} else if ( hrzn and vert ) {
		eMirror = OMX_MirrorBoth;
	}

	OMX_CONFIG_MIRRORTYPE mirror;
	OMX_INIT_STRUCTURE(mirror);
	mirror.nPortIndex = 90;
	mirror.eMirror = eMirror;
	return SetConfig( OMX_IndexConfigCommonMirror, &mirror );
}


OMX_ERRORTYPE VideoRender::setStereo( bool stereo )
{
	OMX_CONFIG_DISPLAYREGIONTYPE region;
	OMX_INIT_STRUCTURE( region );
	region.nPortIndex = 90;
	region.set = OMX_DISPLAY_SET_MODE;
	region.mode = OMX_DISPLAY_MODE_STEREO_LEFT_TO_LEFT;
	return SetConfig( OMX_IndexConfigDisplayRegion, &region );
}
