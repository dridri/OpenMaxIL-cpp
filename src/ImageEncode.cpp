#include <unistd.h>
#include "ImageEncode.h"
#include <IL/OMX_Types.h>
#include <IL/OMX_Component.h>
#include <IL/OMX_Broadcom.h>

using namespace IL;

ImageEncode::ImageEncode( const CodingType& coding_type, bool verbose )
	: Component( "OMX.broadcom.image_encode", { PortInit( 340, Image ) }, { PortInit( 341, Image ) }, verbose )
	, mBuffer( nullptr )
	, mBufferPtr( nullptr )
	, mDataAvailable( false )
	, mEndOfFrame( false )
{
	OMX_IMAGE_PARAM_PORTFORMATTYPE format;
	OMX_INIT_STRUCTURE( format );
	format.nPortIndex = 341;
	format.eCompressionFormat = (OMX_IMAGE_CODINGTYPE)coding_type;
	SetParameter( OMX_IndexParamImagePortFormat, &format );

	if ( coding_type == CodingJPEG ) {
		OMX_IMAGE_PARAM_QFACTORTYPE qfactor;
		OMX_INIT_STRUCTURE( qfactor );
		qfactor.nPortIndex = 341;
		qfactor.nQFactor = 100;
		SetParameter( OMX_IndexParamQFactor, &qfactor );
	}

	SendCommand( OMX_CommandStateSet, OMX_StateIdle, nullptr );
}


ImageEncode::~ImageEncode()
{
}


OMX_ERRORTYPE ImageEncode::SetupTunnel( Component* next, uint8_t port_input )
{
	return Component::SetupTunnel( 341, next, port_input );
}


OMX_ERRORTYPE ImageEncode::AllocateOutputBuffer()
{
	OMX_ERRORTYPE ret = AllocateBuffers( &mBuffer, 201, true );
	mOutputPorts[201].bEnabled = true;
	mBufferPtr = mBuffer->pBuffer;
	return ret;
}


OMX_ERRORTYPE ImageEncode::SetState( const Component::State& st )
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;

	if ( st == StateExecuting ) {
		if ( mOutputPorts[341].bTunneled == false and mBuffer == nullptr ) {
			AllocateBuffers( &mBuffer, 341, true );
			mOutputPorts[341].bEnabled = true;
			mBufferPtr = mBuffer->pBuffer;
		}

		ret = IL::Component::SetState(st);
		if ( ret != OMX_ErrorNone ) {
			return ret;
		}

		if ( mBuffer ) {
// 			usleep( 1000 * 500 );
			ret = ((OMX_COMPONENTTYPE*)mHandle)->FillThisBuffer( mHandle, mBuffer );
		}
	} else {
		ret = IL::Component::SetState(st);
	}

	return ret;
}


OMX_ERRORTYPE ImageEncode::FillBufferDone( OMX_BUFFERHEADERTYPE* buf )
{
	std::unique_lock<std::mutex> locker( mDataAvailableMutex );
	mEndOfFrame = ( buf->nFlags & OMX_BUFFERFLAG_ENDOFFRAME );
	mDataAvailable = true;
	mDataAvailableCond.notify_all();
	return OMX_ErrorNone;
}



const bool ImageEncode::dataAvailable( bool wait )
{
	if ( not wait or mDataAvailable ) {
		return mDataAvailable;
	}

	std::unique_lock<std::mutex> locker( mDataAvailableMutex );
	mDataAvailableCond.wait( locker );
	locker.unlock();

	return mDataAvailable;
}


void ImageEncode::fillBuffer()
{
	if ( mBuffer ) {
		OMX_ERRORTYPE err = ((OMX_COMPONENTTYPE*)mHandle)->FillThisBuffer( mHandle, mBuffer );
	}
}


uint32_t ImageEncode::getOutputData( uint8_t* pBuf, bool* eof, bool wait )
{
	uint32_t datalen = 0;

	std::unique_lock<std::mutex> locker( mDataAvailableMutex );


	if ( mDataAvailable ) {
		memcpy( pBuf, mBuffer->pBuffer, mBuffer->nFilledLen );
		datalen = mBuffer->nFilledLen;
		mDataAvailable = false;
	} else if ( wait ) {
		mDataAvailableCond.wait( locker );
		memcpy( pBuf, mBuffer->pBuffer, mBuffer->nFilledLen );
		datalen = mBuffer->nFilledLen;
		mDataAvailable = false;
	} else {
		return OMX_ErrorOverflow;
	}

	*eof = mEndOfFrame;
	if ( mEndOfFrame ) {
		mEndOfFrame = false;
	}

	locker.unlock();

	if ( mBuffer ) {
		OMX_ERRORTYPE err = ((OMX_COMPONENTTYPE*)mHandle)->FillThisBuffer( mHandle, mBuffer );
		if ( err != OMX_ErrorNone ) {
			return err;
		}
	}

	return datalen;
}
