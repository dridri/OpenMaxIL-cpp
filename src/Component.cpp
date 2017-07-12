#include "Component.h"
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <bcm_host.h>
#include <IL/OMX_Video.h>
#include <IL/OMX_Types.h>
#include <IL/OMX_Component.h>
#include <IL/OMX_Broadcom.h>
#include <iostream>
#include "Camera.h"
#include "VideoEncode.h"
#include "VideoDecode.h"


static void print_def( OMX_PARAM_PORTDEFINITIONTYPE def );

using namespace IL;

bool Component::mCoreReady = false;
std::list< OMX_U8* > Component::mAllAllocatedBuffers;
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


Component::Component( const std::string& name, const std::vector< PortInit >& input_ports, const std::vector< PortInit >& output_ports, bool verbose )
	: mName( name )
	, mVerbose( verbose )
	, mState( StateInvalid )
	, mHandle( nullptr )
{
	if ( not mCoreReady ) {
		mCoreReady = true;
		atexit( &Component::onexit );
		OMX_Init();
	}

	mComponents.emplace_back( this );

	for ( PortInit n : input_ports ) {
		Port p;
		p.pParent = this;
		p.nPort = n.id;
		p.type = n.type;
		p.bEnabled = false;
		p.bTunneled = false;
		p.buffer = nullptr;
		p.bufferRunning = false;
		p.bufferNeedsData = false;
		p.bufferDataAvailable = false;
		if ( mVerbose ) {
			printf( "Adding input port { %d, %d }\n", p.nPort, p.type );
		}
		mInputPorts.insert( std::make_pair( n.id, p ) );
	}
	for ( PortInit n : output_ports ) {
		Port p;
		p.pParent = this;
		p.nPort = n.id;
		p.type = n.type;
		p.bEnabled = false;
		p.bTunneled = false;
		p.buffer = nullptr;
		p.bufferRunning = false;
		p.bufferNeedsData = false;
		p.bufferDataAvailable = false;
		if ( mVerbose ) {
			printf( "Adding output port { %d, %d }\n", p.nPort, p.type );
		}
		mOutputPorts.insert( std::make_pair( n.id, p ) );
	}

	InitComponent();
}


Component::~Component()
{
	if ( state() != Component::StateIdle ) {
		SetState( Component::StateIdle, true );
	}
	for ( OMX_U8* buf : mAllocatedBuffers ) {
		for ( OMX_U8* buf2 : mAllAllocatedBuffers ) {
			if ( buf2 == buf ) {
				mAllAllocatedBuffers.remove( buf2 );
				break;
			}
		}
		printf( "Freeing buffer %p\n", buf );
		vcos_free( buf );
	}
	OMX_FreeHandle( mHandle );
}


