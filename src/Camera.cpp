#include <unistd.h>
#include "Camera.h"
#include <cmath>
#include <bcm_host.h>
#include <interface/vcsm/user-vcsm.h>
#include <IL/OMX_Index.h>
#include <IL/OMX_Video.h>
#include <IL/OMX_Types.h>
#include <IL/OMX_Component.h>
#include <IL/OMX_Broadcom.h>

using namespace IL;

OMX_BUFFERHEADERTYPE* test_buffer;

Camera::Camera( uint32_t width, uint32_t height, uint32_t device_number, bool high_speed, uint32_t sensor_mode, bool verbose )
	: Component( "OMX.broadcom.camera", { PortInit( 73, Clock ) }, { PortInit( 70, Video ), PortInit( 71, Video ), PortInit( 72, Image ) }, verbose )
	, mDeviceNumber( device_number )
	, mWidth( width )
	, mHeight( height )
	, mSensorMode( 0 )
	, mReady( false )
	, mVCSMReady( false )
	, mLensShadingAlloc( 0 )
{
	if ( not Component::mHandle ) {
		return;
	}

	Initialize( width, height, sensor_mode );
	if ( high_speed ) {
		HighSpeedMode( sensor_mode );
	} else {
		setSensorMode( sensor_mode );
	}

	while ( not mReady ) {
		usleep( 100 * 1000 );
	}
	if ( mVerbose ) {
		mDebugCallback( 0, "Camera %d ready\n", mDeviceNumber );
	}
}


Camera::~Camera()
{
	if ( mVerbose ) {
		mDebugCallback( 0, "Deleting Camera %d...\n", mDeviceNumber );
	}
	SetCapturing( false );

	if ( mLensShadingAlloc != 0 ) {
		OMX_PARAM_LENSSHADINGOVERRIDETYPE lens_override;
		OMX_INIT_STRUCTURE( lens_override );
		lens_override.bEnabled = OMX_FALSE;
		OMX_ERRORTYPE ret = SetParameter( OMX_IndexParamBrcmLensShadingOverride, &lens_override );
		if ( ret != OMX_ErrorNone ) {
			mDebugCallback( 0, "Cannot disable lens shading override : 0x%08X\n", ret ); fflush(stderr);
		}
		vcsm_free( mLensShadingAlloc );
	}

	if ( mVerbose ) {
		mDebugCallback( 0, "Deleting Camera ok\n" );
	}
}


OMX_ERRORTYPE Camera::SetState( const Component::State& st, bool wait )
{
	OMX_ERRORTYPE err = IL::Component::SetState( st, wait );
	return err;
}


OMX_ERRORTYPE Camera::SetCapturing( bool capturing, uint8_t port )
{
	OMX_CONFIG_PORTBOOLEANTYPE capture;
	OMX_INIT_STRUCTURE( capture );
	capture.nPortIndex = port;
	capture.bEnabled = OMX_TRUE;
	return SetParameter( OMX_IndexConfigPortCapturing, &capture );
}


OMX_ERRORTYPE Camera::SetupTunnelPreview( Component* next, uint8_t port_input )
{
	return SetupTunnel( 70, next, port_input );
}


OMX_ERRORTYPE Camera::SetupTunnelVideo( Component* next, uint8_t port_input )
{
	return SetupTunnel( 71, next, port_input );
}


OMX_ERRORTYPE Camera::SetupTunnelImage( Component* next, uint8_t port_input )
{
	return SetupTunnel( 72, next, port_input );
}


OMX_ERRORTYPE Camera::EventHandler( OMX_EVENTTYPE event, OMX_U32 data1, OMX_U32 data2, OMX_PTR eventdata )
{
	if ( event == OMX_EventParamOrConfigChanged ) {
		mReady = true;
	}
	return IL::Component::EventHandler( event, data1, data2, eventdata );
}


