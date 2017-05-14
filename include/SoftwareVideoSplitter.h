#ifndef IL_SOFTWAREVIDEOSPLITTER_H
#define IL_SOFTWAREVIDEOSPLITTER_H

#include <thread>
#include "Component.h"

namespace IL {

class SoftwareVideoSplitter : public IL::Component
{
public:
	SoftwareVideoSplitter( bool verbose = false );
	~SoftwareVideoSplitter();

	virtual OMX_ERRORTYPE SetState( const State& st );
	OMX_ERRORTYPE SetupTunnel( Component* next, uint8_t port_input = 0 );

protected:
	virtual bool CustomTunnelInput( Component* prev, uint16_t port_output, uint16_t port_input );
	void ThreadRun();

	bool mPaused;
	std::thread* mThread;
	uint16_t mLastOutput;
};

}

#endif // IL_SOFTWAREVIDEOSPLITTER_H
