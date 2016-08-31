#pragma once

#include "Component.h"

namespace IL {

class NullSink : public IL::Component
{
public:
	NullSink( bool verbose = false );
	~NullSink();
};

} // namespace IL
