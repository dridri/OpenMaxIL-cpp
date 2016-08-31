#include "Component.h"
#include <string.h>
#include <IL/OMX_Video.h>
#include <IL/OMX_Types.h>
#include <IL/OMX_Component.h>
#include <IL/OMX_Broadcom.h>

using namespace IL;

OMX_ERRORTYPE Component::genericeventhandler( OMX_HANDLETYPE handle, Component* component, OMX_EVENTTYPE event, OMX_U32 data1, OMX_U32 data2, OMX_PTR eventdata )
{
	return component->EventHandler( event, data1, data2, eventdata );
}


OMX_ERRORTYPE Component::emptied( OMX_HANDLETYPE handle, Component* component, OMX_BUFFERHEADERTYPE* buf )
{
	return component->EmptyBufferDone( buf );
}


OMX_ERRORTYPE Component::filled( OMX_HANDLETYPE handle, Component* component, OMX_BUFFERHEADERTYPE* buf )
{
	return component->FillBufferDone( buf );
}


Component::Component( const std::string& name, const std::vector< uint8_t >& input_ports, const std::vector< uint8_t >& output_ports, bool verbose )
	: mName( name )
	, mVerbose( verbose )
	, mState( StateInvalid )
	, mHandle( nullptr )
	, mInputPorts( input_ports )
	, mOutputPorts( output_ports )
{
	InitComponent();
}


Component::~Component()
{
}


int Component::InitComponent()
{
	OMX_CALLBACKTYPE cb = {
		(OMX_ERRORTYPE (*)(void*, void*, OMX_EVENTTYPE, unsigned int, unsigned int, void*))&Component::genericeventhandler,
		(OMX_ERRORTYPE (*)(void*, void*, OMX_BUFFERHEADERTYPE*))&Component::emptied,
		(OMX_ERRORTYPE (*)(void*, void*, OMX_BUFFERHEADERTYPE*))&Component::filled
	};
	OMX_ERRORTYPE err = OMX_GetHandle( &mHandle, (char*)mName.c_str(), this, &cb );
	if ( err != OMX_ErrorNone ) {
		fprintf( stderr, "OMX_GetHandle(%s) failed : %X\n", mName.c_str(), err );
	} else if ( mVerbose ) {
		fprintf( stderr, "OMX_GetHandle(%s) completed\n", mName.c_str() );
	}

	for ( uint8_t port : mInputPorts ) {
		SendCommand( OMX_CommandPortDisable, port, nullptr );
	}
	for ( uint8_t port : mOutputPorts ) {
		SendCommand( OMX_CommandPortDisable, port, nullptr );
	}

	return 0;
}


OMX_ERRORTYPE Component::SendCommand( OMX_COMMANDTYPE Cmd, OMX_U32 nParam1, OMX_PTR pCmdData )
{
	if ( mHandle == nullptr ) {
		return OMX_ErrorInvalidComponent;
	}

	OMX_ERRORTYPE err = ((OMX_COMPONENTTYPE*)mHandle)->SendCommand( mHandle, Cmd, nParam1, pCmdData );

	if ( err != OMX_ErrorNone ) {
		fprintf( stderr, "[%s] SendCommand %X failed : %X\n", mName.c_str(), Cmd, err );
	} else if ( mVerbose ) {
		fprintf( stderr, "[%s] SendCommand %X completed\n", mName.c_str(), Cmd );
	}

	return err;
}


OMX_ERRORTYPE Component::GetParameter( OMX_INDEXTYPE nParamIndex, OMX_PTR pComponentParameterStructure )
{
	if ( mHandle == nullptr ) {
		return OMX_ErrorInvalidComponent;
	}

	OMX_ERRORTYPE err = ((OMX_COMPONENTTYPE*)mHandle)->GetParameter( mHandle, nParamIndex, pComponentParameterStructure );

	if ( err != OMX_ErrorNone ) {
		fprintf( stderr, "[%s] GetParameter %X failed : %X\n", mName.c_str(), nParamIndex, err );
	} else if ( mVerbose ) {
		fprintf( stderr, "[%s] GetParameter %X completed\n", mName.c_str(), nParamIndex );
	}

	return err;
}


OMX_ERRORTYPE Component::SetParameter( OMX_INDEXTYPE nIndex, OMX_PTR pComponentParameterStructure )
{
	if ( mHandle == nullptr ) {
		return OMX_ErrorInvalidComponent;
	}

	OMX_ERRORTYPE err = ((OMX_COMPONENTTYPE*)mHandle)->SetParameter( mHandle, nIndex, pComponentParameterStructure );

	if ( err != OMX_ErrorNone ) {
		fprintf( stderr, "[%s] SetParameter %X failed : %X\n", mName.c_str(), nIndex, err );
	} else if ( mVerbose ) {
		fprintf( stderr, "[%s] SetParameter %X completed\n", mName.c_str(), nIndex );
	}

	return err;
}


