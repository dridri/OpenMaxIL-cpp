#pragma once

#include "Component.h"
#include <IL/OMX_Video.h>
#include <mutex>
#include <condition_variable>

namespace IL {

class VideoDecode : public IL::Component
{
public:
	typedef enum {
		CodingUnused = OMX_VIDEO_CodingUnused,
		CodingAutoDetect = OMX_VIDEO_CodingAutoDetect,
		CodingMPEG2 = OMX_VIDEO_CodingMPEG2,
		CodingH263 = OMX_VIDEO_CodingH263,
		CodingMPEG4 = OMX_VIDEO_CodingMPEG4,
		CodingWMV = OMX_VIDEO_CodingWMV,
		CodingRV = OMX_VIDEO_CodingRV,
		CodingAVC = OMX_VIDEO_CodingAVC,
		CodingMJPEG = OMX_VIDEO_CodingMJPEG,
		CodingVP6 = OMX_VIDEO_CodingVP6,
		CodingVP7 = OMX_VIDEO_CodingVP7,
		CodingVP8 = OMX_VIDEO_CodingVP8,
		CodingYUV = OMX_VIDEO_CodingYUV,
		CodingSorenson = OMX_VIDEO_CodingSorenson,
		CodingTheora = OMX_VIDEO_CodingTheora,
		CodingMVC = OMX_VIDEO_CodingMVC,
	} CodingType;

	VideoDecode( uint32_t fps, const CodingType& coding_type = CodingAVC, bool verbose = false );
	VideoDecode( uint32_t width, uint32_t height, uint32_t fps, const CodingType& coding_type = CodingAVC, bool verbose = false );
	~VideoDecode();
	void setRGB565Mode( bool en );

	virtual OMX_ERRORTYPE SetState( const State& st );
	OMX_ERRORTYPE SetupTunnel( Component* next, uint8_t port_input = 0 );

	const bool valid();
	const uint32_t width();
	const uint32_t height();

	const bool needData() const;
	void fillInput( uint8_t* pBuf, uint32_t len, bool corrupted = false );

	const bool dataAvailable() const;
	int32_t getOutputData( uint8_t* pBuf, bool wait = true );

protected:
	virtual OMX_ERRORTYPE EventHandler( OMX_EVENTTYPE event, OMX_U32 data1, OMX_U32 data2, OMX_PTR eventdata );
	virtual OMX_ERRORTYPE EmptyBufferDone( OMX_BUFFERHEADERTYPE* buf );
	virtual OMX_ERRORTYPE FillBufferDone( OMX_BUFFERHEADERTYPE* buf );

	CodingType mCodingType;
	OMX_BUFFERHEADERTYPE* mBuffer;
	OMX_BUFFERHEADERTYPE mBufferCopy;
	OMX_BUFFERHEADERTYPE* mOutputBuffer;
	OMX_U8* mBufferPtr;
	bool mFirstData;
	bool mDecoderValid;
	bool mVideoRunning;
	bool mNeedData;
	std::mutex mNeedDataMutex;
	std::condition_variable mNeedDataCond;
	bool mDataAvailable;
	std::mutex mDataAvailableMutex;
	std::condition_variable mDataAvailableCond;
};

} // namespace IL
