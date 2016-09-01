#include "VideoDecode.h"
#include <IL/OMX_Types.h>
#include <IL/OMX_Component.h>
#include <IL/OMX_Broadcom.h>
#include <iostream>

using namespace IL;

VideoDecode::VideoDecode( uint32_t fps, const CodingType& coding_type, bool verbose )
	: Component( "OMX.broadcom.video_decode", { 130 }, { 131 }, verbose )
	, mCodingType( coding_type )
	, mBuffer( nullptr )
	, mFirstData( true )
	, mDecoderValid( false )
	, mVideoRunning( false )
	, mNeedData( false )
{
	OMX_VIDEO_PARAM_PORTFORMATTYPE pfmt;
	OMX_INIT_STRUCTURE( pfmt );
	pfmt.nPortIndex = 130;
// 	pfmt.eColorFormat = OMX_COLOR_FormatYUV420PackedPlanar;
	pfmt.eCompressionFormat = (OMX_VIDEO_CODINGTYPE)coding_type;
	pfmt.xFramerate = fps << 16;
	SetParameter( OMX_IndexParamVideoPortFormat, &pfmt );

	SendCommand( OMX_CommandStateSet, OMX_StateIdle, nullptr );
}


VideoDecode::~VideoDecode()
{
}


OMX_ERRORTYPE VideoDecode::SetState( const Component::State& st )
{
	if ( mOutputPorts[130].bTunneled == false and mBuffer == nullptr ) {
		AllocateBuffers( &mBuffer, 130, true );
		mOutputPorts[130].bEnabled = true;
	}

	OMX_ERRORTYPE ret = IL::Component::SetState(st);
	if ( ret != OMX_ErrorNone ) {
		return ret;
	}

	if ( mBuffer ) {
		mNeedData = true;
	}
	return ret;
}


OMX_ERRORTYPE VideoDecode::EventHandler( OMX_EVENTTYPE event, OMX_U32 data1, OMX_U32 data2, OMX_PTR eventdata )
{
	if ( event == OMX_EventPortSettingsChanged and not mDecoderValid ) {
		mFirstData = true;
		mDecoderValid = true;
	}
	return IL::Component::EventHandler( event, data1, data2, eventdata );
}


OMX_ERRORTYPE VideoDecode::EmptyBufferDone( OMX_BUFFERHEADERTYPE* buf )
{
// 	std::cout << "EmptyBufferDone\n";
	std::unique_lock<std::mutex> locker( mNeedDataMutex );
	mNeedData = true;
	mNeedDataCond.notify_all();
	return OMX_ErrorNone;
}


const bool VideoDecode::needData() const
{
	return mNeedData;
}


void VideoDecode::fillInput( uint8_t* pBuf, uint32_t len )
{
	if ( not mVideoRunning and mDecoderValid ) {
		mVideoRunning = true;

		mOutputPorts[131].pTunnel->SetState( StateIdle );
		mOutputPorts[131].pTunnel->SendCommand( OMX_CommandPortDisable, mOutputPorts[131].nTunnelPort, nullptr );
		SendCommand( OMX_CommandPortDisable, 131, nullptr );

		OMX_PARAM_PORTDEFINITIONTYPE def;
		OMX_INIT_STRUCTURE( def );
		def.nPortIndex = 131;
		GetParameter( OMX_IndexParamPortDefinition, &def );
		def.eDomain = OMX_PortDomainVideo;
		def.nPortIndex = mOutputPorts[131].nTunnelPort;
		mOutputPorts[131].pTunnel->SetParameter( OMX_IndexParamPortDefinition, &def );

		SetupTunnel( 131, mOutputPorts[131].pTunnel, mOutputPorts[131].nTunnelPort );

		mOutputPorts[131].pTunnel->SendCommand( OMX_CommandPortEnable, mOutputPorts[131].nTunnelPort, nullptr );
		mOutputPorts[131].pTunnel->SetState( StateExecuting );
		SendCommand( OMX_CommandPortEnable, 131, nullptr );
	}

	mNeedDataMutex.lock();
	if ( mNeedData ) {
		mNeedData = false;
		mNeedDataMutex.unlock();
	} else {
		mNeedDataMutex.unlock();
		std::unique_lock<std::mutex> locker( mNeedDataMutex );
		mNeedDataCond.wait( locker );
		mNeedData = false;
	}

	if ( mBuffer ) {
		memcpy( mBuffer->pBuffer, pBuf, len );
		mBuffer->nFilledLen = len;
		mBuffer->nFlags = ( mFirstData ? OMX_BUFFERFLAG_STARTTIME : OMX_BUFFERFLAG_TIME_UNKNOWN );
		((OMX_COMPONENTTYPE*)mHandle)->EmptyThisBuffer( mHandle, mBuffer );
		mFirstData = false;
	}
}
