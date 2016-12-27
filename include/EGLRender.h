#pragma once

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include "Component.h"

namespace IL {

class EGLRender : public IL::Component
{
public:
	EGLRender( bool verbose = false );
	~EGLRender();

	OMX_ERRORTYPE setEGLImage( EGLImageKHR image );
	OMX_ERRORTYPE sinkToEGL();

protected:
	OMX_BUFFERHEADERTYPE* mBuffer;
};

} // namespace IL