OMX_ERRORTYPE Component::SetState( const State& st_, bool wait )
{
	if ( mHandle == nullptr ) {
		return OMX_ErrorInvalidComponent;
	}

	if ( st_ == StateExecuting ) {
		OMX_PARAM_PORTDEFINITIONTYPE portdef;
		OMX_INIT_STRUCTURE(portdef);
		for ( auto x : mInputPorts ) {
			Port p = x.second;
			if ( p.nPort != 0 ) {
				portdef.nPortIndex = p.nPort;
				GetParameter( OMX_IndexParamPortDefinition, &portdef );
				p.bEnabled = (bool)portdef.bEnabled;
				if ( p.bTunneled and not p.bEnabled ) {
					OMX_ERRORTYPE err = SendCommand( OMX_CommandPortEnable, p.nPort, nullptr );
					if ( err != OMX_ErrorNone ) {
						printf( "[%s]SendCommand( OMX_CommandPortEnable, %d ) failed\n", mName.c_str(), p.nPort );
						return err;
					}
					if ( wait ) {
						omx_block_until_port_changed( p.nPort, OMX_TRUE );
					}
					p.bEnabled = true;
				}
				if ( p.bEnabled ) {
					GetParameter( OMX_IndexParamPortDefinition, &portdef );
					print_def( portdef );
				}
			}
		}
		for ( auto x : mOutputPorts ) {
			Port p = x.second;
			if ( p.nPort != 0 ) {
				portdef.nPortIndex = p.nPort;
				GetParameter( OMX_IndexParamPortDefinition, &portdef );
				p.bEnabled = (bool)portdef.bEnabled;
				if ( p.bTunneled and not p.bEnabled ) {
					OMX_ERRORTYPE err = SendCommand( OMX_CommandPortEnable, p.nPort, nullptr );
					if ( err != OMX_ErrorNone ) {
						printf( "[%s]SendCommand( OMX_CommandPortEnable, %d ) failed\n", mName.c_str(), p.nPort );
						return err;
					}
					if ( wait ) {
						omx_block_until_port_changed( p.nPort, OMX_TRUE );
					}
					p.bEnabled = true;
				}
				if ( p.bEnabled ) {
					GetParameter( OMX_IndexParamPortDefinition, &portdef );
					print_def( portdef );
				}
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

	if ( wait ) {
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
	}

	if ( st_ == StateIdle ) {
		OMX_PARAM_PORTDEFINITIONTYPE portdef;
		OMX_INIT_STRUCTURE(portdef);
		for ( auto x : mInputPorts ) {
			Port p = x.second;
			if ( p.nPort != 0 ) {
				portdef.nPortIndex = p.nPort;
				GetParameter( OMX_IndexParamPortDefinition, &portdef );
				p.bEnabled = (bool)portdef.bEnabled;
				if ( p.bTunneled and p.bEnabled ) {
					SendCommand( OMX_CommandPortDisable, p.nPort, nullptr );
					if ( wait ) {
						omx_block_until_port_changed( p.nPort, OMX_FALSE );
					}
					p.bEnabled = false;
				}
			}
		}
		for ( auto x : mOutputPorts ) {
			Port p = x.second;
			if ( p.nPort != 0 ) {
				portdef.nPortIndex = p.nPort;
				GetParameter( OMX_IndexParamPortDefinition, &portdef );
				p.bEnabled = (bool)portdef.bEnabled;
				if ( p.bTunneled and p.bEnabled ) {
					SendCommand( OMX_CommandPortDisable, p.nPort, nullptr );
					if ( wait ) {
						omx_block_until_port_changed( p.nPort, OMX_FALSE );
					}
					p.bEnabled = false;
				}
			}
		}
	}

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
		return -1;
	} else if ( mVerbose ) {
		fprintf( stderr, "OMX_GetHandle(%s) completed\n", mName.c_str() );
	}

	for ( auto port : mInputPorts ) {
		OMX_CONFIG_PORTBOOLEANTYPE zerocopy;
		OMX_INIT_STRUCTURE( zerocopy );
		zerocopy.nPortIndex = port.first;
		zerocopy.bEnabled = OMX_TRUE;
// 		SetParameter( OMX_IndexParamBrcmZeroCopy, &zerocopy );

		SendCommand( OMX_CommandPortDisable, port.first, nullptr );
	}
	for ( auto port : mOutputPorts ) {
		OMX_CONFIG_PORTBOOLEANTYPE zerocopy;
		OMX_INIT_STRUCTURE( zerocopy );
		zerocopy.nPortIndex = port.first;
		zerocopy.bEnabled = OMX_TRUE;
// 		SetParameter( OMX_IndexParamBrcmZeroCopy, &zerocopy );

		SendCommand( OMX_CommandPortDisable, port.first, nullptr );
	}

	return 0;
}


OMX_ERRORTYPE Component::AllocateInputBuffer( uint16_t port )
{
	if ( port == 0 ) {
		for ( uint32_t i = 40; i <= 350; i++ ) {
			if ( mInputPorts.find(i) != mInputPorts.end() and mInputPorts[i].type == Video ) {
				port = i;
				break;
			}
		}
	}
	OMX_ERRORTYPE ret = AllocateBuffers( &mInputPorts[port].buffer, port, true );
	mInputPorts[port].buffer_copy = (OMX_BUFFERHEADERTYPE*)malloc( sizeof( OMX_BUFFERHEADERTYPE ) );
	memcpy( mInputPorts[port].buffer_copy, mInputPorts[port].buffer, sizeof( OMX_BUFFERHEADERTYPE ) );
	mInputPorts[port].bEnabled = true;
	return ret;
}



OMX_ERRORTYPE Component::AllocateOutputBuffer( uint16_t port )
{
	if ( port == 0 ) {
		for ( uint32_t i = 40; i <= 350; i++ ) {
			if ( mOutputPorts.find(i) != mOutputPorts.end() and mOutputPorts[i].type == Video ) {
				port = i;
				break;
			}
		}
	}
	OMX_ERRORTYPE ret = AllocateBuffers( &mOutputPorts[port].buffer, port, true );
	mOutputPorts[port].buffer_copy = (OMX_BUFFERHEADERTYPE*)malloc( sizeof( OMX_BUFFERHEADERTYPE ) );
	printf( "=> memcpy( %p, %p, %d )\n", mOutputPorts[port].buffer_copy, mOutputPorts[port].buffer, sizeof( OMX_BUFFERHEADERTYPE ) );
	memcpy( mOutputPorts[port].buffer_copy, mOutputPorts[port].buffer, sizeof( OMX_BUFFERHEADERTYPE ) );
	mOutputPorts[port].bEnabled = true;

	return ret;
}


const bool Component::needData( uint16_t port )
{
	return mInputPorts[port].bufferNeedsData;
}


void Component::fillInput( uint16_t port, uint8_t* pBuf, uint32_t len, bool corrupted, bool eof )
{
	OMX_BUFFERHEADERTYPE* buffer = mInputPorts[port].buffer;

	if ( buffer ) {
		memcpy( buffer->pBuffer, pBuf, len );

		// Send buffer to GPU
		buffer->nTimeStamp = { 0, 0 };
		buffer->nFilledLen = len;
		buffer->nFlags = ( eof ? OMX_BUFFERFLAG_ENDOFFRAME : 0 ) | ( corrupted ? OMX_BUFFERFLAG_DATACORRUPT : 0 ) | OMX_BUFFERFLAG_TIME_UNKNOWN;
		OMX_ERRORTYPE err = ((OMX_COMPONENTTYPE*)mHandle)->EmptyThisBuffer( mHandle, buffer );
		if ( err != OMX_ErrorNone ) {
			printf( "Component::fillInput : EmptyThisBuffer error : 0x%08X (bufSize : %d ; corrupted : %d)\n", (uint32_t)err, len, corrupted );
			fflush( stdout );
		}
	}
}


const bool Component::dataAvailable( uint16_t port )
{
	return mOutputPorts[port].bufferDataAvailable;
}


int32_t Component::getOutputData( uint16_t port, uint8_t* pBuf, bool wait )
{
	uint32_t datalen = 0;
	OMX_BUFFERHEADERTYPE* buffer = mOutputPorts[port].buffer;

	OMX_ERRORTYPE err = ((OMX_COMPONENTTYPE*)mHandle)->FillThisBuffer( mHandle, buffer );
	if ( err != OMX_ErrorNone ) {
		printf( "Component::getOutputData : FillThisBuffer error : 0x%08X\n", (uint32_t)err );
		return err;
	}

	if ( buffer ) {
		std::unique_lock<std::mutex> locker( mDataAvailableMutex );

		if ( mVerbose ) {
			printf( "mOutputPorts[%d].bufferDataAvailable = %d\n", port, mOutputPorts[port].bufferDataAvailable );
		}
		if ( not mOutputPorts[port].bufferDataAvailable and wait ) {
			if ( mVerbose ) {
				printf( "Component::getOutputData() : Waiting...\n" );
			}
			mDataAvailableCond.wait( locker );
		} else if ( not wait ) {
			locker.unlock();
			return OMX_ErrorOverflow;
		}

		if ( buffer->pBuffer != mOutputPorts[port].buffer_copy->pBuffer ) {
			printf( "LEAK : %p != %p\n", buffer->pBuffer, mOutputPorts[port].buffer_copy->pBuffer );
			memcpy( buffer, mOutputPorts[port].buffer_copy, sizeof(OMX_BUFFERHEADERTYPE) );
			fflush( stdout );
		}

		if ( buffer->nFilledLen > 0 and pBuf ) {
			memcpy( pBuf, buffer->pBuffer, buffer->nFilledLen );
		}
		datalen = buffer->nFilledLen;
		locker.unlock();
	}

	return datalen;
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

	char name[256] = "";
	sprintf( name, "0x%08X", nParamIndex );
	if ( nParamIndex == OMX_IndexParamPortDefinition ) {
		sprintf( name, "OMX_IndexParamPortDefinition" );
	}

	OMX_ERRORTYPE err = ((OMX_COMPONENTTYPE*)mHandle)->GetParameter( mHandle, nParamIndex, pComponentParameterStructure );

	if ( err != OMX_ErrorNone ) {
		fprintf( stderr, "[%s] GetParameter %s failed : %X\n", mName.c_str(), name, err );
	} else if ( mVerbose and nParamIndex != OMX_IndexParamPortDefinition ) {
		fprintf( stderr, "[%s] GetParameter %s completed\n", mName.c_str(), name );
	}

	return err;
}


OMX_ERRORTYPE Component::SetParameter( OMX_INDEXTYPE nIndex, OMX_PTR pComponentParameterStructure )
{
	if ( mHandle == nullptr ) {
		return OMX_ErrorInvalidComponent;
	}

	char name[256] = "";
	sprintf( name, "0x%08X", nIndex );
	if ( nIndex == OMX_IndexParamPortDefinition ) {
		sprintf( name, "OMX_IndexParamPortDefinition" );
	}

	OMX_ERRORTYPE err = ((OMX_COMPONENTTYPE*)mHandle)->SetParameter( mHandle, nIndex, pComponentParameterStructure );

	if ( err != OMX_ErrorNone ) {
		fprintf( stderr, "[%s] SetParameter %s failed : %X\n", mName.c_str(), name, err );
	} else if ( mVerbose and nIndex != OMX_IndexParamPortDefinition ) {
		fprintf( stderr, "[%s] SetParameter %s completed\n", mName.c_str(), name );
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


OMX_ERRORTYPE Component::SetupTunnel( uint16_t port_output, Component* next, uint16_t port_input, Port* port_copy )
{
	if ( next->CustomTunnelInput( this, port_output, port_input ) ) {
		return OMX_ErrorNone;
	}

	if ( mVerbose ) {
		printf( "%p[%s] SetupTunnel( %d, %p, %d )\n", mHandle, mName.c_str(), port_output, next, port_input );
	}

	if ( mHandle == nullptr or next == nullptr or next->mHandle == nullptr ) {
		return OMX_ErrorInvalidComponent;
	}

	if ( port_input == 0 ) {
		for ( auto p : next->mInputPorts ) {
			Port port = p.second;
			printf( "===> types : %d[%d] | %d[%d]\n", (uint32_t)port.type, port.nPort, (uint32_t)mOutputPorts[port_output].type, mOutputPorts[port_output].nPort );
			if ( ( (uint32_t)port.type & (uint32_t)mOutputPorts[port_output].type ) != 0 ) {
				port_input = port.nPort;
				break;
			}
		}
		if ( mVerbose ) {
			printf( "%p[%s] SetupTunnel detected input port : %d\n", mHandle, mName.c_str(), port_input );
		}
	}

	OMX_PARAM_PORTDEFINITIONTYPE def;
	OMX_INIT_STRUCTURE( def );
	def.nPortIndex = port_output;
	GetParameter( OMX_IndexParamPortDefinition, &def );
	print_def( def );
	def.nPortIndex = port_input;
	next->GetParameter( OMX_IndexParamPortDefinition, &def );
	print_def( def );

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
		if ( false and port_copy ) {
			if ( mVerbose ) {
				fprintf( stderr, "Copy from %p:%d to %p:%d\n", port_copy->pParent, port_copy->nPort, next, port_input );
				fprintf( stderr, "Copy from %s:%d to %s:%d\n", port_copy->pParent->name().c_str(), port_copy->nPort, next->name().c_str(), port_input );
			}
			OMX_PARAM_PORTDEFINITIONTYPE def;
			OMX_INIT_STRUCTURE( def );
			def.nPortIndex = port_copy->nPort;
			port_copy->pParent->GetParameter( OMX_IndexParamPortDefinition, &def );
			print_def( def );
			def.nPortIndex = port_input;
			next->SetParameter( OMX_IndexParamPortDefinition, &def );
		}
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


OMX_ERRORTYPE Component::DestroyTunnel( uint16_t port_output )
{
	OMX_ERRORTYPE err = OMX_SetupTunnel( mHandle, port_output, 0, 0 );
	if ( err != OMX_ErrorNone ) {
		return err;
	}
	err = OMX_SetupTunnel( 0, 0, mOutputPorts[port_output].pTunnel, mOutputPorts[port_output].nTunnelPort );
	if ( err != OMX_ErrorNone ) {
		return err;
	}

	mOutputPorts[port_output].pTunnel->inputPorts()[mOutputPorts[port_output].nTunnelPort].bTunneled = false;
	mOutputPorts[port_output].pTunnel->inputPorts()[mOutputPorts[port_output].nTunnelPort].nTunnelPort = 0;
	mOutputPorts[port_output].pTunnel->inputPorts()[mOutputPorts[port_output].nTunnelPort].pTunnel = nullptr;
	mOutputPorts[port_output].bTunneled = false;
	mOutputPorts[port_output].nTunnelPort = 0;
	mOutputPorts[port_output].pTunnel = nullptr;

	return OMX_ErrorNone;
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

	uint32_t size = portdef.nBufferSize;
	if ( dynamic_cast<VideoEncode*>(this) != nullptr or dynamic_cast<VideoDecode*>(this) != nullptr ) {
		size *= 4; // Buffer can be too small in low-light=>high luminosity transition
	}

	for ( i = 0; i < portdef.nBufferCountActual; i++ ) {
		OMX_U8* buf;

		buf = (OMX_U8*)vcos_malloc_aligned( size, portdef.nBufferAlignment, "buffer" );
		if ( buf ) {
			mAllocatedBuffers.emplace_back( buf );
			mAllAllocatedBuffers.emplace_back( buf );
		} else {
			return OMX_ErrorInsufficientResources;
		}
		if ( mVerbose ) {
			printf( "Allocated a buffer of %d bytes\n", size );
		}
		ret = OMX_UseBuffer( mHandle, end, port, nullptr, size, buf );
		if ( ret != OMX_ErrorNone ) {
			printf( "OMX_UseBuffer error 0x%08X ! (state : %d)\n", ret, (int)state() );
			*buffer = list;
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

	// First, stop cameras
	for( auto comp : mComponents ) {
		if ( dynamic_cast< Camera* >( comp ) != nullptr ) {
			printf( "Stopping Camera %s\n", comp->mName.c_str() );
			dynamic_cast< Camera* >( comp )->SetCapturing( false );
			printf( "Camera stopped\n" );
		}
	}
	usleep( 1000 * 100 );

	// Secondly, reversly stop components
	std::list< Component* >::iterator iter = mComponents.begin();
	std::list< Component* >::reverse_iterator riter = mComponents.rbegin();
// 	for ( iter = mComponents.rbegin(); iter != mComponents.rend(); iter++ ) {
	for ( iter = mComponents.begin(); iter != mComponents.end(); iter++ ) {
		printf( "Stopping component %s\n", (*iter)->mName.c_str() );
		(*iter)->SetState( Component::StateIdle, false );
	}

	// Thirdly, 

	// Fourth, reversly remove components
	for ( riter = mComponents.rbegin(); riter != mComponents.rend(); riter++ ) {
		printf( "Deleting component %s\n", (*riter)->mName.c_str() );
		delete (*riter);
		printf( "Component delete ok\n" );
	}
	mComponents.clear();

	// Finally, free remaining buffers
	for ( OMX_U8* buf : mAllAllocatedBuffers ) {
		vcos_free( buf );
		printf( "Freed static buffer %p\n", buf );
	}
	mAllAllocatedBuffers.clear();
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
			if ( (int)data1 != OMX_ErrorSameState ) {
				fprintf( stderr, "[%s]OMX Error %X\n", mName.c_str(), data1 );
			}
		} else {
			fprintf( stderr, "Event on %p (%s) type %X [ %d, %d, %p ]\n", mHandle, mName.c_str(), event, data1, data2, eventdata );
		}
	}
	return OMX_ErrorNone;
}


OMX_ERRORTYPE Component::EmptyBufferDone( OMX_BUFFERHEADERTYPE* buf )
{
	if ( mVerbose ) {
		fprintf( stdout, "Component::EmptyBufferDone on %p (%s)%s\n", mHandle, mName.c_str(), ( buf->nFlags & OMX_BUFFERFLAG_ENDOFFRAME ) ? " (eof)" : "" );
	}
	for ( auto p : mInputPorts ) {
		if ( p.second.buffer == buf ) {
			p.second.bufferNeedsData = true;
			break;
		}
	}
	return OMX_ErrorNone;
}


OMX_ERRORTYPE Component::FillBufferDone( OMX_BUFFERHEADERTYPE* buf )
{
	if ( mVerbose ) {
		fprintf( stdout, "Component::FillBufferDone on %p[%s] (%p)\n", mHandle, mName.c_str(), buf );
	}
	for ( auto p : mOutputPorts ) {
		if ( p.second.buffer == buf ) {
			std::unique_lock<std::mutex> locker( mDataAvailableMutex );
			p.second.bufferDataAvailable = true;
			mDataAvailableCond.notify_all();
			break;
		}
	}
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


OMX_ERRORTYPE Component::EnablePort( uint16_t port )
{
	OMX_ERRORTYPE ret = SendCommand( OMX_CommandPortEnable, port, nullptr );
	omx_block_until_port_changed( port, OMX_TRUE );
	mInputPorts[port].bEnabled = true;
	mOutputPorts[port].bEnabled = true;
	return ret;
}


OMX_ERRORTYPE Component::CopyPort( Port* from, Port* to )
{
	printf( "CopyPort( %s->%d, %s->%d\n", from->pParent->name().c_str(), from->nPort, to->pParent->name().c_str(), to->nPort );
	OMX_PARAM_PORTDEFINITIONTYPE def;
	OMX_INIT_STRUCTURE( def );
	def.nPortIndex = from->nPort;
	from->pParent->GetParameter( OMX_IndexParamPortDefinition, &def );
	def.nPortIndex = to->nPort;
	OMX_ERRORTYPE ret = to->pParent->SetParameter( OMX_IndexParamPortDefinition, &def );
	to->pParent->GetParameter( OMX_IndexParamPortDefinition, &def );
	print_def( def );
	return ret;
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


uint64_t Component::ticks64()
{
	struct timespec now;
	clock_gettime( CLOCK_MONOTONIC, &now );
	return (uint64_t)now.tv_sec * 1000000ULL + (uint64_t)now.tv_nsec / 1000ULL;
}
