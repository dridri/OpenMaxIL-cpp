#pragma once

#include "Component.h"

namespace IL {

class VideoSplitter : public IL::Component
{
public:
	VideoSplitter( bool verbose = false );
	~VideoSplitter();
};

} // namespace IL
