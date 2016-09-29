#include "Component.h"
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <bcm_host.h>
#include <IL/OMX_Video.h>
#include <IL/OMX_Types.h>
#include <IL/OMX_Component.h>
#include <IL/OMX_Broadcom.h>
#include <iostream>


static void print_def( OMX_PARAM_PORTDEFINITIONTYPE def );

using namespace IL;

bool Component::mCoreReady = false;
std::list< OMX_U8* > Component::mAllocatedBuffers;
std::list< Component* > Component::mComponents;

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
{
	if ( not mCoreReady ) {
		mCoreReady = true;
		atexit( &Component::onexit );
		// TODO : OMX_Init()
	}

	mComponents.emplace_back( this );

	for ( auto n : input_ports ) {
		Port p;
		p.nPort = n;
		p.bEnabled = false;
		p.bTunneled = false;
		mInputPorts.insert( std::make_pair( n, p ) );
	}
	for ( auto n : output_ports ) {
		Port p;
		p.nPort = n;
		p.bEnabled = false;
		p.bTunneled = false;
		mOutputPorts.insert( std::make_pair( n, p ) );
	}

	InitComponent();
}


Component::~Component()
{
}


OMX_ERRORTYPE Component::SetState( const State& st_ )
{
	if ( mHandle == nullptr ) {
		return OMX_ErrorInvalidComponent;
	}

	if ( st_ == StateExecuting ) {
		for ( auto x : mInputPorts ) {
			Port p = x.second;
			if ( p.bTunneled and not p.bEnabled ) {
				OMX_ERRORTYPE err = SendCommand( OMX_CommandPortEnable, p.nPort, nullptr );
				if ( err != OMX_ErrorNone ) {
					printf( "[%s]SendCommand( OMX_CommandPortEnable, %d ) failed\n", mName.c_str(), p.nPort );
					return err;
				}
				omx_block_until_port_changed( p.nPort, OMX_TRUE );
			}
		}
		for ( auto x : mOutputPorts ) {
			Port p = x.second;
			if ( p.bTunneled and not p.bEnabled ) {
				OMX_ERRORTYPE err = SendCommand( OMX_CommandPortEnable, p.nPort, nullptr );
				if ( err != OMX_ErrorNone ) {
					printf( "[%s]SendCommand( OMX_CommandPortEnable, %d ) failed\n", mName.c_str(), p.nPort );
					return err;
				}
				omx_block_until_port_changed( p.nPort, OMX_TRUE );
			}
		}
	}

	OMX_STATETYPE st = OMX_StateInvalid;
	switch ( st_ ) {
		case StateLoaded :
			st = OMX_StateLoaded;
			break;
		case StateIdle :
			st = OMX_StateIdle;
			break;
		case StateExecuting :
			st = OMX_StateExecuting;
			break;
		case StatePause :
			st = OMX_StatePause;
			break;
		case StateWaitForResources :
			st = OMX_StateWaitForResources;
			break;
		case StateInvalid :
		default :
			st = OMX_StateInvalid;
			break;
	}

	OMX_ERRORTYPE err = SendCommand( OMX_CommandStateSet, st, nullptr );
	if ( err != OMX_ErrorNone ) {
		printf( "[%s]SendCommand( OMX_CommandStateSet, %d ) failed\n", mName.c_str(), st );
		return err;
	}

	printf( "[%s]Wating state to be %d\n", mName.c_str(), st );
	OMX_ERRORTYPE r;
	OMX_STATETYPE st_wait = OMX_StateInvalid;
	do {
		if ( ( r = OMX_GetState( mHandle, &st_wait ) ) != OMX_ErrorNone ) {
			printf( "[%s]omx_block_until_state_changed: Failed to get component state : %d", mName.c_str(), r );
			return r;
		}
		if ( st_wait != st ) {
			usleep( 10000 );
		}
	} while ( st_wait != st );

	return OMX_ErrorNone;
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

	for ( auto port : mInputPorts ) {
		OMX_CONFIG_PORTBOOLEANTYPE zerocopy;
		OMX_INIT_STRUCTURE( zerocopy );
		zerocopy.nPortIndex = port.first;
		zerocopy.bEnabled = OMX_TRUE;
		SetParameter( OMX_IndexParamBrcmZeroCopy, &zerocopy );

		SendCommand( OMX_CommandPortDisable, port.first, nullptr );
	}
	for ( auto port : mOutputPorts ) {
		OMX_CONFIG_PORTBOOLEANTYPE zerocopy;
		OMX_INIT_STRUCTURE( zerocopy );
		zerocopy.nPortIndex = port.first;
		zerocopy.bEnabled = OMX_TRUE;
		SetParameter( OMX_IndexParamBrcmZeroCopy, &zerocopy );

		SendCommand( OMX_CommandPortDisable, port.first, nullptr );
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
	} else {
		mOutputPorts[ port_output ].bTunneled = true;
		mOutputPorts[ port_output ].pTunnel = next;
		mOutputPorts[ port_output ].nTunnelPort = port_input;
		next->mInputPorts[ port_input ].bTunneled = true;
		next->mInputPorts[ port_input ].pTunnel = this;
		next->mInputPorts[ port_input ].nTunnelPort = port_output;
		if ( mVerbose ) {
			fprintf( stderr, "[%s] SetupTunnel from %d to %s:%d completed\n", mName.c_str(), port_output, next->mName.c_str(), port_input );
			OMX_PARAM_PORTDEFINITIONTYPE def;
			OMX_INIT_STRUCTURE( def );
			def.nPortIndex = port_output;
			GetParameter( OMX_IndexParamPortDefinition, &def );
			print_def( def );
			def.nPortIndex = port_input;
			next->GetParameter( OMX_IndexParamPortDefinition, &def );
			print_def( def );
		}
	}
	return err;
}


