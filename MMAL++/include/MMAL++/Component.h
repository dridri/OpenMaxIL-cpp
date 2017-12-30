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
	typedef enum {
		None = 0x0,
		Clock = 0x1,
		Audio = 0x2,
		Image = 0x4,
		Video = 0x8
	} PortType;

	typedef struct Port {
		Component* pParent;
		uint16_t nPort;
		PortType type;
		bool bEnabled;
		bool bTunneled;
		Component* pTunnel;
		uint16_t nTunnelPort;
		bool bDisableProprietary;
	} Port;

	const std::string& name() const;
	const State state();

	MMAL_STATUS_T EnablePort( MMAL_PORT_T* port, void (*cb)(MMAL_PORT_T*, MMAL_BUFFER_HEADER_T*) = nullptr );
	virtual int SetState( const State& st, bool wait = true );
	int SetupTunnel( uint16_t port_output, Component* next, uint16_t port_input = 0, Port* port_copy = nullptr );
	int DestroyTunnel( uint16_t port_output );
	int AllocateInputBuffer( uint16_t port = 0 );
	int AllocateOutputBuffer( uint16_t port = 0 );

	int DisableProprietaryTunnels( uint16_t port = 0, bool disable = true );

	const bool needData( uint16_t port );
	void fillInput( uint16_t port, uint8_t* pBuf, uint32_t len, bool corrupted = false, bool eof = false );

	const bool dataAvailable( uint16_t port );
	int32_t getOutputData( uint16_t port, uint8_t* pBuf, bool wait = true );

	std::map< uint16_t, Port >& inputPorts() { return mInputPorts; }
	std::map< uint16_t, Port >& outputPorts() { return mOutputPorts; }

protected:
	class PortInit {
	public:
		PortInit() : id(0), type(None) {}
		PortInit(uint16_t i, PortType t) : id(i), type(t) {}
		uint16_t id;
		PortType type;
	};
	Component( const std::string& name, const std::vector< PortInit >& input_ports, const std::vector< PortInit >& output_ports, bool verbose );
	virtual ~Component();

	MMAL_STATUS_T FillPortBuffer( MMAL_PORT_T* port, MMAL_POOL_T* pool );

	std::string mName;
	bool mVerbose;
	MMAL_COMPONENT_T* mHandle;
	std::map< uint16_t, Port > mInputPorts;
	std::map< uint16_t, Port > mOutputPorts;
	static uint64_t ticks64();

private:
	static bool mCoreReady;
	static void onexit();
	static std::list< Component* > mComponents;

	int InitComponent();
	static void BufferCallback( MMAL_PORT_T* port, MMAL_BUFFER_HEADER_T* buffer );
};

}; // namespace MMAL
