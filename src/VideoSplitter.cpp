#include "VideoSplitter.h"
#include <IL/OMX_Video.h>
#include <IL/OMX_Types.h>
#include <IL/OMX_Component.h>
#include <IL/OMX_Broadcom.h>

using namespace IL;


VideoSplitter::VideoSplitter( bool verbose )
	: Component( "OMX.broadcom.video_splitter", { 250 }, { 251, 252, 253, 254 }, verbose )
{
}


VideoSplitter::~VideoSplitter()
{
}