int Camera::Initialize( uint32_t width, uint32_t height, uint32_t sensor_mode )
{
	// Enable callback for camera
	OMX_CONFIG_REQUESTCALLBACKTYPE cbtype;
	OMX_INIT_STRUCTURE( cbtype );
	cbtype.nPortIndex = OMX_ALL;
	cbtype.nIndex = OMX_IndexParamCameraDeviceNumber;
	cbtype.bEnable = OMX_TRUE;
	SetConfig( OMX_IndexConfigRequestCallback, &cbtype );

	// Configure device number
	OMX_PARAM_U32TYPE device;
	OMX_INIT_STRUCTURE( device );
	device.nPortIndex = OMX_ALL;
	device.nU32 = mDeviceNumber;
	SetParameter( OMX_IndexParamCameraDeviceNumber, &device );

	if ( width > 0 && height > 0 ) {
		setResolution( width, height, 71 );
		setResolution( width, height, 72 );
	}
	setFramerate( 30 );
	setBrightness( 50 );
	setContrast( 0 );
	setSaturation( 0 );
	setSharpness( 0 );
	setWhiteBalanceControl( WhiteBalControlAuto );
	setExposureControl( ExposureControlAuto );
	setExposureValue( ExposureMeteringModeAverage, 0, 0, 0 );
	setFrameStabilisation( true );

	return 0;
}


OMX_ERRORTYPE Camera::setFocusOverlay( bool overlay )
{
	OMX_CONFIG_BOOLEANTYPE box;
	OMX_INIT_STRUCTURE( box );
	box.bEnabled = (OMX_BOOL)overlay;
	return SetConfig( OMX_IndexConfigDrawBoxAroundFaces, &box );
}


OMX_ERRORTYPE Camera::setResolution( uint32_t width, uint32_t height, uint8_t port )
{
	mWidth = width;
	mHeight = height;

	OMX_PARAM_PORTDEFINITIONTYPE def;
	OMX_INIT_STRUCTURE( def );

	if ( port == 70 or port == 71 ) {
		if ( port == 71 ) {
// 			port = 70;
		}
		def.nPortIndex = 70;
		GetParameter( OMX_IndexParamPortDefinition, &def );
		def.format.video.nFrameWidth  = mWidth;
		def.format.video.nFrameHeight = mHeight;
// 		def.format.video.nFrameWidth  = std::min( mWidth, 1640u );
// 		def.format.video.nFrameHeight = std::min( mHeight, 1232u );
		printf( "===============> outputPorts()[%d].bDisableProprietary : %d\n", 70, outputPorts()[70].bDisableProprietary );
		if ( outputPorts()[70].bDisableProprietary ) {
			def.format.video.nSliceHeight = mHeight;
		} else {
			def.format.video.nSliceHeight = 0;
		}
		def.format.video.nStride = ( def.format.video.nFrameWidth + def.nBufferAlignment - 1 ) & ( ~(def.nBufferAlignment - 1) );
		def.format.video.nStride = 0;
		SetParameter( OMX_IndexParamPortDefinition, &def );

		GetParameter( OMX_IndexParamPortDefinition, &def );
		def.nPortIndex = 71;
		if ( outputPorts()[71].bDisableProprietary ) {
			def.format.video.nSliceHeight = mHeight;
		} else {
			def.format.video.nSliceHeight = 0;
		}
		SetParameter( OMX_IndexParamPortDefinition, &def );
	}

	if ( port == 72 ) {
		def.nPortIndex = 72;
		GetParameter( OMX_IndexParamPortDefinition, &def );
		def.format.image.nFrameWidth = mWidth;
		def.format.image.nFrameHeight = mHeight;
		printf( "===============> outputPorts()[%d].bDisableProprietary : %d\n", port, outputPorts()[port].bDisableProprietary );
		if ( outputPorts()[port].bDisableProprietary ) {
			def.format.image.nSliceHeight = mHeight;
		} else {
			def.format.image.nSliceHeight = 0;
		}
		def.format.image.nStride = ( def.format.image.nFrameWidth + def.nBufferAlignment - 1 ) & ( ~(def.nBufferAlignment - 1) );
		def.format.image.nStride = 0;
		SetParameter( OMX_IndexParamPortDefinition, &def );
	}

	return OMX_ErrorNone;
}


