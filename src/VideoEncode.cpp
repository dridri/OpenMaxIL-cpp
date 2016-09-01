#include "VideoEncode.h"
#include <unistd.h>
#include <IL/OMX_Types.h>
#include <IL/OMX_Component.h>
#include <IL/OMX_Broadcom.h>
#include <iostream>

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

	if ( coding_type == CodingAVC ) {
		setIDRPeriod( 30 );

		OMX_CONFIG_BOOLEANTYPE headerOnOpen;
		OMX_INIT_STRUCTURE( headerOnOpen );
		headerOnOpen.bEnabled = OMX_TRUE;
		SetConfig( OMX_IndexParamBrcmHeaderOnOpen, &headerOnOpen );

		OMX_CONFIG_PORTBOOLEANTYPE inlinePPSSPS;
		OMX_INIT_STRUCTURE( inlinePPSSPS );
		inlinePPSSPS.nPortIndex = 201;
		inlinePPSSPS.bEnabled = OMX_TRUE;
		SetParameter( OMX_IndexParamBrcmVideoAVCInlineHeaderEnable, &inlinePPSSPS );

		OMX_VIDEO_PARAM_PROFILELEVELTYPE profile;
		OMX_INIT_STRUCTURE( profile );
		profile.nPortIndex = 201;
		profile.eProfile = OMX_VIDEO_AVCProfileHigh;
		profile.eLevel = OMX_VIDEO_AVCLevel4;
		SetParameter( OMX_IndexParamVideoProfileLevelCurrent, &profile );

		OMX_CONFIG_BOOLEANTYPE lowLatency;
		OMX_INIT_STRUCTURE( lowLatency );
		lowLatency.bEnabled = OMX_TRUE;
		SetConfig( OMX_IndexConfigBrcmVideoH264LowLatency, &lowLatency );

		OMX_VIDEO_PARAM_QUANTIZATIONTYPE quantization;
		OMX_INIT_STRUCTURE( quantization );
		quantization.nPortIndex = 201;
		quantization.nQpI = 50;
		quantization.nQpP = 50;
		quantization.nQpB = 0;
		SetParameter( OMX_IndexParamVideoQuantization, &quantization );

		OMX_VIDEO_PARAM_AVCTYPE avc;
		OMX_INIT_STRUCTURE( avc );
		avc.nPortIndex = 201;
		GetParameter( OMX_IndexParamVideoAvc, &avc );
	// 	avc.nSliceHeaderSpacing = 15; // ??
		avc.nPFrames = 10;
		avc.nBFrames = 0;
		avc.bUseHadamard = OMX_TRUE;
// 		avc.nRefFrames = 2;
// 		avc.nRefIdx10ActiveMinus1 = 0;
// 		avc.nRefIdx11ActiveMinus1 = 0;
		avc.bEnableUEP = OMX_FALSE;
		avc.bEnableFMO = OMX_TRUE;
		avc.bEnableASO = OMX_TRUE;
		avc.bEnableRS = OMX_TRUE;
		avc.eProfile = OMX_VIDEO_AVCProfileHigh; //OMX_VIDEO_AVCProfileBaseline
		avc.eLevel = OMX_VIDEO_AVCLevel4; //OMX_VIDEO_AVCLevel31
		avc.nAllowedPictureTypes = OMX_VIDEO_PictureTypeI | OMX_VIDEO_PictureTypeP | OMX_VIDEO_PictureTypeEI | OMX_VIDEO_PictureTypeEP;
		avc.bFrameMBsOnly = OMX_TRUE;
		avc.bMBAFF = OMX_FALSE;
// 		avc.bEntropyCodingCABAC = OMX_FALSE;
// 		avc.bWeightedPPrediction = OMX_TRUE; // ??
		avc.nWeightedBipredicitonMode = OMX_FALSE;
		avc.bconstIpred = OMX_TRUE;
// 		avc.bDirect8x8Inference = OMX_FALSE;
// 		avc.bDirectSpatialTemporal = OMX_FALSE;
// 		avc.nCabacInitIdc = 0;
		avc.eLoopFilterMode = OMX_VIDEO_AVCLoopFilterDisable;
		if ( SetParameter( OMX_IndexParamVideoAvc, &avc ) != OMX_ErrorNone ) {
			exit(0);
		}
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
		usleep( 1000 * 500 );
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
// 	printf( "FillBufferDone 1 (%p, %d, %d)\n", mBuffer->pBuffer, mBuffer->nOffset, mBuffer->nFilledLen );
	std::unique_lock<std::mutex> locker( mDataAvailableMutex );
// 	std::cout << "FillBufferDone 2\n";
	mDataAvailable = true;
	mDataAvailableCond.notify_all();
// 	std::cout << "FillBufferDone 3\n";
	return OMX_ErrorNone;
}


const bool VideoEncode::dataAvailable() const
{
	return mDataAvailable;
}


uint32_t VideoEncode::getOutputData( uint8_t* pBuf, bool wait )
{
// 	std::cout << "getOutputData 1\n";
	uint32_t datalen = 0;

	mDataAvailableMutex.lock();
// 	std::cout << "getOutputData 2\n";

	if ( mDataAvailable ) {
// 		std::cout << "getOutputData 3\n";
		memcpy( pBuf, mBuffer->pBuffer, mBuffer->nFilledLen );
		datalen = mBuffer->nFilledLen;
// 		std::cout << "getOutputData 4\n";
		mDataAvailable = false;
		mDataAvailableMutex.unlock();
// 		std::cout << "getOutputData 5\n";
	} else if ( wait ) {
// 		std::cout << "getOutputData 6\n";
		mDataAvailableMutex.unlock();
// 		std::cout << "getOutputData 7\n";
		std::unique_lock<std::mutex> locker( mDataAvailableMutex );
// 		std::cout << "getOutputData 8\n";
		mDataAvailableCond.wait( locker );
// 		std::cout << "getOutputData 9\n";
		memcpy( pBuf, mBuffer->pBuffer, mBuffer->nFilledLen );
		datalen = mBuffer->nFilledLen;
		mDataAvailable = false;
// 		std::cout << "getOutputData 10\n";
	}
// 	std::cout << "getOutputData 11\n";

	if ( mBuffer ) {
		OMX_ERRORTYPE err = ((OMX_COMPONENTTYPE*)mHandle)->FillThisBuffer( mHandle, mBuffer );
// 		printf( "FillThisBuffer error : %08X\n", err );
	}
// 	std::cout << "getOutputData 12\n";

	return datalen;
}
