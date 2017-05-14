#ifndef IL_IMAGEENCODE_H
#define IL_IMAGEENCODE_H

#include "Component.h"
#include <IL/OMX_Image.h>
#include <mutex>
#include <condition_variable>

namespace IL {

class ImageEncode : public IL::Component
{
public:
	typedef enum {
		CodingUnused = OMX_IMAGE_CodingUnused,
		CodingAutoDetect = OMX_IMAGE_CodingAutoDetect,
		CodingJPEG = OMX_IMAGE_CodingJPEG,
		CodingJPEG2K = OMX_IMAGE_CodingJPEG2K,
		CodingEXIF = OMX_IMAGE_CodingEXIF,
		CodingTIFF = OMX_IMAGE_CodingTIFF,
		CodingGIF = OMX_IMAGE_CodingGIF,
		CodingPNG = OMX_IMAGE_CodingPNG,
		CodingLZW = OMX_IMAGE_CodingLZW,
		CodingBMP = OMX_IMAGE_CodingBMP,
		CodingTGA = OMX_IMAGE_CodingTGA,
		CodingPPM = OMX_IMAGE_CodingPPM,
	} CodingType;

	ImageEncode( const CodingType& coding_type = CodingJPEG, bool verbose = false );
	~ImageEncode();

	OMX_ERRORTYPE AllocateOutputBuffer();
	OMX_ERRORTYPE SetupTunnel( Component* next, uint8_t port_input );
	OMX_ERRORTYPE SetState( const Component::State& st );

	void fillBuffer();
	const bool dataAvailable( bool wait = false );
	uint32_t getOutputData( uint8_t* pBuf, bool* eof, bool wait = true );

protected:
	virtual OMX_ERRORTYPE FillBufferDone( OMX_BUFFERHEADERTYPE* buf );

	OMX_BUFFERHEADERTYPE* mBuffer;
	OMX_U8* mBufferPtr;
	bool mDataAvailable;
	bool mEndOfFrame;
	std::mutex mDataAvailableMutex;
	std::condition_variable mDataAvailableCond;
};
}

#endif // IL_IMAGEENCODE_H
