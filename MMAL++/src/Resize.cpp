#include "Resize.h"

using namespace MMAL;

Resize::Resize( uint32_t width, uint32_t height, bool verbose )
	: Component( "vc.ril.resize", { 0 }, { 0 }, verbose )
	, mWidth( width )
	, mHeight( height )
{
	mHandle->output[0]->format->es->video.frame_rate.num = 0;
	mHandle->output[0]->format->es->video.frame_rate.den = 1;
	mHandle->output[0]->format->es->video.crop.width = width;
	mHandle->output[0]->format->es->video.crop.height = height;
	mHandle->output[0]->format->es->video.width = VCOS_ALIGN_UP(width, 32);
	mHandle->output[0]->format->es->video.height = VCOS_ALIGN_UP(height, 16);
	mHandle->output[0]->format->encoding = MMAL_ENCODING_OPAQUE;
	mHandle->output[0]->format->encoding_variant = MMAL_ENCODING_I420;
	mmal_port_format_commit( mHandle->output[0] );
}


Resize::~Resize()
{
}


int Resize::SetupTunnel( Component* next, uint8_t port_input )
{
	return Component::SetupTunnel( 0, next, port_input );
}


uint32_t Resize::width() const
{
	return mWidth;
}


uint32_t Resize::height() const
{
	return mHeight;
}
