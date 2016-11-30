#pragma once

#include "Component.h"
#include <mutex>
#include <condition_variable>

namespace MMAL {

class VideoDecode : public MMAL::Component
{
public:
	typedef enum {
		CodingUnused = 0,
		CodingMPEG2 = MMAL_ENCODING_MP2V,
		CodingH263 = MMAL_ENCODING_H263,
		CodingMPEG4 = MMAL_ENCODING_MP4V,
		CodingWMV1 = MMAL_ENCODING_WMV1,
		CodingWMV2 = MMAL_ENCODING_WMV2,
		CodingWMV3 = MMAL_ENCODING_WMV3,
		CodingAVC = MMAL_ENCODING_H264,
		CodingMJPEG = MMAL_ENCODING_MJPEG,
		CodingVP6 = MMAL_ENCODING_VP6,
		CodingVP7 = MMAL_ENCODING_VP7,
		CodingVP8 = MMAL_ENCODING_VP8,
		CodingYUV = MMAL_ENCODING_I420,
		CodingTheora = MMAL_ENCODING_THEORA,
		CodingMVC = MMAL_ENCODING_MVC,
	} CodingType;

	VideoDecode( uint32_t fps, const CodingType& coding_type = CodingAVC, bool verbose = false );
	~VideoDecode();

	virtual int SetState( const State& st );
	int SetupTunnel( Component* next, uint8_t port_input = 0 );

	const bool needData() const;
	void fillInput( uint8_t* pBuf, uint32_t len, bool corrupted = false );

protected:
	void ControlCallback( MMAL_PORT_T* port, MMAL_BUFFER_HEADER_T* buffer );
	void InputBufferCallback( MMAL_PORT_T* port, MMAL_BUFFER_HEADER_T* buffer );

	MMAL_POOL_T* mInputPool;
	bool mNeedData;
};

} // namespace MMAL
