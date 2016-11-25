#pragma once

#include "Component.h"

namespace MMAL {

class VideoRender : public MMAL::Component
{
public:
	VideoRender( uint32_t offset_x = 0, uint32_t offset_y = 0, uint32_t width = 0, uint32_t height = 0, bool verbose = false );
	~VideoRender();

	int setMirror( bool hrzn, bool vert );
protected:
};

} // namespace MMAL
