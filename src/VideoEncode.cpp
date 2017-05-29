#include "VideoEncode.h"
#include <unistd.h>
#include <IL/OMX_Types.h>
#include <IL/OMX_Component.h>
#include <IL/OMX_Broadcom.h>
#include <iostream>

using namespace IL;

VideoEncode::VideoEncode( uint32_t bitrate_kbps, const CodingType& coding_type, bool live_mode, bool verbose )
	: Component( "OMX.broadcom.video_encode", { PortInit( 200, Video ) }, { PortInit( 201, Video ) }, verbose )
	, mCodingType( coding_type )
	, mBuffer( nullptr )
	, mBufferPtr( nullptr )
	, mDataAvailable( false )
{
	OMX_PARAM_PORTDEFINITIONTYPE def;
	OMX_INIT_STRUCTURE( def );
	def.nPortIndex = 201;
	GetParameter( OMX_IndexParamPortDefinition, &def );
	def.format.video.nFrameWidth = 1280;
	def.format.video.nFrameHeight = 720;
	def.format.video.nStride = 1280;
	def.format.video.nSliceHeight = 720;
	def.format.video.xFramerate = 30 << 16;
	def.format.video.eCompressionFormat = (OMX_VIDEO_CODINGTYPE)coding_type;
	def.format.video.eColorFormat = OMX_COLOR_FormatYUV420PackedPlanar;
	def.format.video.nBitrate = bitrate_kbps * 1024;
	SetParameter( OMX_IndexParamPortDefinition, &def );

	OMX_VIDEO_PARAM_PORTFORMATTYPE pfmt;
	OMX_INIT_STRUCTURE( pfmt );
	pfmt.nPortIndex = 201;
	pfmt.eCompressionFormat = (OMX_VIDEO_CODINGTYPE)coding_type;
	SetParameter( OMX_IndexParamVideoPortFormat, &pfmt );
/*
	OMX_CONFIG_BRCMUSEPROPRIETARYCALLBACKTYPE propCb;
	OMX_INIT_STRUCTURE( propCb );
	propCb.nPortIndex = 200;
	propCb.bEnable = OMX_TRUE;
	SetConfig( OMX_IndexConfigBrcmUseProprietaryCallback, &propCb );
*/
	OMX_VIDEO_PARAM_BITRATETYPE bitrate;
	OMX_INIT_STRUCTURE( bitrate );
	bitrate.nPortIndex = 201;
// 	bitrate.eControlRate = OMX_Video_ControlRateConstantSkipFrames;
	bitrate.eControlRate = OMX_Video_ControlRateVariableSkipFrames;
	bitrate.nTargetBitrate = bitrate_kbps * 1024;
	SetParameter( OMX_IndexParamVideoBitrate, &bitrate );

	OMX_CONFIG_BOOLEANTYPE immutableInput;
	OMX_INIT_STRUCTURE( immutableInput );
	immutableInput.bEnabled = OMX_TRUE;
	SetParameter( OMX_IndexParamBrcmImmutableInput, &immutableInput );

	if ( coding_type == CodingAVC ) {
		OMX_CONFIG_BOOLEANTYPE headerOnOpen;
		OMX_INIT_STRUCTURE( headerOnOpen );
		headerOnOpen.bEnabled = OMX_TRUE;
		SetConfig( OMX_IndexParamBrcmHeaderOnOpen, &headerOnOpen );

		if ( live_mode ) {
			setIDRPeriod( 1 );
	/*
			OMX_PARAM_U32TYPE MBmode;
			OMX_INIT_STRUCTURE( MBmode );
			MBmode.nPortIndex = 201;
			MBmode.nU32 = 4; //1=4x4, 2=8x8, 4=16x16
			SetConfig( OMX_IndexConfigBrcmVideoH264IntraMBMode, &MBmode );
	*/
	/*
			OMX_CONFIG_PORTBOOLEANTYPE inlinePPSSPS;
			OMX_INIT_STRUCTURE( inlinePPSSPS );
			inlinePPSSPS.nPortIndex = 201;
			inlinePPSSPS.bEnabled = OMX_TRUE;
			SetParameter( OMX_IndexParamBrcmVideoAVCInlineHeaderEnable, &inlinePPSSPS );
	*/
	/*
			OMX_CONFIG_PORTBOOLEANTYPE inlineVectors;
			OMX_INIT_STRUCTURE( inlineVectors );
			inlineVectors.nPortIndex = 201;
			inlineVectors.bEnabled = OMX_TRUE;
			SetParameter( OMX_IndexParamBrcmVideoAVCInlineVectorsEnable, &inlineVectors );
	*/
			OMX_CONFIG_BOOLEANTYPE lowLatency;
			OMX_INIT_STRUCTURE( lowLatency );
			lowLatency.bEnabled = OMX_TRUE;
			SetConfig( OMX_IndexConfigBrcmVideoH264LowLatency, &lowLatency );

			OMX_VIDEO_EEDE_ENABLE eede_enable;
			OMX_INIT_STRUCTURE( eede_enable );
			eede_enable.nPortIndex = 201;
			eede_enable.enable = OMX_TRUE;
			SetParameter( OMX_IndexParamBrcmEEDEEnable, &eede_enable );

			OMX_VIDEO_EEDE_LOSSRATE eede_lossrate;
			OMX_INIT_STRUCTURE( eede_lossrate );
			eede_lossrate.nPortIndex = 201;
			eede_lossrate.loss_rate = 50;
			SetParameter( OMX_IndexParamBrcmEEDELossRate, &eede_lossrate );
	/*
			OMX_PARAM_BRCMVIDEOAVCSEIENABLETYPE avcsei;
			OMX_INIT_STRUCTURE( avcsei );
			avcsei.nPortIndex = 201;
			avcsei.bEnable = OMX_TRUE;
			SetParameter( OMX_IndexParamBrcmVideoAVCSEIEnable, &avcsei );
	*/
			OMX_VIDEO_PARAM_AVCTYPE avc;
			OMX_INIT_STRUCTURE( avc );
			avc.nPortIndex = 201;
			GetParameter( OMX_IndexParamVideoAvc, &avc );
		// 	avc.nSliceHeaderSpacing = 15; // ??
			avc.nPFrames = 1;
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
	// 		avc.eProfile = OMX_VIDEO_AVCProfileBaseline;
			avc.eLevel = OMX_VIDEO_AVCLevel42;
	// 		avc.eLevel = OMX_VIDEO_AVCLevel4;
	// 		avc.eLevel = OMX_VIDEO_AVCLevel32;
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
				printf( "ERROR SETTING OMX_IndexParamVideoAvc\n" );
// 				exit(0);
			}
	/*
			OMX_CONFIG_BOOLEANTYPE temporal_denoise;
			OMX_INIT_STRUCTURE( temporal_denoise );
			temporal_denoise.bEnabled = OMX_FALSE;
			SetConfig( OMX_IndexConfigTemporalDenoiseEnable, &temporal_denoise );
	*/
		}
	}

