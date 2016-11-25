#include "Resize.h"
#include <IL/OMX_Video.h>
#include <IL/OMX_Types.h>
#include <IL/OMX_Component.h>
#include <IL/OMX_Broadcom.h>

using namespace IL;

Resize::Resize( uint32_t width, uint32_t height, bool verbose )
	: Component( "OMX.broadcom.resize", { PortInit( 60, (PortType)(Image | Video) ) }, { PortInit( 61, (PortType)(Image | Video) ) }, verbose )
	, mWidth( width )
	, mHeight( height )
{
	OMX_PARAM_PORTDEFINITIONTYPE imgportdef;

	OMX_INIT_STRUCTURE( imgportdef );
	imgportdef.nPortIndex = 61;
	GetParameter( OMX_IndexParamPortDefinition, &imgportdef );
	imgportdef.format.image.nFrameWidth  = mWidth;
	imgportdef.format.image.nFrameHeight = mHeight;
	imgportdef.format.image.nStride      = ( imgportdef.format.video.nFrameWidth + imgportdef.nBufferAlignment - 1 ) & ( ~( imgportdef.nBufferAlignment - 1 ) );
	imgportdef.format.video.nSliceHeight = 0;
	imgportdef.format.image.nStride      = 0;
	imgportdef.format.image.eColorFormat = OMX_COLOR_FormatYUV420PackedPlanar;
	SetParameter( OMX_IndexParamPortDefinition, &imgportdef );

	// TBD
/*
	OMX_PARAM_RESIZETYPE resize;
	OMX_INIT_STRUCTURE( resize );
	resize.nPortIndex = 61;
	resize.eMode = OMX_RESIZE_BOX;
	resize.nMaxWidth = PREV_WIDTH;
	resize.nMaxHeight = PREV_HEIGHT;
	resize.nMaxBytes = PREV_WIDTH * PREV_HEIGHT * 16;
	resize.bPreserveAspectRatio = OMX_TRUE;
	resize.bAllowUpscaling = OMX_FALSE;
	SetParameter( OMX_IndexParamResize, &resize );
*/
	SendCommand( OMX_CommandStateSet, OMX_StateIdle, nullptr );
}


Resize::~Resize()
{
}


OMX_ERRORTYPE Resize::SetupTunnel( Component* next, uint8_t port_input )
{
	return Component::SetupTunnel( 61, next, port_input );
}


uint32_t Resize::width() const
{
	return mWidth;
}


uint32_t Resize::height() const
{
	return mHeight;
}
