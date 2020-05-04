#pragma once

#include "Component.h"

/*
   If you do not use the -ex option the shutter speed will never be longer than 1/8, with -ex night it will never be longer than 1/4 while with -ex verylong it will go to 1/2
*/


namespace IL {
class Camera : public IL::Component
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
	} ExposureControl;

	typedef enum {
		ExposureMeteringModeAverage,     // 25% spot - Center-weighted average metering.
		ExposureMeteringModeSpot,        // 10% spot - Spot (partial) metering.
		ExposureMeteringModeMatrix,      // Matrix or evaluative metering.
		ExposureMeteringKhronosExtensions = 0x6F000000, /**< Reserved region for introducing Khronos Standard Extensions */ 
		ExposureMeteringVendorStartUnused = 0x7F000000, /**< Reserved region for introducing Vendor Extensions */
		ExposureMeteringModeBacklit,     // 30% spot
	} ExposureMeteringMode;

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
	
	typedef struct Settings {
		OMX_U32 nExposure;
		OMX_U32 nAnalogGain;
		OMX_U32 nDigitalGain;
		OMX_U32 nLux;
		OMX_U32 nRedGain;
		OMX_U32 nBlueGain;
		OMX_U32 nFocusPosition;
	} Settings;


	Camera( uint32_t width, uint32_t height, uint32_t device_number = 0, bool high_speed = false, uint32_t sensor_mode = 0, bool verbose = false );
	~Camera();
	virtual OMX_ERRORTYPE SetState( const State& st, bool wait = true );
	OMX_ERRORTYPE SetCapturing( bool capturing, uint8_t port = 71 );
	OMX_ERRORTYPE SetupTunnelPreview( Component* next, uint8_t port_input = 0 );
	OMX_ERRORTYPE SetupTunnelVideo( Component* next, uint8_t port_input = 0 );
	OMX_ERRORTYPE SetupTunnelImage( Component* next, uint8_t port_input = 0 );

	OMX_ERRORTYPE setSensorMode( uint8_t mode );
	OMX_ERRORTYPE setResolution( uint32_t width, uint32_t height, uint8_t port = 71 );
	OMX_ERRORTYPE setFramerate( uint32_t fps );
	OMX_ERRORTYPE setBrightness( uint32_t value );
	OMX_ERRORTYPE setSharpness( uint32_t value );
	OMX_ERRORTYPE setSaturation( int32_t value );
	OMX_ERRORTYPE setContrast( int32_t value );
	OMX_ERRORTYPE setWhiteBalanceControl( WhiteBalControl value );
	OMX_ERRORTYPE setExposureControl( ExposureControl value );
	OMX_ERRORTYPE setExposureValue( ExposureMeteringMode metering = ExposureMeteringModeAverage, uint16_t exposure_compensation = 0, uint32_t iso_sensitivity = 0, uint32_t shutter_speed_us = 0 );
	OMX_ERRORTYPE setImageFilter( ImageFilter filter );
	OMX_ERRORTYPE setFrameStabilisation( bool enabled );
	OMX_ERRORTYPE setMirror( bool hrzn, bool vert );
	OMX_ERRORTYPE setRotation( int32_t angle_degrees );
	OMX_ERRORTYPE setInputCrop( float left, float top, float width, float height );

	OMX_ERRORTYPE disableLensShading();
	OMX_ERRORTYPE setLensShadingGrid( uint32_t grid_cell_size, uint32_t grid_width, uint32_t grid_height, uint32_t transform, const uint8_t* ls_grid );

	OMX_ERRORTYPE lastSettings( Settings* settings );

	const uint8_t sensorMode();
	const uint32_t framerate();
	const uint32_t brightness();
	const int32_t contrast();
	const int32_t saturation();
	const int32_t width( uint8_t port = 71 );
	const int32_t height( uint8_t port = 71 );

protected:
	virtual OMX_ERRORTYPE EventHandler( OMX_EVENTTYPE event, OMX_U32 data1, OMX_U32 data2, OMX_PTR eventdata );

private:
	int Initialize( uint32_t width, uint32_t height, uint32_t sensor_mode = 0 );
	int HighSpeedMode( uint32_t sensor_mode = 0 );

	uint32_t mDeviceNumber;
	uint32_t mWidth;
	uint32_t mHeight;
	uint8_t mSensorMode;
	bool mReady;
	bool mVCSMReady;
	uint32_t mLensShadingAlloc;
};

}; // namespace IL
