#include <unistd.h>
#include "Camera.h"
#include <IL/OMX_Video.h>
#include <IL/OMX_Types.h>
#include <IL/OMX_Component.h>
#include <IL/OMX_Broadcom.h>

using namespace IL;

Camera::Camera( uint32_t width, uint32_t height, uint32_t device_number, bool high_speed, bool verbose )
	: Component( "OMX.broadcom.camera", { PortInit( 73, Clock ) }, { PortInit( 70, Video ), PortInit( 71, Video ), PortInit( 72, Image ) }, verbose )
	, mDeviceNumber( device_number )
	, mWidth( width )
	, mHeight( height )
{
	if ( not Component::mHandle ) {
		return;
	}

	Initialize( width, height );
	if ( high_speed ) {
		HighSpeedMode();
	}

	while ( not mReady ) {
		usleep( 100 * 1000 );
	}
	if ( mVerbose ) {
		fprintf( stderr, "Camera %d ready\n", mDeviceNumber );
	}

	SendCommand( OMX_CommandStateSet, OMX_StateIdle, nullptr );
}


Camera::~Camera()
{
}


OMX_ERRORTYPE Camera::SetState( const Component::State& st )
{
	OMX_ERRORTYPE err = IL::Component::SetState(st);
	if ( err != OMX_ErrorNone ) {
		return err;
	}
	OMX_CONFIG_PORTBOOLEANTYPE capture;
	OMX_INIT_STRUCTURE( capture );
	capture.nPortIndex = 71;
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


int Camera::Initialize( uint32_t width, uint32_t height )
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

	// Set port definition
	OMX_PARAM_PORTDEFINITIONTYPE def;
	OMX_INIT_STRUCTURE( def );
	def.nPortIndex = 70;
	GetParameter( OMX_IndexParamPortDefinition, &def );
	def.format.video.nFrameWidth  = width;
	def.format.video.nFrameHeight = height;
	def.format.video.xFramerate   = 30 << 16;
	if ( width < 1920 and height < 1080 ) {
		def.format.video.xFramerate   = 60 << 16; // default to 60 FPS
	}
	def.format.video.nStride      = ( def.format.video.nFrameWidth + def.nBufferAlignment - 1 ) & ( ~(def.nBufferAlignment - 1) );
	def.format.video.eColorFormat = OMX_COLOR_FormatYUV420PackedPlanar;
	SetParameter( OMX_IndexParamPortDefinition, &def );

	// Also set it for port 71 (video port)
	GetParameter( OMX_IndexParamPortDefinition, &def );
	def.nPortIndex = 71;
	SetParameter( OMX_IndexParamPortDefinition, &def );

	setBrightness( 50 );
	setContrast( 0 );
	setSaturation( 0 );
	setSharpness( 0 );
	setWhiteBalanceControl( WhiteBalControlAuto );
	setExposureControl( ExposureControlAuto );
	setExposureValue( 0, 2.8f, 0, 0 );
	setFrameStabilisation( true );

	return 0;
}


