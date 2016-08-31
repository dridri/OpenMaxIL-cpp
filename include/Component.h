#pragma once

#include <IL/OMX_Types.h>
#include <IL/OMX_Core.h>
#include <string>
#include <vector>

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

	OMX_ERRORTYPE SetupTunnel( uint8_t port_output, Component* next, uint8_t port_input );

protected:
	Component( const std::string& name, const std::vector< uint8_t >& input_ports, const std::vector< uint8_t >& output_ports, bool verbose );
	virtual ~Component();

	virtual OMX_ERRORTYPE EventHandler( OMX_EVENTTYPE event, OMX_U32 data1, OMX_U32 data2, OMX_PTR eventdata );
	virtual OMX_ERRORTYPE EmptyBufferDone( OMX_BUFFERHEADERTYPE* buf );
	virtual OMX_ERRORTYPE FillBufferDone( OMX_BUFFERHEADERTYPE* buf );

	template<typename T> void OMX_INIT_STRUCTURE( T& omx_struct );
	OMX_ERRORTYPE SendCommand( OMX_COMMANDTYPE Cmd, OMX_U32 nParam1, OMX_PTR pCmdData );
	OMX_ERRORTYPE GetParameter( OMX_INDEXTYPE nParamIndex, OMX_PTR pComponentParameterStructure );
	OMX_ERRORTYPE SetParameter( OMX_INDEXTYPE nIndex,OMX_PTR pComponentParameterStructure );
	OMX_ERRORTYPE GetConfig( OMX_INDEXTYPE nIndex, OMX_PTR pComponentConfigStructure );
	OMX_ERRORTYPE SetConfig( OMX_INDEXTYPE nIndex, OMX_PTR pComponentConfigStructure );

	std::string mName;
	bool mVerbose;
	State mState;
	OMX_HANDLETYPE mHandle;
	std::vector< uint8_t > mInputPorts;
	std::vector< uint8_t > mOutputPorts;

private:
	static OMX_ERRORTYPE genericeventhandler( OMX_HANDLETYPE handle, Component* component, OMX_EVENTTYPE event, OMX_U32 data1, OMX_U32 data2, OMX_PTR eventdata );
	static OMX_ERRORTYPE emptied( OMX_HANDLETYPE handle, Component* component, OMX_BUFFERHEADERTYPE* buf );
	static OMX_ERRORTYPE filled( OMX_HANDLETYPE handle, Component* component, OMX_BUFFERHEADERTYPE* buf );

	int InitComponent();
};

}; // namespace IL
