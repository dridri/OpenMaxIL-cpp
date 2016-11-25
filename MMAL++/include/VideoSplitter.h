#pragma once

#include "Component.h"

namespace MMAL {

class VideoSplitter : public MMAL::Component
{
public:
	VideoSplitter( bool verbose = false );
	~VideoSplitter();
	int SetupTunnel( Component* next, uint8_t port_input = 0 );

protected:
};

} // namespace MMAL
