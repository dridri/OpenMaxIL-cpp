#pragma once

#include "Component.h"

namespace IL {

class Resize : public IL::Component
{
public:
	Resize( uint32_t width, uint32_t height, bool verbose = false );
	~Resize();

	uint32_t width() const;
	uint32_t height() const;

protected:
	uint32_t mWidth;
	uint32_t mHeight;
};

} // namespace IL