int Camera::HighSpeedMode()
{
	OMX_PRIORITYMGMTTYPE prio;
	OMX_INIT_STRUCTURE( prio );
	GetParameter( OMX_IndexParamPriorityMgmt, &prio );
	printf( "PRIORITY : %d, %d\n", prio.nGroupPriority, prio.nGroupID );
	prio.nGroupPriority = 99;
	SetParameter( OMX_IndexParamPriorityMgmt, &prio );
	GetParameter( OMX_IndexParamPriorityMgmt, &prio );
	printf( "PRIORITY : %d, %d\n", prio.nGroupPriority, prio.nGroupID );

	setFrameStabilisation( false );

	/*
	- 0 - auto
	- 1 - 1080P30 cropped (680 pixels off left/right, 692 pixels off top/bottom), up to 30fps
	- 2 - 3240x2464 Full 4:3, up to 15fps
	- 3 - 3240x2464 Full 4:3, up to 15fps (identical to 2)
	- 4 - 1640x1232 binned 4:3, up 40fps
	- 5 - 1640x922 2x2 binned 16:9 (310 pixels off top/bottom before binning), up to 40fps
	- 6 - 720P binned and cropped (360 pixels off left/right, 512 pixels off top/bottom before binning), 40 to 90fps (120fps if overclocked)
	- 7 - VGA binned and cropped (1000 pixels off left/right, 752 pixels off top/bottom before binning), 40 to 90fps (120fps if overclocked)
	*/
	OMX_PARAM_U32TYPE sensorMode;
	OMX_INIT_STRUCTURE( sensorMode );
	sensorMode.nPortIndex = OMX_ALL;
	if ( mWidth <= 640 && mHeight <= 480 ) {
		sensorMode.nU32 = 7;
	} else if ( mWidth <= 1280 && mHeight <= 720 ) {
		sensorMode.nU32 = 6;
	} else if ( mWidth == 1920 && mHeight == 1080 ) {
		sensorMode.nU32 = 1;
	} else {
		sensorMode.nU32 = 0;
	}
	SetParameter( OMX_IndexParamCameraCustomSensorConfig, &sensorMode );

	OMX_CONFIG_ZEROSHUTTERLAGTYPE zero_shutter;
	OMX_INIT_STRUCTURE( zero_shutter );
	zero_shutter.bZeroShutterMode = 0;
	zero_shutter.bConcurrentCapture = 1;
	SetParameter( OMX_IndexParamCameraZeroShutterLag, &zero_shutter );

	OMX_CONFIG_CAPTUREMODETYPE capture_mode;
	OMX_INIT_STRUCTURE( capture_mode );
	capture_mode.nPortIndex = 71;
	capture_mode.bContinuous = OMX_TRUE;
	SetConfig( OMX_IndexConfigCaptureMode, &capture_mode );
/*
	OMX_PARAM_CAMERAIMAGEPOOLTYPE pool;
	OMX_INIT_STRUCTURE( pool );
	GetParameter( OMX_IndexParamCameraImagePool, &pool );
// 	pool.nNumHiResVideoFrames = 1;
// 	pool.nHiResVideoWidth = WIDTH;
// 	pool.nHiResVideoHeight = HEIGHT;
	pool.eHiResVideoType = OMX_COLOR_FormatYUV420PackedPlanar;
// 	pool.nNumHiResStillsFrames = 1; <<==2
// 	pool.nHiResStillsWidth = WIDTH;
// 	pool.nHiResStillsHeight = HEIGHT;
	pool.eHiResStillsType = OMX_COLOR_FormatYUV420PackedPlanar;
// 	pool.nNumLoResFrames = 1;
// 	pool.nLoResWidth = WIDTH;
// 	pool.nLoResHeight = HEIGHT;
	pool.eLoResType = OMX_COLOR_FormatYUV420PackedPlanar;
// 	pool.nNumSnapshotFrames = 1;
	pool.eSnapshotType = OMX_COLOR_FormatYUV420PackedPlanar;
	pool.eInputPoolMode = OMX_CAMERAIMAGEPOOLINPUTMODE_TWOPOOLS;
// 	pool.nNumInputVideoFrames = 1;
// 	pool.nInputVideoWidth = WIDTH;
// 	pool.nInputVideoHeight = HEIGHT;
	pool.eInputVideoType = OMX_COLOR_FormatYUV420PackedPlanar;
// 	pool.nNumInputStillsFrames = 1; //// <== 0
// 	pool.nInputStillsWidth = WIDTH;
// 	pool.nInputStillsHeight = HEIGHT;
	pool.eInputStillsType = OMX_COLOR_FormatYUV420PackedPlanar;
	SetParameter( OMX_IndexParamCameraImagePool, &pool );

	OMX_PARAM_CAMERACAPTUREMODETYPE captureMode;
	OMX_INIT_STRUCTURE( captureMode );
	captureMode.nPortIndex = OMX_ALL;
	captureMode.eMode = OMX_CameraCaptureModeResumeViewfinderImmediately;
	SetParameter( OMX_IndexParamCameraCaptureMode, &captureMode );
*/
	OMX_PARAM_CAMERADISABLEALGORITHMTYPE disableAlgorithm;
	OMX_INIT_STRUCTURE( disableAlgorithm );
	disableAlgorithm.bDisabled = OMX_TRUE;
	disableAlgorithm.eAlgorithm = OMX_CameraDisableAlgorithmFacetracking;
	SetParameter( OMX_IndexParamCameraDisableAlgorithm, &disableAlgorithm );
	disableAlgorithm.eAlgorithm = OMX_CameraDisableAlgorithmRedEyeReduction;
	SetParameter( OMX_IndexParamCameraDisableAlgorithm, &disableAlgorithm );
	disableAlgorithm.eAlgorithm = OMX_CameraDisableAlgorithmStillsDenoise;
	SetParameter( OMX_IndexParamCameraDisableAlgorithm, &disableAlgorithm );
	disableAlgorithm.eAlgorithm = OMX_CameraDisableAlgorithmFaceRecognition;
	SetParameter( OMX_IndexParamCameraDisableAlgorithm, &disableAlgorithm );
	disableAlgorithm.eAlgorithm = OMX_CameraDisableAlgorithmFaceBeautification;
	SetParameter( OMX_IndexParamCameraDisableAlgorithm, &disableAlgorithm );
	disableAlgorithm.eAlgorithm = OMX_CameraDisableAlgorithmSceneDetection;
	SetParameter( OMX_IndexParamCameraDisableAlgorithm, &disableAlgorithm );
	disableAlgorithm.eAlgorithm = OMX_CameraDisableAlgorithmAntiShake;
	SetParameter( OMX_IndexParamCameraDisableAlgorithm, &disableAlgorithm );
	disableAlgorithm.eAlgorithm = OMX_CameraDisableAlgorithmVideoStabilisation;
	SetParameter( OMX_IndexParamCameraDisableAlgorithm, &disableAlgorithm );
	disableAlgorithm.eAlgorithm = OMX_CameraDisableAlgorithmVideoDenoise;
	SetParameter( OMX_IndexParamCameraDisableAlgorithm, &disableAlgorithm );
	disableAlgorithm.eAlgorithm = OMX_CameraDisableAlgorithmImageEffects;
	SetParameter( OMX_IndexParamCameraDisableAlgorithm, &disableAlgorithm );
	disableAlgorithm.eAlgorithm = OMX_CameraDisableAlgorithmDarkSubtract;
	SetParameter( OMX_IndexParamCameraDisableAlgorithm, &disableAlgorithm );
	disableAlgorithm.eAlgorithm = OMX_CameraDisableAlgorithmDynamicRangeExpansion;
	SetParameter( OMX_IndexParamCameraDisableAlgorithm, &disableAlgorithm );
	disableAlgorithm.eAlgorithm = OMX_CameraDisableAlgorithmHighDynamicRange;
	SetParameter( OMX_IndexParamCameraDisableAlgorithm, &disableAlgorithm );
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


OMX_ERRORTYPE Camera::setExposureValue( uint16_t exposure_compensation, float aperture, uint32_t iso_sensitivity, uint32_t shutter_speed_us )
{
	OMX_CONFIG_EXPOSUREVALUETYPE exposure_value;
	OMX_INIT_STRUCTURE( exposure_value );
	exposure_value.nPortIndex = OMX_ALL;
	exposure_value.xEVCompensation = exposure_compensation << 16;
	exposure_value.nApertureFNumber = (uint32_t)( aperture * 655536.0f );
	exposure_value.bAutoSensitivity = (OMX_BOOL)( iso_sensitivity == 0 );
	exposure_value.nSensitivity = iso_sensitivity;
	exposure_value.bAutoShutterSpeed = (OMX_BOOL)( shutter_speed_us == 0 );
	exposure_value.nShutterSpeedMsec = shutter_speed_us;
	return SetConfig( OMX_IndexConfigCommonExposureValue, &exposure_value );
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