OMX_ERRORTYPE Camera::setSensorMode( uint8_t sensor_mode )
{
	OMX_PARAM_U32TYPE sensorMode;
	OMX_INIT_STRUCTURE( sensorMode );
	sensorMode.nPortIndex = OMX_ALL;

	sensorMode.nU32 = sensor_mode;
	OMX_ERRORTYPE err = SetParameter( OMX_IndexParamCameraCustomSensorConfig, &sensorMode );
	if ( err == OMX_ErrorNone ) {
		mSensorMode = sensorMode.nU32;
	}

	return err;
}


int Camera::HighSpeedMode( uint32_t sensor_mode )
{
	OMX_PRIORITYMGMTTYPE prio;
	OMX_INIT_STRUCTURE( prio );
	GetParameter( OMX_IndexParamPriorityMgmt, &prio );
	prio.nGroupPriority = 99;
	SetParameter( OMX_IndexParamPriorityMgmt, &prio );
	GetParameter( OMX_IndexParamPriorityMgmt, &prio );

	setFrameStabilisation( false );

	/*
	- 0 - auto
	- 1 - 1080P30 cropped (680 pixels off left/right, 692 pixels off top/bottom), up to 30fps
	- 2 - 3240x2464 Full 4:3, up to 15fps (3280)
	- 3 - 3240x2464 Full 4:3, up to 15fps (identical to 2)
	- 4 - 1640x1232 binned 4:3, up 40fps
	- 5 - 1640x922 2x2 binned 16:9 (310 pixels off top/bottom before binning), up to 40fps
	- 6 - 720P binned and cropped (360 pixels off left/right, 512 pixels off top/bottom before binning), 40 to 90fps (120fps if overclocked)
	- 7 - VGA binned and cropped (1000 pixels off left/right, 752 pixels off top/bottom before binning), 40 to 90fps (120fps if overclocked)
	*/
	OMX_PARAM_U32TYPE sensorMode;
	OMX_INIT_STRUCTURE( sensorMode );
	sensorMode.nPortIndex = OMX_ALL;
	if ( sensor_mode != 0 ) {
		sensorMode.nU32 = sensor_mode;
	} else {
		if ( mWidth <= 640 && mHeight <= 480 ) {
			sensorMode.nU32 = 7;
		} else if ( mWidth <= 1280 && mHeight <= 720 ) {
			sensorMode.nU32 = 6;
		} else if ( mWidth == 1920 && mHeight == 1080 ) {
			sensorMode.nU32 = 1;
		} else if ( mWidth <= 1640 && mHeight <= 922 ) {
			sensorMode.nU32 = 5;
		} else if ( mWidth <= 1640 && mHeight <= 1232 ) {
			sensorMode.nU32 = 4;
		} else if ( mWidth <= 3240 && mHeight <= 2464 ) {
			sensorMode.nU32 = 2;
		} else {
			sensorMode.nU32 = 0;
		}
	}

	OMX_ERRORTYPE err = SetParameter( OMX_IndexParamCameraCustomSensorConfig, &sensorMode );
	if ( err == OMX_ErrorNone ) {
		mSensorMode = sensorMode.nU32;
	}

	OMX_CONFIG_ZEROSHUTTERLAGTYPE zero_shutter;
	OMX_INIT_STRUCTURE( zero_shutter );
	zero_shutter.bZeroShutterMode = OMX_TRUE;
	zero_shutter.bConcurrentCapture = OMX_TRUE;
	SetParameter( OMX_IndexParamCameraZeroShutterLag, &zero_shutter );


	OMX_CONFIG_CAPTUREMODETYPE capture_mode;
	OMX_INIT_STRUCTURE( capture_mode );
	capture_mode.nPortIndex = 71;
	capture_mode.bContinuous = OMX_TRUE;
	SetConfig( OMX_IndexConfigCaptureMode, &capture_mode );

	OMX_PARAM_CAMERAIMAGEPOOLTYPE pool;
	OMX_INIT_STRUCTURE( pool );
	GetParameter( OMX_IndexParamCameraImagePool, &pool );
// 	pool.nNumHiResVideoFrames = 2;
// 	pool.nHiResVideoWidth = mWidth;
// 	pool.nHiResVideoHeight = mHeight;
// 	pool.eHiResVideoType = OMX_COLOR_FormatYUV420PackedPlanar;
	pool.nNumHiResStillsFrames = 2;
	pool.nHiResStillsWidth = 3240; // TODO : get resolution of port 72
	pool.nHiResStillsHeight = 2464;
	pool.eHiResStillsType = OMX_COLOR_FormatYUV420PackedPlanar;
// 	pool.nNumLoResFrames = 2;
// 	pool.nLoResWidth = mWidth;
// 	pool.nLoResHeight = mHeight;
// 	pool.eLoResType = OMX_COLOR_FormatYUV420PackedPlanar;
// 	pool.nNumSnapshotFrames = 2;
// 	pool.eSnapshotType = OMX_COLOR_FormatYUV420PackedPlanar;
	pool.eInputPoolMode = OMX_CAMERAIMAGEPOOLINPUTMODE_TWOPOOLS;
// 	pool.nNumInputVideoFrames = 2;
// 	pool.nInputVideoWidth = mWidth;
// 	pool.nInputVideoHeight = mHeight;
// 	pool.eInputVideoType = OMX_COLOR_FormatYUV420PackedPlanar;
	pool.nNumInputStillsFrames = 2;
	pool.nInputStillsWidth = 3240;
	pool.nInputStillsHeight = 2464;
	pool.eInputStillsType = OMX_COLOR_FormatYUV420PackedPlanar;
// 	if ( SetParameter( OMX_IndexParamCameraImagePool, &pool ) != OMX_ErrorNone ) {
// 		exit(0);
// 	}

	OMX_PARAM_CAMERACAPTUREMODETYPE captureMode;
	OMX_INIT_STRUCTURE( captureMode );
	captureMode.nPortIndex = OMX_ALL;
	captureMode.eMode = OMX_CameraCaptureModeResumeViewfinderImmediately;
	SetParameter( OMX_IndexParamCameraCaptureMode, &captureMode );

	OMX_PARAM_CAMERADISABLEALGORITHMTYPE disableAlgorithm;
	OMX_INIT_STRUCTURE( disableAlgorithm );
	disableAlgorithm.bDisabled = OMX_TRUE;
	std::list<OMX_CAMERADISABLEALGORITHMTYPE> algo_disable = {
		OMX_CameraDisableAlgorithmFacetracking,
		OMX_CameraDisableAlgorithmRedEyeReduction,
// 		OMX_CameraDisableAlgorithmVideoStabilisation,
		OMX_CameraDisableAlgorithmWriteRaw,
// 		OMX_CameraDisableAlgorithmVideoDenoise,
// 		OMX_CameraDisableAlgorithmStillsDenoise,
// 		OMX_CameraDisableAlgorithmAntiShake,
// 		OMX_CameraDisableAlgorithmImageEffects,
// 		OMX_CameraDisableAlgorithmDarkSubtract,
// 		OMX_CameraDisableAlgorithmDynamicRangeExpansion,
		OMX_CameraDisableAlgorithmFaceRecognition,
		OMX_CameraDisableAlgorithmFaceBeautification,
		OMX_CameraDisableAlgorithmSceneDetection,
// 		OMX_CameraDisableAlgorithmHighDynamicRange,
	};
	for ( OMX_CAMERADISABLEALGORITHMTYPE algo : algo_disable ) {
		disableAlgorithm.eAlgorithm = algo;
		SetParameter( OMX_IndexParamCameraDisableAlgorithm, &disableAlgorithm );
	}
/*
	// ALL UNSUPPORTED
	OMX_CONFIG_BOOLEANTYPE colour_denoise;
	OMX_INIT_STRUCTURE( colour_denoise );
	colour_denoise.bEnabled = OMX_FALSE;
	SetConfig( OMX_IndexConfigStillColourDenoiseEnable, &colour_denoise );
	SetConfig( OMX_IndexConfigVideoColourDenoiseEnable, &colour_denoise );

	OMX_CONFIG_BOOLEANTYPE async;
	OMX_INIT_STRUCTURE( async );
	async.bEnabled = OMX_TRUE;
	SetParameter( OMX_IndexParamAsynchronousOutput, &async );

	OMX_CONFIG_BOOLEANTYPE pre_post_process;
	OMX_INIT_STRUCTURE( pre_post_process );
	pre_post_process.bEnabled = OMX_FALSE;
	SetConfig( OMX_IndexConfigBrcmCameraRnDPreprocess, &pre_post_process );
	SetConfig( OMX_IndexConfigBrcmCameraRnDPostprocess, &pre_post_process );
*/
/*
	OMX_CONFIG_BOOLEANTYPE sharpen_disable;
	OMX_INIT_STRUCTURE( sharpen_disable );
	sharpen_disable.bEnabled = OMX_TRUE;
	SetParameter( OMX_IndexParamSWSharpenDisable, &sharpen_disable );
	SetParameter( OMX_IndexParamSWSaturationDisable, &sharpen_disable );
*/

	return 0;
}


