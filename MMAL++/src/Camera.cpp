#include <unistd.h>
#include "MMAL++/Camera.h"

using namespace MMAL;

static void camera_control_callback( MMAL_PORT_T* port, MMAL_BUFFER_HEADER_T* buffer )
{
	if (buffer->cmd == MMAL_EVENT_PARAMETER_CHANGED) {
		MMAL_EVENT_PARAMETER_CHANGED_T *param = (MMAL_EVENT_PARAMETER_CHANGED_T *)buffer->data;
		switch (param->hdr.id) {
			case MMAL_PARAMETER_CAMERA_SETTINGS:
			{
				MMAL_PARAMETER_CAMERA_SETTINGS_T *settings = (MMAL_PARAMETER_CAMERA_SETTINGS_T*)param;
				vcos_log_error("Exposure now %u, analog gain %u/%u, digital gain %u/%u", settings->exposure, settings->analog_gain.num, settings->analog_gain.den, settings->digital_gain.num, settings->digital_gain.den);
				vcos_log_error("AWB R=%u/%u, B=%u/%u", settings->awb_red_gain.num, settings->awb_red_gain.den, settings->awb_blue_gain.num, settings->awb_blue_gain.den);
			}
			break;
		}
	} else if (buffer->cmd == MMAL_EVENT_ERROR) {
		vcos_log_error("No data received from sensor. Check all connections, including the Sunny one on the camera board");
	} else {
		vcos_log_error("Received unexpected camera control callback event, 0x%08x", buffer->cmd);
	}

	mmal_buffer_header_release(buffer);
}


Camera::Camera( uint32_t width, uint32_t height, uint32_t device_number, bool high_speed, bool verbose )
	: Component( "vc.ril.camera", std::vector<uint8_t>(), { 0, 1, 2 }, verbose )
	, mDeviceNumber( device_number )
	, mWidth( width )
	, mHeight( height )
{
	Initialize( width, height );
	if ( high_speed ) {
		HighSpeedMode();
	}

	if ( mVerbose ) {
		fprintf( stderr, "Camera %d ready\n", mDeviceNumber );
	}
}


Camera::~Camera()
{
}


int Camera::SetState( const Component::State& st )
{
	MMAL::Component::SetState(st);
	return mmal_port_parameter_set_boolean( mHandle->output[1], MMAL_PARAMETER_CAPTURE, 1 );
}


int Camera::SetupTunnelPreview( Component* next, uint8_t port_input )
{
	return SetupTunnel( 0, next, port_input );
}


int Camera::SetupTunnelVideo( Component* next, uint8_t port_input )
{
	return SetupTunnel( 1, next, port_input );
}


int Camera::SetupTunnelImage( Component* next, uint8_t port_input )
{
	return SetupTunnel( 2, next, port_input );
}