OMX_ERRORTYPE Component::AllocateBuffers( OMX_BUFFERHEADERTYPE** buffer, int port, bool enable )
{
	if ( mVerbose ) {
		printf( "%p[%s]->AllocateBuffers( %p, %d, %d )\n", mHandle, mName.c_str(), buffer, port, enable );
	}
	OMX_ERRORTYPE ret = OMX_ErrorNone;
	uint32_t i;
	OMX_BUFFERHEADERTYPE* list = NULL;
	OMX_BUFFERHEADERTYPE** end = &list;
	OMX_PARAM_PORTDEFINITIONTYPE portdef;

	OMX_INIT_STRUCTURE( portdef );
	portdef.nPortIndex = port;
	if ( ( ret = GetParameter( OMX_IndexParamPortDefinition, &portdef ) ) != OMX_ErrorNone ) {
		return ret;
	}

	if ( enable ) {
		if ( ( ret = SendCommand( OMX_CommandPortEnable, port, nullptr ) ) != OMX_ErrorNone ) {
			return ret;
		}
		omx_block_until_port_changed( port, OMX_TRUE );
	}

	if ( mVerbose ) {
		printf( "   portdef.nBufferCountActual : %d\n   portdef.nBufferSize : %d\n", portdef.nBufferCountActual, portdef.nBufferSize );
	}

	for ( i = 0; i < portdef.nBufferCountActual; i++ ) {
		OMX_U8* buf;

		buf = (OMX_U8*)vcos_malloc_aligned( portdef.nBufferSize, portdef.nBufferAlignment, "buffer" );
		if ( buf ) {
			mAllocatedBuffers.emplace_back( buf );
		} else {
			return OMX_ErrorInsufficientResources;
		}
		if ( mVerbose ) {
			printf( "Allocated a buffer of %d bytes\n", portdef.nBufferSize );
		}
		ret = OMX_UseBuffer( mHandle, end, port, nullptr, portdef.nBufferSize, buf );
		if ( ret != OMX_ErrorNone ) {
			return ret;
		}
		end = ( OMX_BUFFERHEADERTYPE** )&( (*end)->pAppPrivate );
	}

	*buffer = list;
	return ret;
}


