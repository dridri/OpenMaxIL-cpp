#pragma once

#include "Component.h"

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

	Camera( uint32_t width, uint32_t height, uint32_t device_number = 0, bool high_speed = false, bool verbose = false );
	~Camera();
	virtual OMX_ERRORTYPE SetState( const State& st );
	OMX_ERRORTYPE SetupTunnelPreview( Component* next, uint8_t port_input = 0 );
	OMX_ERRORTYPE SetupTunnelVideo( Component* next, uint8_t port_input = 0 );
	OMX_ERRORTYPE SetupTunnelImage( Component* next, uint8_t port_input = 0 );

	OMX_ERRORTYPE setFramerate( uint32_t fps );
	OMX_ERRORTYPE setBrightness( uint32_t value );
	OMX_ERRORTYPE setSharpness( uint32_t value );
	OMX_ERRORTYPE setSaturation( int32_t value );
	OMX_ERRORTYPE setContrast( int32_t value );
	OMX_ERRORTYPE setWhiteBalanceControl( WhiteBalControl value );
	OMX_ERRORTYPE setExposureControl( ExposureControl value );
	OMX_ERRORTYPE setExposureValue( uint16_t exposure_compensation, float aperture, uint32_t iso_sensitivity, uint32_t shutter_speed_us );
	OMX_ERRORTYPE setFrameStabilisation( bool enabled );
	OMX_ERRORTYPE setMirror( bool hrzn, bool vert );

	const uint32_t brightness();
	const int32_t contrast();
	const int32_t saturation();

protected:
	virtual OMX_ERRORTYPE EventHandler( OMX_EVENTTYPE event, OMX_U32 data1, OMX_U32 data2, OMX_PTR eventdata );

private:
	int Initialize( uint32_t width, uint32_t height );
	int HighSpeedMode();

	uint32_t mDeviceNumber;
	uint32_t mWidth;
	uint32_t mHeight;
	bool mReady;
};

}; // namespace IL