int Camera::Initialize( uint32_t width, uint32_t height )
{
	MMAL_STATUS_T status;

	// Setup basic config
	MMAL_PARAMETER_INT32_T camera_num = { { MMAL_PARAMETER_CAMERA_NUM, sizeof(camera_num)}, (int32_t)mDeviceNumber };
	if ( ( status = mmal_port_parameter_set( mHandle->control, &camera_num.hdr ) ) < 0 ) {
		return status;
	}
	// Set sensor mode
	/* 0 - auto
	 - 1 - 1080P30 cropped (680 pixels off left/right, 692 pixels off top/bottom), up to 30fps
	 - 2 - 3240x2464 Full 4:3, up to 15fps
	 - 3 - 3240x2464 Full 4:3, up to 15fps (identical to 2)
	 - 4 - 1640x1232 binned 4:3, up 40fps
	 - 5 - 1640x922 2x2 binned 16:9 (310 pixels off top/bottom before binning), up to 40fps
	 - 6 - 720P binned and cropped (360 pixels off left/right, 512 pixels off top/bottom before binning), 40 to 90fps (120fps if overclocked)
	 - 7 - VGA binned and cropped (1000 pixels off left/right, 752 pixels off top/bottom before binning), 40 to 90fps (120fps if overclocked)
	*/
	uint32_t sensorMode;
	if ( mWidth <= 640 && mHeight <= 480 ) {
		sensorMode = 7;
	} else if ( mWidth <= 1280 && mHeight <= 720 ) {
		sensorMode = 6;
	} else if ( mWidth == 1920 && mHeight == 1080 ) {
		sensorMode = 1;
	} else {
		sensorMode = 0;
	}
	if ( ( status = mmal_port_parameter_set_uint32( mHandle->control, MMAL_PARAMETER_CAMERA_CUSTOM_SENSOR_CONFIG, sensorMode ) ) < 0 ) {
		return status;
	}
	// Enable control port
	if ( ( status = mmal_port_enable( mHandle->control, camera_control_callback ) ) < 0 ) {
		return status;
	}

	// Setup capture config
	MMAL_PARAMETER_CAMERA_CONFIG_T cam_config =
	{
		{ MMAL_PARAMETER_CAMERA_CONFIG, sizeof(cam_config) },
		.max_stills_w = width,
		.max_stills_h = height,
		.stills_yuv422 = 0,
		.one_shot_stills = 0,
		.max_preview_video_w = width,
		.max_preview_video_h = height,
		.num_preview_video_frames = 3 + vcos_max( 0, ( 60 - 30 ) / 10 ),
		.stills_capture_circular_buffer_height = 0,
		.fast_preview_resume = 1,
		.use_stc_timestamp = MMAL_PARAM_TIMESTAMP_MODE_RAW_STC
	};
	mmal_port_parameter_set( mHandle->control, &cam_config.hdr );

	// Setup output video port
	mHandle->output[1]->format->es->video.frame_rate.num = 30;
	mHandle->output[1]->format->es->video.frame_rate.den = 1;
	mHandle->output[1]->format->es->video.crop.width = width;
	mHandle->output[1]->format->es->video.crop.height = height;
	mHandle->output[1]->format->es->video.width = VCOS_ALIGN_UP(width, 32);
	mHandle->output[1]->format->es->video.height = VCOS_ALIGN_UP(height, 16);
	mHandle->output[1]->format->encoding = MMAL_ENCODING_OPAQUE;
	mHandle->output[1]->format->encoding_variant = MMAL_ENCODING_I420;
	if ( ( status = mmal_port_format_commit( mHandle->output[1] ) ) < 0 ) {
		return status;
	}

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
	return 0;
}


const uint32_t Camera::brightness()
{
	return 0;
}


const int32_t Camera::contrast()
{
	return 0;
}


const int32_t Camera::saturation()
{
	return 0;
}


int Camera::setFramerate( uint32_t fps )
{
	mHandle->output[1]->format->es->video.frame_rate.num = fps;
	mHandle->output[1]->format->es->video.frame_rate.den = 1;
	return mmal_port_format_commit( mHandle->output[1] );
}


int Camera::setBrightness( uint32_t value )
{
	return 0;
}


int Camera::setSharpness( uint32_t value )
{
	return 0;
}


int Camera::setSaturation( int32_t value )
{
	return 0;
}


int Camera::setContrast( int32_t value )
{
	return 0;
}


int Camera::setWhiteBalanceControl( WhiteBalControl value )
{
	return 0;
}


int Camera::setExposureControl( ExposureControl value )
{
	return 0;
}


int Camera::setExposureValue( uint16_t exposure_compensation, float aperture, uint32_t iso_sensitivity, uint32_t shutter_speed_us )
{
	return 0;
}


int Camera::setFrameStabilisation( bool enabled )
{
	return 0;
}


int Camera::setMirror( bool hrzn, bool vert )
{
	MMAL_PARAMETER_MIRROR_T mirror;
	mirror.hdr.id = MMAL_PARAMETER_MIRROR;
	mirror.hdr.size = sizeof(MMAL_PARAM_MIRROR_T);
	if ( hrzn and vert ) {
		mirror.value = MMAL_PARAM_MIRROR_BOTH;
	} else if ( hrzn ) {
		mirror.value = MMAL_PARAM_MIRROR_HORIZONTAL;
	} else if ( vert ) {
		mirror.value = MMAL_PARAM_MIRROR_VERTICAL;
	} else {
		mirror.value = MMAL_PARAM_MIRROR_NONE;
	}
	mmal_port_parameter_set( mHandle->output[0], &mirror.hdr );
	return mmal_port_parameter_set( mHandle->output[1], &mirror.hdr );
}
