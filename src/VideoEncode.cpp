#include "VideoEncode.h"
#include <IL/OMX_Types.h>
#include <IL/OMX_Component.h>
#include <IL/OMX_Broadcom.h>

using namespace IL;

VideoEncode::VideoEncode( uint32_t bitrate_kbps, const CodingType& coding_type, bool verbose )
	: Component( "OMX.broadcom.video_encode", { 200 }, { 201 }, verbose )
	, mCodingType( coding_type )
	, mBuffer( nullptr )
	, mDataAvailable( false )
{
	OMX_VIDEO_PARAM_PORTFORMATTYPE pfmt;
	OMX_INIT_STRUCTURE( pfmt );
	pfmt.nPortIndex = 201;
	pfmt.eCompressionFormat = (OMX_VIDEO_CODINGTYPE)coding_type;
	SetParameter( OMX_IndexParamVideoPortFormat, &pfmt );

	OMX_VIDEO_PARAM_BITRATETYPE bitrate;
	OMX_INIT_STRUCTURE( bitrate );
	bitrate.nPortIndex = 201;
	bitrate.eControlRate = OMX_Video_ControlRateVariable;
	bitrate.nTargetBitrate = bitrate_kbps * 1024;
	SetParameter( OMX_IndexParamVideoBitrate, &bitrate );

	OMX_CONFIG_BOOLEANTYPE headerOnOpen;
	OMX_INIT_STRUCTURE( headerOnOpen );
	headerOnOpen.bEnabled = OMX_TRUE;
	SetConfig( OMX_IndexParamBrcmHeaderOnOpen, &headerOnOpen );

	if ( coding_type == CodingAVC ) {
// 		setIDRPeriod( 30 );

		OMX_CONFIG_PORTBOOLEANTYPE inlinePPSSPS;
		OMX_INIT_STRUCTURE( inlinePPSSPS );
		inlinePPSSPS.nPortIndex = 201;
		inlinePPSSPS.bEnabled = OMX_TRUE;
		SetParameter( OMX_IndexParamBrcmVideoAVCInlineHeaderEnable, &inlinePPSSPS );
/*
		OMX_VIDEO_PARAM_PROFILELEVELTYPE profile;
		OMX_INIT_STRUCTURE( profile );
		profile.nPortIndex = 201;
		profile.eProfile = OMX_VIDEO_AVCProfileHigh;
		profile.eLevel = OMX_VIDEO_AVCLevel4;
		SetParameter( OMX_IndexParamVideoProfileLevelCurrent, &profile );
*/
		OMX_CONFIG_BOOLEANTYPE lowLatency;
		OMX_INIT_STRUCTURE( lowLatency );
		lowLatency.bEnabled = OMX_TRUE;
		SetConfig( OMX_IndexConfigBrcmVideoH264LowLatency, &lowLatency );
	}

	SendCommand( OMX_CommandStateSet, OMX_StateIdle, nullptr );
}


VideoEncode::~VideoEncode()
{
}


OMX_ERRORTYPE VideoEncode::SetState( const Component::State& st )
{
	if ( mOutputPorts[201].bTunneled == false and mBuffer == nullptr ) {
		AllocateBuffers( &mBuffer, 201, true );
		mOutputPorts[201].bEnabled = true;
	}

	OMX_ERRORTYPE ret = IL::Component::SetState(st);
	if ( ret != OMX_ErrorNone ) {
		return ret;
	}

	if ( mBuffer ) {
		ret = ((OMX_COMPONENTTYPE*)mHandle)->FillThisBuffer( mHandle, mBuffer );
	}
	return ret;
}


OMX_ERRORTYPE VideoEncode::setIDRPeriod( uint32_t period )
{
	if ( mCodingType != CodingAVC ) {
		return OMX_ErrorUnsupportedSetting;
	}

	OMX_VIDEO_CONFIG_AVCINTRAPERIOD idr;
	OMX_INIT_STRUCTURE( idr );
	idr.nPortIndex = 201;
	idr.nIDRPeriod = period;
	idr.nPFrames = 0;
	return SetParameter( OMX_IndexConfigVideoAVCIntraPeriod, &idr );
}


OMX_ERRORTYPE VideoEncode::FillBufferDone( OMX_BUFFERHEADERTYPE* buf )
{
	std::unique_lock<std::mutex> locker( mDataAvailableMutex );
	mDataAvailable = true;
	mDataAvailableCond.notify_all();
	return Component::FillBufferDone( buf );
}


const bool VideoEncode::dataAvailable() const
{
	return mDataAvailable;
}


uint32_t VideoEncode::getOutputData( uint8_t* pBuf, bool wait )
{
	uint32_t datalen = 0;

	mDataAvailableMutex.lock();

	if ( mDataAvailable ) {
		memcpy( pBuf, mBuffer->pBuffer, mBuffer->nFilledLen );
		datalen = mBuffer->nFilledLen;
		mDataAvailable = false;
		mDataAvailableMutex.unlock();
	} else if ( wait ) {
		mDataAvailableMutex.unlock();
		std::unique_lock<std::mutex> locker( mDataAvailableMutex );
		mDataAvailableCond.wait( locker );
		memcpy( pBuf, mBuffer->pBuffer, mBuffer->nFilledLen );
		datalen = mBuffer->nFilledLen;
		mDataAvailable = false;
	}

	if ( mBuffer ) {
		((OMX_COMPONENTTYPE*)mHandle)->FillThisBuffer( mHandle, mBuffer );
	}

	return datalen;
}
