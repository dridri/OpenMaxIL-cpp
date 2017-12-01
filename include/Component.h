#pragma once

#include <string.h>
#include <IL/OMX_Types.h>
#include <IL/OMX_Core.h>
#include <IL/OMX_Component.h>
#include <string>
#include <vector>
#include <list>
#include <map>
#include <mutex>
#include <condition_variable>
#include <functional>

extern "C" void bcm_host_init( void );

namespace IL {

class Component {
public:
	typedef enum {
		StateInvalid,
		StateLoaded,
		StateIdle,
		StateExecuting,
		StatePause,
		StateWaitForResources
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
		bool bufferRunning;
		bool bufferNeedsData;
		bool bufferDataAvailable;
		OMX_BUFFERHEADERTYPE* buffer;
		OMX_BUFFERHEADERTYPE* buffer_copy;
	} Port;

	const std::string& name() const;
	const State state();

	OMX_ERRORTYPE EnablePort( uint16_t port );
	virtual OMX_ERRORTYPE SetState( const State& st, bool wait = true );
	OMX_ERRORTYPE SetupTunnel( uint16_t port_output, Component* next, uint16_t port_input = 0, Port* port_copy = nullptr );
	OMX_ERRORTYPE DestroyTunnel( uint16_t port_output );
	OMX_ERRORTYPE AllocateInputBuffer( uint16_t port = 0 );
	OMX_ERRORTYPE AllocateOutputBuffer( uint16_t port = 0 );

	OMX_ERRORTYPE DisableProprietaryTunnels( uint8_t port = 0, bool disable = true );

	OMX_ERRORTYPE SendCommand( OMX_COMMANDTYPE Cmd, OMX_U32 nParam1, OMX_PTR pCmdData );
	OMX_ERRORTYPE GetParameter( OMX_INDEXTYPE nParamIndex, OMX_PTR pComponentParameterStructure );
	OMX_ERRORTYPE SetParameter( OMX_INDEXTYPE nIndex,OMX_PTR pComponentParameterStructure );
	OMX_ERRORTYPE GetConfig( OMX_INDEXTYPE nIndex, OMX_PTR pComponentConfigStructure );
	OMX_ERRORTYPE SetConfig( OMX_INDEXTYPE nIndex, OMX_PTR pComponentConfigStructure );

	const bool needData( uint16_t port );
	void fillInput( uint16_t port, uint8_t* pBuf, uint32_t len, bool corrupted = false, bool eof = false );

	const bool dataAvailable( uint16_t port );
	int32_t getOutputData( uint16_t port, uint8_t* pBuf, bool wait = true );

	std::map< uint16_t, Port >& inputPorts() { return mInputPorts; }
	std::map< uint16_t, Port >& outputPorts() { return mOutputPorts; }

	static OMX_ERRORTYPE CopyPort( Port* from, Port* to );
	static void setDebugOutputCallback( void (*callback)( int level, const std::string fmt, ... ) );
	static void DestroyAll();

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

	virtual OMX_ERRORTYPE EventHandler( OMX_EVENTTYPE event, OMX_U32 data1, OMX_U32 data2, OMX_PTR eventdata );
	virtual OMX_ERRORTYPE EmptyBufferDone( OMX_BUFFERHEADERTYPE* buf );
	virtual OMX_ERRORTYPE FillBufferDone( OMX_BUFFERHEADERTYPE* buf );

	virtual bool CustomTunnelInput( Component* prev, uint16_t port_output, uint16_t port_input ) { return false; }

	OMX_ERRORTYPE AllocateBuffers( OMX_BUFFERHEADERTYPE** buffer, int port, bool enable );
	void omx_block_until_port_changed( OMX_U32 nPortIndex, OMX_BOOL bEnabled );
	void omx_block_until_state_changed( OMX_STATETYPE state );

	template<typename T> static void OMX_INIT_STRUCTURE( T& omx_struct )
	{
		memset( &omx_struct, 0, sizeof(omx_struct) );
		omx_struct.nSize = sizeof(omx_struct);
		omx_struct.nVersion.nVersion = OMX_VERSION;
		omx_struct.nVersion.s.nVersionMajor = OMX_VERSION_MAJOR;
		omx_struct.nVersion.s.nVersionMinor = OMX_VERSION_MINOR;
		omx_struct.nVersion.s.nRevision = OMX_VERSION_REVISION;
		omx_struct.nVersion.s.nStep = OMX_VERSION_STEP;
	}

	std::string mName;
	bool mVerbose;
	State mState;
	OMX_HANDLETYPE mHandle;
	std::map< uint16_t, Port > mInputPorts;
	std::map< uint16_t, Port > mOutputPorts;
	std::list< OMX_U8* > mAllocatedBuffers;

	std::mutex mDataAvailableMutex;
	std::condition_variable mDataAvailableCond;

	static uint64_t ticks64();
	static void (*mDebugCallback)( int level, const std::string fmt, ... );
	static void DefaultDebugCallback( int level, const std::string fmt, ... );

private:
	static OMX_ERRORTYPE genericeventhandler( OMX_HANDLETYPE handle, Component* component, OMX_EVENTTYPE event, OMX_U32 data1, OMX_U32 data2, OMX_PTR eventdata );
	static OMX_ERRORTYPE emptied( OMX_HANDLETYPE handle, Component* component, OMX_BUFFERHEADERTYPE* buf );
	static OMX_ERRORTYPE filled( OMX_HANDLETYPE handle, Component* component, OMX_BUFFERHEADERTYPE* buf );
	static bool mCoreReady;
	static std::list< OMX_U8* > mAllAllocatedBuffers;
	static std::list< Component* > mComponents;

	int InitComponent();

	static void print_def( OMX_PARAM_PORTDEFINITIONTYPE def );
};

}; // namespace IL
