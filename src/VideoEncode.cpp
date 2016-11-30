#include "VideoEncode.h"
#include <unistd.h>
#include <IL/OMX_Types.h>
#include <IL/OMX_Component.h>
#include <IL/OMX_Broadcom.h>
#include <iostream>

using namespace IL;

VideoEncode::VideoEncode( uint32_t bitrate_kbps, const CodingType& coding_type, bool verbose )
	: Component( "OMX.broadcom.video_encode", { PortInit( 200, Video ) }, { PortInit( 201, Video ) }, verbose )
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
	bitrate.eControlRate = OMX_Video_ControlRateVariableSkipFrames;
	bitrate.nTargetBitrate = bitrate_kbps * 1024;
	SetParameter( OMX_IndexParamVideoBitrate, &bitrate );

	if ( coding_type == CodingAVC ) {
		setIDRPeriod( 1 );

		OMX_PARAM_U32TYPE MBmode;
		OMX_INIT_STRUCTURE( MBmode );
		MBmode.nPortIndex = 201;
		MBmode.nU32 = 4; //1=4x4, 2=8x8, 4=16x16
		SetConfig( OMX_IndexConfigBrcmVideoH264IntraMBMode, &MBmode );

		OMX_CONFIG_BOOLEANTYPE headerOnOpen;
		OMX_INIT_STRUCTURE( headerOnOpen );
		headerOnOpen.bEnabled = OMX_TRUE;
		SetConfig( OMX_IndexParamBrcmHeaderOnOpen, &headerOnOpen );

		OMX_CONFIG_PORTBOOLEANTYPE inlinePPSSPS;
		OMX_INIT_STRUCTURE( inlinePPSSPS );
		inlinePPSSPS.nPortIndex = 201;
		inlinePPSSPS.bEnabled = OMX_TRUE;
		SetParameter( OMX_IndexParamBrcmVideoAVCInlineHeaderEnable, &inlinePPSSPS );

		OMX_CONFIG_PORTBOOLEANTYPE inlineVectors;
		OMX_INIT_STRUCTURE( inlineVectors );
		inlineVectors.nPortIndex = 201;
		inlinePPSSPS.bEnabled = OMX_TRUE;
		SetParameter( OMX_IndexParamBrcmVideoAVCInlineVectorsEnable, &inlineVectors );

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

		OMX_VIDEO_EEDE_ENABLE eede_enable;
		OMX_INIT_STRUCTURE( eede_enable );
		eede_enable.nPortIndex = 201;
		eede_enable.enable = OMX_TRUE;
		SetParameter( OMX_IndexParamBrcmEEDEEnable, &eede_enable );

		OMX_VIDEO_EEDE_LOSSRATE eede_lossrate;
		OMX_INIT_STRUCTURE( eede_lossrate );
		eede_lossrate.nPortIndex = 201;
		eede_lossrate.loss_rate = 32;
		SetParameter( OMX_IndexParamBrcmEEDELossRate, &eede_lossrate );

		OMX_PARAM_BRCMVIDEOAVCSEIENABLETYPE avcsei;
		OMX_INIT_STRUCTURE( avcsei );
		avcsei.nPortIndex = 201;
		avcsei.bEnable = OMX_TRUE;
		SetParameter( OMX_IndexParamBrcmVideoAVCSEIEnable, &avcsei );

		OMX_VIDEO_PARAM_AVCTYPE avc;
		OMX_INIT_STRUCTURE( avc );
		avc.nPortIndex = 201;
		GetParameter( OMX_IndexParamVideoAvc, &avc );
	// 	avc.nSliceHeaderSpacing = 15; // ??
		avc.nPFrames = 2;
		avc.nBFrames = 0;
		avc.bUseHadamard = OMX_TRUE;
		avc.nRefFrames = 0;
// 		avc.nRefIdx10ActiveMinus1 = 0;
// 		avc.nRefIdx11ActiveMinus1 = 0;
		avc.bEnableUEP = OMX_FALSE;
		avc.bEnableFMO = OMX_TRUE;
		avc.bEnableASO = OMX_TRUE;
		avc.bEnableRS = OMX_TRUE;
		avc.eProfile = OMX_VIDEO_AVCProfileHigh;
		avc.eLevel = OMX_VIDEO_AVCLevel4;
// 		avc.eProfile = OMX_VIDEO_AVCProfileBaseline;
// 		avc.eLevel = OMX_VIDEO_AVCLevel31;
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

		OMX_CONFIG_BOOLEANTYPE temporal_denoise;
		OMX_INIT_STRUCTURE( temporal_denoise );
		temporal_denoise.bEnabled = OMX_FALSE;
		SetConfig( OMX_IndexConfigTemporalDenoiseEnable, &temporal_denoise );
	}

	SendCommand( OMX_CommandStateSet, OMX_StateIdle, nullptr );
}


