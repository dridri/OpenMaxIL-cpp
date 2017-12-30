#pragma once

#include "Component.h"

namespace MMAL {

class Camera : public MMAL::Component
{
public:
	typedef enum {
		WhiteBalControlOff = 0,
		WhiteBalControlAuto,
		WhiteBalControlSunLight,
		WhiteBalControlCloudy,
		WhiteBalControlShade,
		WhiteBalControlTungsten,
		WhiteBalControlFluorescent,
		WhiteBalControlIncandescent,
		WhiteBalControlFlash,
		WhiteBalControlHorizon,
	} WhiteBalControl;

	typedef enum {
		ExposureControlOff = 0,
		ExposureControlAuto,
		ExposureControlNight,
		ExposureControlBackLight,
		ExposureControlSpotLight,
		ExposureControlSports,
		ExposureControlSnow,
		ExposureControlBeach,
		ExposureControlLargeAperture,
		ExposureControlSmallAperture,
		ExposureControlKhronosExtensions = 0x6F000000, /**< Reserved region for introducing Khronos Standard Extensions */ 
		ExposureControlVendorStartUnused = 0x7F000000, /**< Reserved region for introducing Vendor Extensions */
		ExposureControlVeryLong,
		ExposureControlFixedFps,
		ExposureControlNightWithPreview,
		ExposureControlAntishake,
		ExposureControlFireworks,
		ExposureControlMax = 0x7FFFFFFF
	} ExposureControl;

	typedef enum {
		ImageFilterNone,
		ImageFilterNoise,
		ImageFilterEmboss,
		ImageFilterNegative,
		ImageFilterSketch,
		ImageFilterOilPaint,
		ImageFilterHatch,
		ImageFilterGpen,
		ImageFilterAntialias,
		ImageFilterDeRing,
		ImageFilterSolarize,
		ImageFilterKhronosExtensions = 0x6F000000, /**< Reserved region for introducing Khronos Standard Extensions */ 
		ImageFilterVendorStartUnused = 0x7F000000, /**< Reserved region for introducing Vendor Extensions */
		/* Broadcom specific image filters */
		ImageFilterWatercolor,
		ImageFilterPastel,
		ImageFilterSharpen,
		ImageFilterFilm,
		ImageFilterBlur,
		ImageFilterSaturation,
		ImageFilterDeInterlaceLineDouble,
		ImageFilterDeInterlaceAdvanced,
		ImageFilterColourSwap,
		ImageFilterWashedOut,
		ImageFilterColourPoint,
		ImageFilterPosterise,
		ImageFilterColourBalance,
		ImageFilterCartoon,
		ImageFilterAnaglyph,
		ImageFilterDeInterlaceFast,
		ImageFilterMax = 0x7FFFFFFF
	} ImageFilter;

	Camera( uint32_t width, uint32_t height, uint32_t device_number = 0, bool high_speed = false, uint32_t sensor_mode = 0, bool verbose = false );
	~Camera();
	virtual int SetState( const State& st );
	int SetCapturing( bool capturing, uint8_t port = 1 );
	int SetupTunnelPreview( Component* next, uint8_t port_input = 0 );
	int SetupTunnelVideo( Component* next, uint8_t port_input = 0 );
	int SetupTunnelImage( Component* next, uint8_t port_input = 0 );

	int setSensorMode( uint8_t mode );
	int setResolution( uint32_t width, uint32_t height, uint8_t port = 1 );
	int setFramerate( uint32_t fps );
	int setBrightness( uint32_t value );
	int setSharpness( uint32_t value );
	int setSaturation( int32_t value );
	int setContrast( int32_t value );
	int setWhiteBalanceControl( WhiteBalControl value );
	int setExposureControl( ExposureControl value );
	int setExposureValue( uint16_t exposure_compensation, float aperture, uint32_t iso_sensitivity, uint32_t shutter_speed_us );
	int setImageFilter( ImageFilter filter );
	int setFrameStabilisation( bool enabled );
	int setMirror( bool hrzn, bool vert );
	int setRotation( int32_t angle_degrees );

	int disableLensShading();
	int setLensShadingGrid( uint32_t grid_cell_size, uint32_t grid_width, uint32_t grid_height, uint32_t transform, const uint8_t* ls_grid );

	const uint8_t sensorMode();
	const uint32_t framerate();
	const uint32_t brightness();
	const int32_t contrast();
	const int32_t saturation();
	const int32_t width( uint8_t port = 1 );
	const int32_t height( uint8_t port = 1 );

protected:

private:
	int Initialize( uint32_t width, uint32_t height );
	int HighSpeedMode();

	uint32_t mDeviceNumber;
	uint32_t mWidth;
	uint32_t mHeight;
	uint8_t mSensorMode;
	bool mReady;
	bool mVCSMReady;
	uint32_t mLensShadingAlloc;
};

}; // namespace MMAL
