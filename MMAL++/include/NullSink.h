#pragma once

#include "Component.h"

namespace MMAL {

class NullSink : public MMAL::Component
{
public:
	NullSink( bool verbose = false );
	~NullSink();
};

} // namespace MMAL