// 	SendCommand( OMX_CommandStateSet, OMX_StateIdle, nullptr );
}


VideoEncode::~VideoEncode()
{
}


OMX_U8* VideoEncode::buffer() const
{
	return mBufferPtr;
}


OMX_U32 VideoEncode::bufferLength() const
{
	if ( mBuffer ) {
		return mBuffer->nFilledLen;
	}
	return 0;
}


OMX_ERRORTYPE VideoEncode::setFramerate( uint32_t fps )
{
	OMX_CONFIG_FRAMERATETYPE framerate;
	OMX_INIT_STRUCTURE( framerate );
	framerate.nPortIndex = 201;
	framerate.xEncodeFramerate = fps << 16;
	return SetParameter( OMX_IndexConfigVideoFramerate, &framerate );
}


OMX_ERRORTYPE VideoEncode::setInlinePPSSPS( bool en )
{
	OMX_CONFIG_PORTBOOLEANTYPE inlinePPSSPS;
	OMX_INIT_STRUCTURE( inlinePPSSPS );
	inlinePPSSPS.nPortIndex = 201;
	inlinePPSSPS.bEnabled = (OMX_BOOL)en;
	return SetParameter( OMX_IndexParamBrcmVideoAVCInlineHeaderEnable, &inlinePPSSPS );
}


void VideoEncode::RequestIFrame()
{
	OMX_CONFIG_BOOLEANTYPE request;
	OMX_INIT_STRUCTURE( request );
	request.bEnabled = OMX_TRUE;
	SetConfig( OMX_IndexConfigBrcmVideoRequestIFrame, &request );
}


OMX_ERRORTYPE VideoEncode::SetupTunnel( Component* next, uint8_t port_input )
{
	return Component::SetupTunnel( 201, next, port_input );
}


