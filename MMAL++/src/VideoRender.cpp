#include "MMAL++/VideoRender.h"

using namespace MMAL;

VideoRender::VideoRender( uint32_t offset_x, uint32_t offset_y, uint32_t width, uint32_t height, bool verbose )
	: Component( "vc.ril.video_render", { PortInit( 0, Video ) }, std::vector< PortInit >(), verbose )
{
	MMAL_DISPLAYREGION_T param;
	param.hdr.id = MMAL_PARAMETER_DISPLAYREGION;
	param.hdr.size = sizeof(MMAL_DISPLAYREGION_T);

	param.set = ( MMAL_DISPLAY_SET_FULLSCREEN | MMAL_DISPLAY_SET_NOASPECT | MMAL_DISPLAY_SET_LAYER | ( width != 0 and height != 0 ? MMAL_DISPLAY_SET_DEST_RECT : 0 ) );
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
	MMAL_DISPLAYREGION_T param;
	param.hdr.id = MMAL_PARAMETER_DISPLAYREGION;
	param.hdr.size = sizeof(MMAL_DISPLAYREGION_T);

	param.set = MMAL_DISPLAY_SET_TRANSFORM;
	if ( hrzn and vert ) {
		param.transform = MMAL_DISPLAY_ROT180;
	} else if ( hrzn ) {
		param.transform = MMAL_DISPLAY_MIRROR_ROT0;
	} else if ( vert ) {
		param.transform = MMAL_DISPLAY_MIRROR_ROT180;
	} else {
		param.transform = MMAL_DISPLAY_ROT0;
	}

	return mmal_port_parameter_set( mHandle->input[0], &param.hdr );
}


int VideoRender::setStereo( bool stereo ) {
	MMAL_DISPLAYREGION_T param;
	param.hdr.id = MMAL_PARAMETER_DISPLAYREGION;
	param.hdr.size = sizeof(MMAL_DISPLAYREGION_T);

	param.set = MMAL_DISPLAY_SET_MODE;
	param.mode = MMAL_DISPLAY_MODE_STEREO_LEFT_TO_LEFT;

	return mmal_port_parameter_set( mHandle->input[0], &param.hdr );
}
