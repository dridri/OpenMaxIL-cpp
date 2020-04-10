#pragma once

#include "Component.h"

namespace IL {

class Clock : public IL::Component
{
public:
	Clock( bool verbose = false );
	~Clock();

	OMX_ERRORTYPE SetupTunnel( Component* next, uint8_t port_input = 0 );

protected:
};

} // namespace IL
