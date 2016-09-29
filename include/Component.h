#pragma once

#include <string.h>
#include <IL/OMX_Types.h>
#include <IL/OMX_Core.h>
#include <string>
#include <vector>
#include <list>
#include <map>

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

	const std::string& name() const;
	const State state();

	virtual OMX_ERRORTYPE SetState( const State& st );
	OMX_ERRORTYPE SetupTunnel( uint8_t port_output, Component* next, uint8_t port_input );

	OMX_ERRORTYPE SendCommand( OMX_COMMANDTYPE Cmd, OMX_U32 nParam1, OMX_PTR pCmdData );
	OMX_ERRORTYPE GetParameter( OMX_INDEXTYPE nParamIndex, OMX_PTR pComponentParameterStructure );
	OMX_ERRORTYPE SetParameter( OMX_INDEXTYPE nIndex,OMX_PTR pComponentParameterStructure );
	OMX_ERRORTYPE GetConfig( OMX_INDEXTYPE nIndex, OMX_PTR pComponentConfigStructure );
	OMX_ERRORTYPE SetConfig( OMX_INDEXTYPE nIndex, OMX_PTR pComponentConfigStructure );

protected:
	Component( const std::string& name, const std::vector< uint8_t >& input_ports, const std::vector< uint8_t >& output_ports, bool verbose );
	virtual ~Component();

	virtual OMX_ERRORTYPE EventHandler( OMX_EVENTTYPE event, OMX_U32 data1, OMX_U32 data2, OMX_PTR eventdata );
	virtual OMX_ERRORTYPE EmptyBufferDone( OMX_BUFFERHEADERTYPE* buf );
	virtual OMX_ERRORTYPE FillBufferDone( OMX_BUFFERHEADERTYPE* buf );

	OMX_ERRORTYPE AllocateBuffers( OMX_BUFFERHEADERTYPE** buffer, int port, bool enable );
	void omx_block_until_port_changed( OMX_U32 nPortIndex, OMX_BOOL bEnabled );
	void omx_block_until_state_changed( OMX_STATETYPE state );

	template<typename T> void OMX_INIT_STRUCTURE( T& omx_struct )
	{
		memset( &omx_struct, 0, sizeof(omx_struct) );
		omx_struct.nSize = sizeof(omx_struct);
		omx_struct.nVersion.nVersion = OMX_VERSION;
		omx_struct.nVersion.s.nVersionMajor = OMX_VERSION_MAJOR;
		omx_struct.nVersion.s.nVersionMinor = OMX_VERSION_MINOR;
		omx_struct.nVersion.s.nRevision = OMX_VERSION_REVISION;
		omx_struct.nVersion.s.nStep = OMX_VERSION_STEP;
	}

	typedef struct Port {
		uint8_t nPort;
		bool bEnabled;
		bool bTunneled;
		Component* pTunnel;
		uint8_t nTunnelPort;
	} Port;

	std::string mName;
	bool mVerbose;
	State mState;
	OMX_HANDLETYPE mHandle;
	std::map< uint8_t, Port > mInputPorts;
	std::map< uint8_t, Port > mOutputPorts;

private:
	static OMX_ERRORTYPE genericeventhandler( OMX_HANDLETYPE handle, Component* component, OMX_EVENTTYPE event, OMX_U32 data1, OMX_U32 data2, OMX_PTR eventdata );
	static OMX_ERRORTYPE emptied( OMX_HANDLETYPE handle, Component* component, OMX_BUFFERHEADERTYPE* buf );
	static OMX_ERRORTYPE filled( OMX_HANDLETYPE handle, Component* component, OMX_BUFFERHEADERTYPE* buf );
	static bool mCoreReady;
	static void onexit();
	static std::list< OMX_U8* > mAllocatedBuffers;
	static std::list< Component* > mComponents;

	int InitComponent();
};

}; // namespace IL