OMX_ERRORTYPE Camera::disableLensShading()
{
	OMX_PARAM_U32TYPE isp_override;
	OMX_INIT_STRUCTURE( isp_override );
	isp_override.nPortIndex = OMX_ALL;
	isp_override.nU32 = ~0x08;

	OMX_ERRORTYPE ret = SetParameter( OMX_IndexParamBrcmIspBlockOverride, &isp_override );
	if ( ret != OMX_ErrorNone ) {
		mDebugCallback( 0, "Cannot disable lens shading : 0x%08X\n", ret ); fflush(stderr);
	}

	return ret;
}


OMX_ERRORTYPE Camera::setLensShadingGrid( uint32_t grid_cell_size, uint32_t grid_width, uint32_t grid_height, uint32_t transform, const uint8_t* ls_grid )
{
	OMX_PARAM_LENSSHADINGOVERRIDETYPE lens_override;
	OMX_INIT_STRUCTURE( lens_override );
/*
	if ( mLensShadingAlloc != 0 ) {
		OMX_ERRORTYPE ret = GetParameter( OMX_IndexParamBrcmLensShadingOverride, &lens_override );
		if ( ret != OMX_ErrorNone ) {
			mDebugCallback( 0, "Cannot get lens shading override : 0x%08X\n", ret ); fflush(stderr);
			return ret;
		}
		lens_override.bEnabled = OMX_FALSE;
		ret = SetParameter( OMX_IndexParamBrcmLensShadingOverride, &lens_override );
		if ( ret != OMX_ErrorNone ) {
			mDebugCallback( 0, "Cannot disable lens shading override : 0x%08X\n", ret ); fflush(stderr);
			return ret;
		}
		vcsm_free( mLensShadingAlloc );
	}
*/
	if ( not mVCSMReady ) {
		mVCSMReady = true;
		vcsm_init();
	}

	lens_override.bEnabled = OMX_TRUE;
	lens_override.nGridCellSize = grid_cell_size;
	lens_override.nWidth = grid_width;
	lens_override.nStride = grid_width;
	lens_override.nHeight = grid_height;
	lens_override.nRefTransform = transform;

	if ( mLensShadingAlloc == 0 ) {
		mLensShadingAlloc = vcsm_malloc( lens_override.nStride * lens_override.nHeight * 4, (char*)"ls_grid" );
		if ( mVerbose ) mDebugCallback( 1, "mLensShadingAlloc : %d (%d)\n", mLensShadingAlloc, lens_override.nStride * lens_override.nHeight * 4 );
		lens_override.nMemHandleTable = vcsm_vc_hdl_from_hdl( mLensShadingAlloc );
		if ( mVerbose ) mDebugCallback( 1, "lens_override.nMemHandleTable : %d\n", lens_override.nMemHandleTable );
	}
	void* grid = vcsm_lock( mLensShadingAlloc );
	if ( mVerbose ) mDebugCallback( 1, "grid : %p\n", grid );
	memcpy( grid, ls_grid, lens_override.nStride * lens_override.nHeight * 4 );
	vcsm_unlock_hdl( mLensShadingAlloc );

	OMX_ERRORTYPE ret = SetParameter( OMX_IndexParamBrcmLensShadingOverride, &lens_override );
	if ( ret != OMX_ErrorNone ) {
		mDebugCallback( 0, "Cannot set lens shading : 0x%08X\n", ret ); fflush(stderr);
	}

	return ret;
}


