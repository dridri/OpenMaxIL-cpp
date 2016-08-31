#pragma once

#include "Component.h"

namespace IL {

class VideoRender : public IL::Component
{
public:
	VideoRender( bool verbose = false );
	~VideoRender();
protected:
};

} // namespace IL