void Component::onexit()
{
	printf( "Component::onexit()\n" );

	for( auto buf : mAllocatedBuffers ) {
		vcos_free( buf );
		printf( "Freed buffer %p\n", buf );
	}

	for( auto comp : mComponents ) {
		OMX_ERRORTYPE err = comp->SendCommand( OMX_CommandStateSet, OMX_StateIdle, nullptr );
		if ( err != OMX_ErrorNone ) {
			printf( "[%s]SendCommand( OMX_CommandStateSet, %d ) failed\n", comp->mName.c_str(), OMX_StateIdle );
			return;
		}
		usleep( 1000 * 100 );
		printf( "FreeHandle( %p ) : %08X\n", comp->mHandle, OMX_FreeHandle( comp->mHandle ) );
	}
}


void Component::omx_block_until_port_changed( OMX_U32 nPortIndex, OMX_BOOL bEnabled )
{
	printf( "Wating port %d to be %d\n", nPortIndex, bEnabled );
	OMX_ERRORTYPE r;
	OMX_PARAM_PORTDEFINITIONTYPE portdef;
	OMX_INIT_STRUCTURE(portdef);
	portdef.nPortIndex = nPortIndex;
	OMX_U32 i = 0;
	while ( i++ == 0 || portdef.bEnabled != bEnabled ) {
		if ( ( r = OMX_GetParameter( mHandle, OMX_IndexParamPortDefinition, &portdef ) ) != OMX_ErrorNone ) {
			printf( "[%s]omx_block_until_port_changed: Failed to get port definition : %d", mName.c_str(), r );
		}
		if ( portdef.bEnabled != bEnabled ) {
			usleep(1000);
		}
	}
}


void Component::omx_block_until_state_changed( OMX_STATETYPE state )
{
	printf( "Wating state to be %d\n", state );
	OMX_ERRORTYPE r;
	OMX_STATETYPE st = (OMX_STATETYPE)-1;
	OMX_U32 i = 0;
	while ( i++ == 0 || st != state ) {
		if ( ( r = OMX_GetState( mHandle, &st ) ) != OMX_ErrorNone ) {
			printf( "[%s]omx_block_until_state_changed: Failed to get handle state : %d", mName.c_str(), r );
		}
		if ( st != state ) {
			usleep(10000);
		}
	}
}



OMX_ERRORTYPE Component::EventHandler( OMX_EVENTTYPE event, OMX_U32 data1, OMX_U32 data2, OMX_PTR eventdata )
{
	if ( mVerbose and event != OMX_EventCmdComplete ) {
		if ( event == OMX_EventError ) {
			fprintf( stderr, "[%s]OMX Error %X\n", mName.c_str(), data1 );
		} else {
			fprintf( stderr, "Event on %p (%s) type %X [ %X, %d, %p ]\n", mHandle, mName.c_str(), event, data1, data2, eventdata );
		}
	}
	return OMX_ErrorNone;
}


OMX_ERRORTYPE Component::EmptyBufferDone( OMX_BUFFERHEADERTYPE* buf )
{
	fprintf( stderr, "EmptyBufferDone on %p (%s)\n", mHandle, mName.c_str() );
	return OMX_ErrorNone;
}


OMX_ERRORTYPE Component::FillBufferDone( OMX_BUFFERHEADERTYPE* buf )
{
	fprintf( stderr, "FillBufferDone on %p (%s)\n", mHandle, mName.c_str() );
	return OMX_ErrorNone;
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


static void print_def( OMX_PARAM_PORTDEFINITIONTYPE def )
{
	printf("Port %u: %s %u/%u %u %u %s,%s,%s %ux%u %ux%u @%ufps %u\n",
		def.nPortIndex,
		def.eDir == OMX_DirInput ? "in" : "out",
		def.nBufferCountActual,
		def.nBufferCountMin,
		def.nBufferSize,
		def.nBufferAlignment,
		def.bEnabled ? "enabled" : "disabled",
		def.bPopulated ? "populated" : "not pop.",
		def.bBuffersContiguous ? "contig." : "not cont.",
		def.format.video.nFrameWidth,
		def.format.video.nFrameHeight,
		def.format.video.nStride,
		def.format.video.nSliceHeight,
		def.format.video.xFramerate >> 16, def.format.video.eColorFormat);
}