OMX_ERRORTYPE VideoEncode::SetState( const Component::State& st )
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;

	if ( st == StateExecuting ) {
		bool first_setup = false;
		mBuffer = mOutputPorts[201].buffer;
		if ( mOutputPorts[201].bTunneled == false and mBuffer == nullptr ) {
			first_setup = true;
			AllocateOutputBuffer( 201 );
			mBuffer = mOutputPorts[201].buffer;
		}
		if ( mBufferPtr == nullptr ) {
			mBufferPtr = mBuffer->pBuffer;
		}
/*
		if ( mInputPorts[200].bTunneled ) {
			OMX_PARAM_PORTDEFINITIONTYPE def;
			OMX_INIT_STRUCTURE( def );
// 			def.nPortIndex = mInputPorts[200].nTunnelPort;
// 			mInputPorts[200].pTunnel->GetParameter( OMX_IndexParamPortDefinition, &def );
			def.nPortIndex = 200;
			GetParameter( OMX_IndexParamPortDefinition, &def );
			def.eDomain = OMX_PortDomainVideo;
			def.nPortIndex = 200;
			def.format.video.nStride = def.format.video.nFrameWidth;
			def.format.video.nSliceHeight = def.format.video.nFrameHeight;
			def.format.video.eCompressionFormat = (OMX_VIDEO_CODINGTYPE)mCodingType;
			def.format.video.eColorFormat = OMX_COLOR_FormatYUV420PackedPlanar;
			SetParameter( OMX_IndexParamPortDefinition, &def );
		}
*/
		ret = IL::Component::SetState(st);
		if ( ret != OMX_ErrorNone ) {
			return ret;
		}

		if (/* first_setup and*/ mBuffer ) {
			usleep( 1000 * 500 );
			ret = ((OMX_COMPONENTTYPE*)mHandle)->FillThisBuffer( mHandle, mBuffer );
		}
	} else {
		ret = IL::Component::SetState(st);
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


const std::map< uint32_t, uint8_t* >& VideoEncode::headers() const
{
	return mHeaders;
}


OMX_ERRORTYPE VideoEncode::FillBufferDone( OMX_BUFFERHEADERTYPE* buf )
{
	std::unique_lock<std::mutex> locker( mDataAvailableMutex );
	if ( mBuffer->nFilledLen > 0 and mBuffer->nFilledLen < 32 and mHeaders.find( mBuffer->nFilledLen ) == mHeaders.end() ) {
		uint8_t* data = (uint8_t*)malloc( mBuffer->nFilledLen );
		memcpy( data, mBufferPtr, mBuffer->nFilledLen );
		mHeaders.emplace( std::make_pair( mBuffer->nFilledLen, data ) );
	}
	mDataAvailable = true;
	mDataAvailableCond.notify_all();
	return OMX_ErrorNone;
}


const bool VideoEncode::dataAvailable( bool wait )
{
	if ( not wait or mDataAvailable ) {
		return mDataAvailable;
	}

	std::unique_lock<std::mutex> locker( mDataAvailableMutex );
	mDataAvailableCond.wait( locker );
	locker.unlock();

	return mDataAvailable;
}


void VideoEncode::fillBuffer()
{
	if ( mBuffer ) {
		OMX_ERRORTYPE err = ((OMX_COMPONENTTYPE*)mHandle)->FillThisBuffer( mHandle, mBuffer );
	}
}


uint32_t VideoEncode::getOutputData( uint8_t* pBuf, bool wait )
{
	uint32_t datalen = 0;

	std::unique_lock<std::mutex> locker( mDataAvailableMutex );

	if ( mDataAvailable ) {
		if ( pBuf ) {
			memcpy( pBuf, mBuffer->pBuffer, mBuffer->nFilledLen );
		}
		datalen = mBuffer->nFilledLen;
		mDataAvailable = false;
	} else if ( wait ) {
		mDataAvailableCond.wait( locker );
		if ( pBuf ) {
			memcpy( pBuf, mBuffer->pBuffer, mBuffer->nFilledLen );
		}
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
/*
	if ( datalen < 32 and mHeaders.find( datalen ) == mHeaders.end() ) {
		uint8_t* data = (uint8_t*)malloc( datalen );
		memcpy( data, pBuf, datalen );
		mHeaders.emplace( std::make_pair( datalen, data ) );
	}
*/
	return datalen;
}
