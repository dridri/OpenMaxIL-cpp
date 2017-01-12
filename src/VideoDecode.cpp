#include "VideoDecode.h"
#include <unistd.h>
#include <IL/OMX_Types.h>
#include <IL/OMX_Component.h>
#include <IL/OMX_Broadcom.h>
#include <iostream>

using namespace IL;

static void PrintBuffer( void* _buf, int size );

VideoDecode::VideoDecode( uint32_t fps, const CodingType& coding_type, bool verbose )
	: Component( "OMX.broadcom.video_decode", { PortInit( 130, Video ) }, { PortInit( 131, Video ) }, verbose )
	, mCodingType( coding_type )
	, mBuffer( nullptr )
	, mOutputBuffer( nullptr )
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

	SendCommand( OMX_CommandStateSet, OMX_StateIdle, nullptr );
}


VideoDecode::~VideoDecode()
{
}


void VideoDecode::setRGB565Mode( bool en )
{
	OMX_PARAM_PORTDEFINITIONTYPE def;
	OMX_INIT_STRUCTURE( def );
	def.nPortIndex = 131;
	GetParameter( OMX_IndexParamPortDefinition, &def );
	def.format.video.eColorFormat = en ? OMX_COLOR_Format16bitRGB565 : OMX_COLOR_FormatYUV420PackedPlanar;
	SetParameter( OMX_IndexParamPortDefinition, &def );
}


OMX_ERRORTYPE VideoDecode::SetupTunnel( Component* next, uint8_t port_input )
{
	return Component::SetupTunnel( 131, next, port_input );
}


OMX_ERRORTYPE VideoDecode::SetState( const Component::State& st )
{
	if ( mOutputPorts[130].bTunneled == false and mBuffer == nullptr ) {
		AllocateBuffers( &mBuffer, 130, true );
		mBufferPtr = mBuffer->pBuffer;
		mOutputPorts[130].bEnabled = true;
		memcpy( &mBufferCopy, mBuffer, sizeof(mBufferCopy) );
	}

	if ( mBuffer ) {
		mNeedData = true;
	}

	OMX_ERRORTYPE ret = IL::Component::SetState(st);

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
	mNeedData = true;
	return OMX_ErrorNone;
}


const bool VideoDecode::needData() const
{
	return mNeedData;
}


const bool VideoDecode::valid()
{
	return width() > 0 and height() > 0;
}


const uint32_t VideoDecode::width()
{
	if ( mDecoderValid ) {
		OMX_PARAM_PORTDEFINITIONTYPE def;
		OMX_INIT_STRUCTURE( def );
		def.nPortIndex = 131;
		GetParameter( OMX_IndexParamPortDefinition, &def );
		return def.format.video.nFrameWidth;
	}
	return 0;
}


const uint32_t VideoDecode::height()
{
	if ( mDecoderValid ) {
		OMX_PARAM_PORTDEFINITIONTYPE def;
		OMX_INIT_STRUCTURE( def );
		def.nPortIndex = 131;
		GetParameter( OMX_IndexParamPortDefinition, &def );
		return def.format.video.nFrameHeight;
	}
	return 0;
}