const uint8_t Camera::sensorMode()
{
	return mSensorMode;
}


const uint32_t Camera::framerate()
{
	OMX_CONFIG_FRAMERATETYPE framerate;
	OMX_INIT_STRUCTURE( framerate );
	framerate.nPortIndex = 70;
	GetConfig( OMX_IndexConfigVideoFramerate, &framerate );
	uint32_t rate = framerate.xEncodeFramerate >> 16;

	OMX_PARAM_U32TYPE sensorMode;
	OMX_INIT_STRUCTURE( sensorMode );
	sensorMode.nPortIndex = OMX_ALL;
	GetParameter( OMX_IndexParamCameraCustomSensorConfig, &sensorMode );
	if ( sensorMode.nU32 == 5 or sensorMode.nU32 == 4 ) {
		rate = std::min( rate, 40u );
	}
	if ( sensorMode.nU32 == 3 or sensorMode.nU32 == 2 ) {
		rate = std::min( rate, 15u );
	}
	if ( sensorMode.nU32 == 1 ) {
		rate = std::min( rate, 30u );
	}

	return rate;
}


const int32_t Camera::width( uint8_t port )
{
	OMX_PARAM_PORTDEFINITIONTYPE def;
	OMX_INIT_STRUCTURE( def );
	def.nPortIndex = port;
	GetParameter( OMX_IndexParamPortDefinition, &def );
	return ( port == 72 ) ? def.format.image.nFrameWidth : def.format.video.nFrameWidth;
}