VideoEncode::~VideoEncode()
{
}


OMX_ERRORTYPE VideoEncode::SetupTunnel( Component* next, uint8_t port_input )
{
	return Component::SetupTunnel( 201, next, port_input );
}


OMX_ERRORTYPE VideoEncode::SetState( const Component::State& st )
{
	if ( mOutputPorts[201].bTunneled == false and mBuffer == nullptr ) {
		AllocateBuffers( &mBuffer, 201, true );
		mOutputPorts[201].bEnabled = true;
		mBufferPtr = mBuffer->pBuffer;
	}

	if ( mInputPorts[200].bTunneled ) {
		OMX_PARAM_PORTDEFINITIONTYPE def;
		OMX_INIT_STRUCTURE( def );
		def.nPortIndex = mInputPorts[200].nTunnelPort;
		mInputPorts[200].pTunnel->GetParameter( OMX_IndexParamPortDefinition, &def );
		def.eDomain = OMX_PortDomainVideo;
		def.nPortIndex = 200;
		def.format.video.nStride = def.format.video.nFrameWidth;
		def.format.video.nSliceHeight = def.format.video.nFrameHeight;
		def.format.video.eCompressionFormat = (OMX_VIDEO_CODINGTYPE)mCodingType;
		def.format.video.eColorFormat = OMX_COLOR_FormatYUV420PackedPlanar;
		SetParameter( OMX_IndexParamPortDefinition, &def );
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
	GetParameter( OMX_IndexConfigVideoAVCIntraPeriod, &idr );
	idr.nIDRPeriod = period;
// 	idr.nPFrames = period;
	return SetParameter( OMX_IndexConfigVideoAVCIntraPeriod, &idr );
}


OMX_ERRORTYPE VideoEncode::FillBufferDone( OMX_BUFFERHEADERTYPE* buf )
{
	std::unique_lock<std::mutex> locker( mDataAvailableMutex );
	mDataAvailable = true;
	mDataAvailableCond.notify_all();
	return OMX_ErrorNone;
}


const bool VideoEncode::dataAvailable() const
{
	return mDataAvailable;
}


uint32_t VideoEncode::getOutputData( uint8_t* pBuf, bool wait )
{
	uint32_t datalen = 0;

	std::unique_lock<std::mutex> locker( mDataAvailableMutex );

	if ( mDataAvailable ) {
		memcpy( pBuf, mBuffer->pBuffer, mBuffer->nFilledLen );
		datalen = mBuffer->nFilledLen;
		mDataAvailable = false;
	} else if ( wait ) {
		mDataAvailableCond.wait( locker );
		memcpy( pBuf, mBuffer->pBuffer, mBuffer->nFilledLen );
		datalen = mBuffer->nFilledLen;
		mDataAvailable = false;
	} else {
		return OMX_ErrorOverflow;
	}

	locker.unlock();

	if ( mBuffer ) {
		OMX_ERRORTYPE err = ((OMX_COMPONENTTYPE*)mHandle)->FillThisBuffer( mHandle, mBuffer );
		if ( err != OMX_ErrorNone ) {
			return err;
		}
	}

	return datalen;
}
