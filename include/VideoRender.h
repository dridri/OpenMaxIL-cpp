#pragma once

#include "Component.h"

namespace IL {

class VideoRender : public IL::Component
{
public:
	VideoRender( int32_t offset_x = 0, int32_t offset_y = 0, int32_t width = 0, int32_t height = 0, bool verbose = false );
	~VideoRender();

	OMX_ERRORTYPE setMirror( bool hrzn, bool vert );
	OMX_ERRORTYPE setStereo( bool stereo );
protected:
};

} // namespace IL