const int32_t Camera::height( uint8_t port )
{
	OMX_PARAM_PORTDEFINITIONTYPE def;
	OMX_INIT_STRUCTURE( def );
	def.nPortIndex = port;
	GetParameter( OMX_IndexParamPortDefinition, &def );
	return ( port == 72 ) ? def.format.image.nFrameHeight : def.format.video.nFrameHeight;
}


const uint32_t Camera::brightness()
{
	OMX_CONFIG_BRIGHTNESSTYPE brightness;
	OMX_INIT_STRUCTURE( brightness );
	brightness.nPortIndex = OMX_ALL;
	GetConfig( OMX_IndexConfigCommonBrightness, &brightness );
	return brightness.nBrightness;
}


const int32_t Camera::contrast()
{
	OMX_CONFIG_CONTRASTTYPE contrast;
	OMX_INIT_STRUCTURE( contrast );
	contrast.nPortIndex = OMX_ALL;
	GetConfig( OMX_IndexConfigCommonContrast, &contrast );
	return contrast.nContrast;
}


const int32_t Camera::saturation()
{
	OMX_CONFIG_SATURATIONTYPE saturation;
	OMX_INIT_STRUCTURE( saturation );
	saturation.nPortIndex = OMX_ALL;
	GetConfig( OMX_IndexConfigCommonSaturation, &saturation );
	return saturation.nSaturation;
}


OMX_ERRORTYPE Camera::lastSettings( Camera::Settings* settings )
{

	OMX_CONFIG_CAMERASETTINGSTYPE omx_settings;
	OMX_INIT_STRUCTURE( omx_settings );
	omx_settings.nPortIndex = 71;
	OMX_ERRORTYPE err = GetConfig( OMX_IndexConfigCameraSettings, &omx_settings );
	if ( err == OMX_ErrorNone ) {
		settings->nExposure = omx_settings.nExposure;
		settings->nAnalogGain = omx_settings.nAnalogGain;
		settings->nDigitalGain = omx_settings.nDigitalGain;
		settings->nLux = omx_settings.nLux;
		settings->nRedGain = omx_settings.nRedGain;
		settings->nBlueGain = omx_settings.nBlueGain;
		settings->nFocusPosition = omx_settings.nFocusPosition;
	}
	return err;
}


