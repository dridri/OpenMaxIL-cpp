#pragma once

#include <string.h>
#include <string>
#include <vector>
#include <list>
#include <map>
#include <functional>

#include <interface/vcos/vcos.h>
#include <interface/mmal/mmal.h>
#include <interface/mmal/mmal_buffer.h>
#include <interface/mmal/mmal_logging.h>
#include <interface/mmal/util/mmal_util.h>
#include <interface/mmal/util/mmal_util_params.h>
#include <interface/mmal/util/mmal_connection.h>

extern "C" void bcm_host_init( void );

namespace MMAL {

class Component {
public:
	typedef enum {
		StateInvalid,
		StateLoaded,
		StateIdle,
		StateExecuting
	} State;
	const std::string& name() const;

	virtual int SetState( const State& st );
	int SetupTunnel( uint8_t port_output, Component* next, uint8_t port_input );

protected:
	Component( const std::string& name, const std::vector< uint8_t >& input_ports, const std::vector< uint8_t >& output_ports, bool verbose );
	virtual ~Component();

	MMAL_STATUS_T FillPortBuffer( MMAL_PORT_T* port, MMAL_POOL_T* pool );
	MMAL_STATUS_T EnablePort( MMAL_PORT_T* port, void (*cb)(MMAL_PORT_T*, MMAL_BUFFER_HEADER_T*) = nullptr );

	typedef struct Port {
		uint8_t nPort;
		bool bEnabled;
		bool bTunneled;
		Component* pTunnel;
		uint8_t nTunnelPort;
	} Port;

	std::string mName;
	bool mVerbose;
	MMAL_COMPONENT_T* mHandle;
	std::map< uint8_t, Port > mInputPorts;
	std::map< uint8_t, Port > mOutputPorts;
	static uint64_t ticks64();

private:
	static bool mCoreReady;
	static void onexit();
	static std::list< Component* > mComponents;

	int InitComponent();
	static void BufferCallback( MMAL_PORT_T* port, MMAL_BUFFER_HEADER_T* buffer );
};

}; // namespace MMAL
