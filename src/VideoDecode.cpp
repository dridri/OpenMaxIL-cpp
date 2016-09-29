#include "VideoDecode.h"
#include <unistd.h>
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

	OMX_CONFIG_BOOLEANTYPE interpolate_timestamp;
	OMX_INIT_STRUCTURE( interpolate_timestamp );
	interpolate_timestamp.bEnabled = OMX_FALSE;
	SetParameter( OMX_IndexParamBrcmInterpolateMissingTimestamps, &interpolate_timestamp );

	OMX_PARAM_BRCMVIDEODECODEERRORCONCEALMENTTYPE errconceal;
	OMX_INIT_STRUCTURE( errconceal );
	errconceal.bStartWithValidFrame = OMX_FALSE;
	SetParameter( OMX_IndexParamBrcmVideoDecodeErrorConcealment, &errconceal );


	OMX_PARAM_DATAUNITTYPE unit;
	OMX_INIT_STRUCTURE( unit );
	unit.nPortIndex = 130;
	unit.eUnitType = OMX_DataUnitCodedPicture;
	unit.eEncapsulationType = OMX_DataEncapsulationElementaryStream;
	SetParameter( OMX_IndexParamBrcmDataUnit, &unit );

	if ( coding_type == CodingAVC ) {
		OMX_PARAM_U32TYPE extrabuffers;
		OMX_INIT_STRUCTURE( extrabuffers );
		extrabuffers.nPortIndex = 131;
		extrabuffers.nU32 = 1;
		SetParameter( OMX_IndexParamBrcmExtraBuffers, &extrabuffers );
/*
		OMX_PARAM_IMAGEPOOLSIZETYPE imagepool;
		imagepool.width = 1280;
		imagepool.height = 720;
		imagepool.num_pages = 2;
		SetParameter( OMX_IndexParamImagePoolSize, &imagepool );
*/
	}

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

	if ( mBuffer ) {
		mNeedData = true;
	}

	OMX_ERRORTYPE ret = IL::Component::SetState(st);
	if ( ret != OMX_ErrorNone ) {
		return ret;
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
// 	std::cout << "EmptyBufferDone()\n";

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
/*
	mNeedDataMutex.lock();
	if ( mNeedData ) {
		mNeedData = false;
		mNeedDataMutex.unlock();
	} else {
		mNeedDataMutex.unlock();
		std::unique_lock<std::mutex> locker( mNeedDataMutex );
		std::cout << "fillInput()::cond_wait\n";
		mNeedDataCond.wait( locker );
		mNeedData = false;
	}
*/
	while ( mNeedData == false ) {
		usleep( 1 );
	}

	if ( mBuffer ) {
		SendCommand( OMX_CommandFlush, 130, nullptr );
		mNeedData = false;
		memcpy( mBuffer->pBuffer, pBuf, len );
		mBuffer->nFilledLen = len;
		mBuffer->nFlags = OMX_BUFFERFLAG_ENDOFFRAME | OMX_BUFFERFLAG_DATACORRUPT | ( mFirstData ? OMX_BUFFERFLAG_STARTTIME : OMX_BUFFERFLAG_TIME_UNKNOWN );
		((OMX_COMPONENTTYPE*)mHandle)->EmptyThisBuffer( mHandle, mBuffer );
		mFirstData = false;
	}
}