OMX_ERRORTYPE Camera::setFramerate( uint32_t fps )
{
	OMX_CONFIG_FRAMERATETYPE framerate;
	OMX_INIT_STRUCTURE( framerate );
	framerate.xEncodeFramerate = fps << 16;
	framerate.nPortIndex = 70;
	OMX_ERRORTYPE err = SetConfig( OMX_IndexConfigVideoFramerate, &framerate );
	if ( err != OMX_ErrorNone ) {
		return err;
	}
	framerate.nPortIndex = 71;
	return SetConfig( OMX_IndexConfigVideoFramerate, &framerate );
}


OMX_ERRORTYPE Camera::setBrightness( uint32_t value )
{
	OMX_CONFIG_BRIGHTNESSTYPE brightness;
	OMX_INIT_STRUCTURE( brightness );
	brightness.nPortIndex = OMX_ALL;
	brightness.nBrightness = value;
	return SetConfig( OMX_IndexConfigCommonBrightness, &brightness );
}


OMX_ERRORTYPE Camera::setSharpness( uint32_t value )
{
	OMX_CONFIG_SHARPNESSTYPE sharpness;
	OMX_INIT_STRUCTURE( sharpness );
	sharpness.nPortIndex = OMX_ALL;
	sharpness.nSharpness = value;
	return SetConfig( OMX_IndexConfigCommonSharpness, &sharpness );
}


OMX_ERRORTYPE Camera::setSaturation( int32_t value )
{
	OMX_CONFIG_SATURATIONTYPE saturation;
	OMX_INIT_STRUCTURE( saturation );
	saturation.nPortIndex = OMX_ALL;
	saturation.nSaturation = value;
	return SetConfig( OMX_IndexConfigCommonSaturation, &saturation );
}


OMX_ERRORTYPE Camera::setContrast( int32_t value )
{
	OMX_CONFIG_CONTRASTTYPE contrast;
	OMX_INIT_STRUCTURE( contrast );
	contrast.nPortIndex = OMX_ALL;
	contrast.nContrast = value;
	return SetConfig( OMX_IndexConfigCommonContrast, &contrast );
}


OMX_ERRORTYPE Camera::setWhiteBalanceControl( WhiteBalControl value )
{
	OMX_CONFIG_WHITEBALCONTROLTYPE white_balance_control;
	OMX_INIT_STRUCTURE( white_balance_control );
	white_balance_control.nPortIndex = OMX_ALL;
	white_balance_control.eWhiteBalControl = (OMX_WHITEBALCONTROLTYPE)value;
	return SetConfig( OMX_IndexConfigCommonWhiteBalance, &white_balance_control );
}


OMX_ERRORTYPE Camera::setExposureControl( ExposureControl value )
{
	OMX_CONFIG_EXPOSURECONTROLTYPE exposure;
	OMX_INIT_STRUCTURE( exposure );
	exposure.nPortIndex = OMX_ALL;
	exposure.eExposureControl = (OMX_EXPOSURECONTROLTYPE)value;
	return SetConfig( OMX_IndexConfigCommonExposure, &exposure );
}


