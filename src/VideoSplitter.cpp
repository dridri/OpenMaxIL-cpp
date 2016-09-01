#include "VideoSplitter.h"
#include <IL/OMX_Video.h>
#include <IL/OMX_Types.h>
#include <IL/OMX_Component.h>
#include <IL/OMX_Broadcom.h>

using namespace IL;


VideoSplitter::VideoSplitter( bool verbose )
	: Component( "OMX.broadcom.video_splitter", { 250 }, { 251, 252, 253, 254 }, verbose )
{
	SendCommand( OMX_CommandStateSet, OMX_StateIdle, nullptr );
}


VideoSplitter::~VideoSplitter()
{
}


OMX_ERRORTYPE VideoSplitter::EventHandler( OMX_EVENTTYPE event, OMX_U32 data1, OMX_U32 data2, OMX_PTR eventdata )
{
	return Component::EventHandler( event, data1, data2, eventdata );
}