void VideoDecode::fillInput( uint8_t* pBuf, uint32_t len, bool corrupted )
{
	if ( not mVideoRunning and mDecoderValid ) {
		mVideoRunning = true;

		if ( mOutputPorts[131].bTunneled and mOutputPorts[131].pTunnel ) {
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

			Component::SetupTunnel( 131, mOutputPorts[131].pTunnel, mOutputPorts[131].nTunnelPort );

			mOutputPorts[131].pTunnel->SendCommand( OMX_CommandPortEnable, mOutputPorts[131].nTunnelPort, nullptr );
			mOutputPorts[131].pTunnel->SetState( StateExecuting );
			SendCommand( OMX_CommandPortEnable, 131, nullptr );
		}
	}

	if ( mBuffer ) {
/*
		uint64_t wait_start = ticks64();
		bool broken = false;
		uint64_t broken_time = 0;
		while ( mFirstData == false and mNeedData == false ) {
			if ( ticks64() - wait_start >= 50 * 1000 ) {
				// EmptyBufferDone() has not been called in the last 50ms, decoder has probably stalled
				broken_time = ticks64() - wait_start;
				broken = true;
				break;
			}
			usleep( 20 );
		}
		if ( broken ) {
			// If the decoder stalled, flush both input and output ports
			printf( "Decoder broken (took %llu ms), flushing..\n", broken_time / 1000 );
			SendCommand( OMX_CommandFlush, 130, nullptr );
			SendCommand( OMX_CommandFlush, 131, nullptr );
			mFirstData = true;
		}
		mNeedData = false;
*/
		// Ensure that everything is ok
		if ( len > mBuffer->nAllocLen ) {
			printf( "LEAK : %d > %d\n", len, mBuffer->nAllocLen );
			len = mBuffer->nAllocLen;
			PrintBuffer( pBuf, len );
			fflush( stdout );
		}
		if ( mBuffer->pBuffer != mBufferPtr ) {
			printf( "LEAK : %p != %p\n", mBuffer->pBuffer, mBufferPtr );
// 			mBuffer->pBuffer = mBufferPtr;
			memcpy( mBuffer, &mBufferCopy, sizeof(mBufferCopy) );
			PrintBuffer( pBuf, len );
			fflush( stdout );
		}

		// Manually copy buffer, since libcofi_rpi's memcpy causes random segfault (missalign?)
		uint32_t* start = (uint32_t*)mBufferPtr;
		uint32_t* end = start + ( len >> 2 ) + 1;
		uint32_t* copy = (uint32_t*)pBuf;
		while ( start < end ) {
			*(start++) = *(copy++);
		}

		// Send buffer to GPU
		mBuffer->nTimeStamp = { 0, 0 };
		mBuffer->nFilledLen = len;
		mBuffer->nFlags = OMX_BUFFERFLAG_ENDOFFRAME | ( corrupted ? OMX_BUFFERFLAG_DATACORRUPT : 0 ) | ( mFirstData ? OMX_BUFFERFLAG_STARTTIME : OMX_BUFFERFLAG_TIME_UNKNOWN );
// 		printf( "Filling buffer\n" );
		OMX_ERRORTYPE err = ((OMX_COMPONENTTYPE*)mHandle)->EmptyThisBuffer( mHandle, mBuffer );
		static int err_cnt = 0;
		if ( err != OMX_ErrorNone ) {
			printf( "EmptyThisBuffer error (#%d) : 0x%08X (bufSize : %d ; corrupted : %d)\n", err_cnt, (uint32_t)err, len, corrupted );
			PrintBuffer( pBuf, len );
			fflush( stdout );
			err_cnt++;
		} else {
// 			printf( "Filling buffer OK\n" );
		}

		mFirstData = false;
	}
}


OMX_ERRORTYPE VideoDecode::FillBufferDone( OMX_BUFFERHEADERTYPE* buf )
{
	std::unique_lock<std::mutex> locker( mDataAvailableMutex );
	mDataAvailable = true;
	mDataAvailableCond.notify_all();
	return OMX_ErrorNone;
}


const bool VideoDecode::dataAvailable() const
{
	return mDataAvailable;
}


uint32_t VideoDecode::getOutputData( uint8_t* pBuf, bool wait )
{
	if ( mOutputPorts[131].bTunneled == false and mOutputBuffer == nullptr ) {
		AllocateBuffers( &mOutputBuffer, 131, true );
		mOutputPorts[131].bEnabled = true;
	}

	uint32_t datalen = 0;

	std::unique_lock<std::mutex> locker( mDataAvailableMutex );

	if ( mDataAvailable ) {
		memcpy( pBuf, mOutputBuffer->pBuffer, mOutputBuffer->nFilledLen );
		datalen = mOutputBuffer->nFilledLen;
		mDataAvailable = false;
	} else if ( wait ) {
		mDataAvailableCond.wait( locker );
		memcpy( pBuf, mOutputBuffer->pBuffer, mOutputBuffer->nFilledLen );
		datalen = mOutputBuffer->nFilledLen;
		mDataAvailable = false;
	} else {
		return OMX_ErrorOverflow;
	}

	locker.unlock();

	if ( mOutputBuffer ) {
		OMX_ERRORTYPE err = ((OMX_COMPONENTTYPE*)mHandle)->FillThisBuffer( mHandle, mOutputBuffer );
		if ( err != OMX_ErrorNone ) {
			return err;
		}
	}

	return datalen;
}


static void PrintBuffer( void* _buf, int size )
{
	unsigned char* buf = (unsigned char*)_buf;
	int b;
	for(b=0; b<size; b++){
		printf( "%02X ", buf[b]);
		if(b > 0 && (b+1) % 16 == 0){
			printf( "\n");
		}
	}
	printf( "\n");
}