OMX_ERRORTYPE Camera::setExposureValue( ExposureMeteringMode metering, uint16_t exposure_compensation, uint32_t iso_sensitivity, uint32_t shutter_speed_us )
{
	OMX_CONFIG_EXPOSUREVALUETYPE exposure_value;
	OMX_INIT_STRUCTURE( exposure_value );
	exposure_value.nPortIndex = OMX_ALL;
	exposure_value.eMetering = (OMX_METERINGTYPE)metering;
	exposure_value.xEVCompensation = exposure_compensation << 16;
	exposure_value.nApertureFNumber = (uint32_t)( 2.8f * 655536.0f );
	exposure_value.bAutoSensitivity = (OMX_BOOL)( iso_sensitivity == 0 );
	exposure_value.nSensitivity = iso_sensitivity;
	exposure_value.bAutoShutterSpeed = (OMX_BOOL)( shutter_speed_us == 0 );
	exposure_value.nShutterSpeedMsec = shutter_speed_us;
	return SetConfig( OMX_IndexConfigCommonExposureValue, &exposure_value );
}


OMX_ERRORTYPE Camera::setImageFilter( ImageFilter filter )
{
	OMX_CONFIG_IMAGEFILTERTYPE image_filter;
	OMX_INIT_STRUCTURE( image_filter );
	image_filter.nPortIndex = OMX_ALL;
	image_filter.eImageFilter = (OMX_IMAGEFILTERTYPE)filter;
	return SetConfig( OMX_IndexConfigCommonImageFilter, &image_filter );
}


OMX_ERRORTYPE Camera::setFrameStabilisation( bool enabled )
{
	OMX_CONFIG_FRAMESTABTYPE frame_stabilisation_control;
	OMX_INIT_STRUCTURE(frame_stabilisation_control);
	frame_stabilisation_control.nPortIndex = OMX_ALL;
	frame_stabilisation_control.bStab = (OMX_BOOL)enabled;
	return SetConfig( OMX_IndexConfigCommonFrameStabilisation, &frame_stabilisation_control );
}


OMX_ERRORTYPE Camera::setMirror( bool hrzn, bool vert )
{
	OMX_MIRRORTYPE eMirror = OMX_MirrorNone;
	if ( hrzn and not vert ) {
		eMirror = OMX_MirrorHorizontal;
	} else if ( vert and not hrzn ) {
		eMirror = OMX_MirrorVertical;
	} else if ( hrzn and vert ) {
		eMirror = OMX_MirrorBoth;
	}

	OMX_CONFIG_MIRRORTYPE mirror;
	OMX_INIT_STRUCTURE(mirror);
	mirror.nPortIndex = 70;
	mirror.eMirror = eMirror;
	OMX_ERRORTYPE err = SetConfig( OMX_IndexConfigCommonMirror, &mirror );
	if ( err != OMX_ErrorNone ) {
		return err;
	}

	mirror.nPortIndex = 71;
	return SetConfig( OMX_IndexConfigCommonMirror, &mirror );
}


OMX_ERRORTYPE Camera::setRotation( int32_t angle_degrees )
{
	OMX_CONFIG_ROTATIONTYPE rot;
	OMX_INIT_STRUCTURE(rot);
	rot.nPortIndex = 70;
	rot.nRotation = angle_degrees;
	OMX_ERRORTYPE err = SetConfig( OMX_IndexConfigCommonRotate, &rot );
	if ( err != OMX_ErrorNone ) {
		return err;
	}

	rot.nPortIndex = 71;
	return SetConfig( OMX_IndexConfigCommonRotate, &rot );
}


OMX_ERRORTYPE Camera::setInputCrop( float left, float top, float width, float height )
{
	OMX_CONFIG_INPUTCROPTYPE crop;
	OMX_INIT_STRUCTURE(crop);
	crop.nPortIndex = OMX_ALL;
	crop.xLeft = ( ((uint32_t)left) << 16 ) | ( (uint32_t)( ( left - std::floor(left) ) * 65535.0f ) );
	crop.xTop = ( ((uint32_t)top) << 16 ) | ( (uint32_t)( ( top - std::floor(top) ) * 65535.0f ) );
	crop.xWidth = ( ((uint32_t)width) << 16 ) | ( (uint32_t)( ( width - std::floor(width) ) * 65535.0f ) );
	crop.xHeight = ( ((uint32_t)height) << 16 ) | ( (uint32_t)( ( height - std::floor(height) ) * 65535.0f ) );
	return SetConfig( OMX_IndexConfigInputCropPercentages, &crop );
}