OMX_ERRORTYPE Component::GetConfig( OMX_INDEXTYPE nIndex, OMX_PTR pComponentConfigStructure )
{
	if ( mHandle == nullptr ) {
		return OMX_ErrorInvalidComponent;
	}

	OMX_ERRORTYPE err = ((OMX_COMPONENTTYPE*)mHandle)->GetConfig( mHandle, nIndex, pComponentConfigStructure );

	if ( err != OMX_ErrorNone ) {
		fprintf( stderr, "[%s] GetConfig %X failed : %X\n", mName.c_str(), nIndex, err );
	} else if ( mVerbose ) {
		fprintf( stderr, "[%s] GetConfig %X completed\n", mName.c_str(), nIndex );
	}

	return err;
}


OMX_ERRORTYPE Component::SetConfig( OMX_INDEXTYPE nIndex, OMX_PTR pComponentConfigStructure )
{
	if ( mHandle == nullptr ) {
		return OMX_ErrorInvalidComponent;
	}

	OMX_ERRORTYPE err = ((OMX_COMPONENTTYPE*)mHandle)->SetConfig( mHandle, nIndex, pComponentConfigStructure );

	if ( err != OMX_ErrorNone ) {
		fprintf( stderr, "[%s] SetConfig %X failed : %X\n", mName.c_str(), nIndex, err );
	} else if ( mVerbose ) {
		fprintf( stderr, "[%s] SetConfig %X completed\n", mName.c_str(), nIndex );
	}
	return err;
}


OMX_ERRORTYPE Component::SetupTunnel( uint8_t port_output, Component* next, uint8_t port_input )
{
	if ( mHandle == nullptr or next == nullptr or next->mHandle == nullptr ) {
		return OMX_ErrorInvalidComponent;
	}

	OMX_ERRORTYPE err = OMX_SetupTunnel( mHandle, port_output, next->mHandle, port_input );

	if ( err != OMX_ErrorNone ) {
		fprintf( stderr, "[%s] SetupTunnel from %d to %s:%d failed : %X\n", mName.c_str(), port_output, next->mName.c_str(), port_input, err );
	} else if ( mVerbose ) {
		fprintf( stderr, "[%s] SetupTunnel from %d to %s:%d completed\n", mName.c_str(), port_output, next->mName.c_str(), port_input );
	}
	return err;
}


OMX_ERRORTYPE Component::EventHandler( OMX_EVENTTYPE event, OMX_U32 data1, OMX_U32 data2, OMX_PTR eventdata )
{
	if ( mVerbose and event != OMX_EventCmdComplete ) {
		fprintf( stderr, "Event on %p (%s) type %d\n", mHandle, mName.c_str(), event );
	}
	return OMX_ErrorNone;
}


OMX_ERRORTYPE Component::EmptyBufferDone( OMX_BUFFERHEADERTYPE* buf )
{
	if ( mVerbose ) {
		fprintf( stderr, "EmptyBufferDone on %p (%s)\n", mHandle, mName.c_str() );
	}
	return OMX_ErrorNone;
}


OMX_ERRORTYPE Component::FillBufferDone( OMX_BUFFERHEADERTYPE* buf )
{
	if ( mVerbose ) {
		fprintf( stderr, "FillBufferDone on %p (%s)\n", mHandle, mName.c_str() );
	}
	return OMX_ErrorNone;
}


template<typename T> void Component::OMX_INIT_STRUCTURE( T& omx_struct )
{
	memset( &omx_struct, 0, sizeof(omx_struct) );
	omx_struct.nSize = sizeof(omx_struct);
	omx_struct.nVersion.nVersion = OMX_VERSION;
	omx_struct.nVersion.s.nVersionMajor = OMX_VERSION_MAJOR;
	omx_struct.nVersion.s.nVersionMinor = OMX_VERSION_MINOR;
	omx_struct.nVersion.s.nRevision = OMX_VERSION_REVISION;
	omx_struct.nVersion.s.nStep = OMX_VERSION_STEP;
}


const std::string& Component::name() const
{
	return mName;
}


const Component::State Component::state()
{
	if ( mHandle ) {
		OMX_STATETYPE st = OMX_StateInvalid;
		OMX_GetState( mHandle, &st );
		switch ( st ) {
			case OMX_StateLoaded :
				mState = StateLoaded;
				break;
			case OMX_StateIdle :
				mState = StateIdle;
				break;
			case OMX_StateExecuting :
				mState = StateExecuting;
				break;
			case OMX_StatePause :
				mState = StatePause;
				break;
			case OMX_StateWaitForResources :
				mState = StateWaitForResources;
				break;
			case OMX_StateInvalid :
			default :
				mState = StateInvalid;
				break;
		}
	}
	return mState;
}
