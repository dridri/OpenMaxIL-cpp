#pragma once

#include "Component.h"
#include <mutex>
#include <list>
#include <condition_variable>

namespace MMAL {

class ImageEncode : public MMAL::Component
{
public:
	typedef enum {
		CodingUnused = 0,
		CodingJPEG = MMAL_ENCODING_JPEG,
		CodingJPEG2K = MMAL_ENCODING_JPEG,
		CodingGIF = MMAL_ENCODING_GIF,
		CodingPNG = MMAL_ENCODING_PNG,
		CodingBMP = MMAL_ENCODING_BMP,
		CodingTGA = MMAL_ENCODING_TGA,
		CodingPPM = MMAL_ENCODING_PPM,
	} CodingType;

	ImageEncode( const CodingType& coding_type = CodingJPEG, bool verbose = false );
	~ImageEncode();

	int AllocateOutputBuffer();
	int SetupTunnel( Component* next, uint8_t port_input );
	int SetState( const Component::State& st );

	void fillBuffer();
	const bool dataAvailable( bool wait = false );
	uint32_t getOutputData( uint8_t* pBuf, bool* eof, bool wait = true );

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
	bool mEndOfFrame;
};

} // namespace MMAL
