#include "VideoRender.h"

using namespace MMAL;

VideoRender::VideoRender( uint32_t offset_x, uint32_t offset_y, uint32_t width, uint32_t height, bool verbose )
	: Component( "vc.ril.video_render", { 0 }, std::vector<uint8_t>(), verbose )
{
	MMAL_DISPLAYREGION_T param;
	param.hdr.id = MMAL_PARAMETER_DISPLAYREGION;
	param.hdr.size = sizeof(MMAL_DISPLAYREGION_T);

	param.set = (MMAL_DISPLAYSETTYPE)( MMAL_DISPLAY_SET_FULLSCREEN | MMAL_DISPLAY_SET_NOASPECT | MMAL_DISPLAY_SET_LAYER | ( width != 0 and height != 0 ? MMAL_DISPLAY_SET_DEST_RECT : 0 ) );
	param.fullscreen = (MMAL_BOOL_T)( width == 0 and height == 0 );
	if ( width != 0 and height != 0 ) {
		param.dest_rect.x = offset_x;
		param.dest_rect.y = offset_y;
		param.dest_rect.width = width;
		param.dest_rect.height = height;
	}
	param.noaspect = MMAL_TRUE;
	param.layer = 0;

	mmal_port_parameter_set( mHandle->input[0], &param.hdr );
}


VideoRender::~VideoRender()
{
}


int VideoRender::setMirror( bool hrzn, bool vert )
{
	MMAL_PARAMETER_MIRROR_T mirror;
	mirror.hdr.id = MMAL_PARAMETER_MIRROR;
	mirror.hdr.size = sizeof(MMAL_PARAM_MIRROR_T);
	if ( hrzn and vert ) {
		mirror.value = MMAL_PARAM_MIRROR_BOTH;
	} else if ( hrzn ) {
		mirror.value = MMAL_PARAM_MIRROR_HORIZONTAL;
	} else if ( vert ) {
		mirror.value = MMAL_PARAM_MIRROR_VERTICAL;
	} else {
		mirror.value = MMAL_PARAM_MIRROR_NONE;
	}
	return mmal_port_parameter_set( mHandle->input[0], &mirror.hdr );
}
