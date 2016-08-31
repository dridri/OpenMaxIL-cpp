#pragma once

#include "Component.h"
#include <IL/OMX_Video.h>
#include <mutex>
#include <condition_variable>

namespace IL {

class VideoEncode : public IL::Component
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

	VideoEncode( uint32_t bitrate_kbps, const CodingType& coding_type = CodingAVC, bool verbose = false );
	~VideoEncode();
	virtual OMX_ERRORTYPE SetState( const State& st );

	const bool dataAvailable() const;
	uint32_t getOutputData( uint8_t* pBuf, bool wait = true );

	OMX_ERRORTYPE setIDRPeriod( uint32_t period );

protected:
	virtual OMX_ERRORTYPE FillBufferDone( OMX_BUFFERHEADERTYPE* buf );

	CodingType mCodingType;
	OMX_BUFFERHEADERTYPE* mBuffer;
	bool mDataAvailable;
	std::mutex mDataAvailableMutex;
	std::condition_variable mDataAvailableCond;
};

} // namespace IL
