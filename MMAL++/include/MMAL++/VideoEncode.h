#pragma once

#include "Component.h"
#include <mutex>
#include <list>
#include <condition_variable>

namespace MMAL {

class VideoEncode : public MMAL::Component
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

	VideoEncode( uint32_t bitrate_kbps, const CodingType& coding_type = CodingAVC, bool verbose = false );
	~VideoEncode();

	virtual int SetState( const State& st );
	int SetupTunnel( Component* next, uint8_t port_input = 0 );

	const bool dataAvailable() const;
	uint32_t getOutputData( uint8_t* pBuf, bool wait = true );

	int setIDRPeriod( uint32_t period );

protected:
	void OutputBufferCallback( MMAL_PORT_T* port, MMAL_BUFFER_HEADER_T* pool );

	typedef struct Buffer {
		uint8_t* buf;
		uint32_t size;
	} Buffer;

	MMAL_POOL_T* mOutputPool;
	std::list< Buffer > mBuffers;
	std::mutex mDataAvailableMutex;
	std::condition_variable mDataAvailableCond;
};

} // namespace MMAL
