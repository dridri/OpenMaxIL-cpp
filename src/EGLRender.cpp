#include "EGLRender.h"
#include <IL/OMX_Video.h>
#include <IL/OMX_Types.h>
#include <IL/OMX_Component.h>
#include <IL/OMX_Broadcom.h>

using namespace IL;

EGLRender::EGLRender( bool verbose )
	: Component( "OMX.broadcom.egl_render", { PortInit( 220, Video ) }, { PortInit( 221, Video ) }, verbose )
	, mBuffer( nullptr )
{
	SendCommand( OMX_CommandStateSet, OMX_StateIdle, nullptr );
}


EGLRender::~EGLRender()
{
}


OMX_ERRORTYPE EGLRender::setEGLImage( EGLImageKHR image )
{
	SendCommand( OMX_CommandPortEnable, 221, nullptr );
	return ((OMX_COMPONENTTYPE*)mHandle)->UseEGLImage( mHandle, &mBuffer, 221, nullptr, image );
}


OMX_ERRORTYPE EGLRender::sinkToEGL()
{
	if ( state() == StateExecuting and mBuffer != nullptr ) {
		return ((OMX_COMPONENTTYPE*)mHandle)->FillThisBuffer( mHandle, mBuffer );
	}
	return OMX_ErrorIncorrectStateOperation;
}
