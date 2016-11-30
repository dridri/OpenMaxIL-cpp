#include <unistd.h>
#include <iostream>
#include "MMAL++/VideoEncode.h"

using namespace MMAL;

VideoEncode::VideoEncode( uint32_t bitrate_kbps, const CodingType& coding_type, bool verbose )
	: Component( "vc.ril.video_encode", { 0 }, { 0 }, verbose )
{
	mmal_format_copy( mHandle->output[0]->format, mHandle->input[0]->format );
	mHandle->output[0]->format->encoding = (MMAL_FOURCC_T)coding_type;
	mHandle->output[0]->format->bitrate = bitrate_kbps * 1000;
	mHandle->output[0]->buffer_size = mHandle->output[0]->buffer_size_recommended;
	mHandle->output[0]->buffer_num = mHandle->output[0]->buffer_num_recommended;
	mHandle->output[0]->format->es->video.frame_rate.num = 0;
	mHandle->output[0]->format->es->video.frame_rate.den = 1;
	if ( mmal_port_format_commit( mHandle->output[0] ) != MMAL_SUCCESS ) {
		vcos_log_error( "Unable to set format on video encoder output port" );
	}

// 	MMAL_PARAMETER_VIDEO_RATECONTROL_T param = { { MMAL_PARAMETER_RATECONTROL, sizeof(param) }, MMAL_VIDEO_RATECONTROL_VARIABLE_SKIP_FRAMES };
// 	mmal_port_parameter_set( mHandle->output[0] , &param.hdr );

	if ( coding_type == CodingAVC ) {
		setIDRPeriod( 10 );
// 		mmal_port_parameter_set_boolean( mHandle->output[0], MMAL_PARAMETER_VIDEO_ENCODE_HEADER_ON_OPEN, true );
		mmal_port_parameter_set_boolean( mHandle->output[0], MMAL_PARAMETER_VIDEO_ENCODE_INLINE_HEADER, true );
// 		mmal_port_parameter_set_boolean( mHandle->output[0], MMAL_PARAMETER_VIDEO_ENCODE_H264_LOW_DELAY_HRD_FLAG, true );
		mmal_port_parameter_set_boolean( mHandle->output[0], MMAL_PARAMETER_VIDEO_ENCODE_H264_LOW_LATENCY, true );
// 		mmal_port_parameter_set_boolean( mHandle->output[0], MMAL_PARAMETER_VIDEO_ENCODE_SEI_ENABLE, true );
// 		mmal_port_parameter_set_boolean( mHandle->output[0], MMAL_PARAMETER_VIDEO_ENCODE_H264_DISABLE_CABAC, true ); // TEST
// 		MMAL_PARAMETER_VIDEO_EEDE_ENABLE_T eede = { { MMAL_PARAMETER_VIDEO_EEDE_ENABLE, sizeof(param) }, 1 };
// 		mmal_port_parameter_set( mHandle->output[0] , &eede.hdr );
// 		MMAL_PARAMETER_VIDEO_EEDE_LOSSRATE_T lossrate = { { MMAL_PARAMETER_VIDEO_EEDE_ENABLE, sizeof(param) }, 32 };
// 		mmal_port_parameter_set( mHandle->output[0] , &lossrate.hdr );
		MMAL_PARAMETER_VIDEO_PROFILE_T profile = { { MMAL_PARAMETER_PROFILE, sizeof(profile) }, { { MMAL_VIDEO_PROFILE_H264_HIGH, MMAL_VIDEO_LEVEL_H264_4 } } };
		mmal_port_parameter_set( mHandle->output[0] , &profile.hdr );
	}

	mOutputPool = mmal_port_pool_create( mHandle->output[0], mHandle->output[0]->buffer_num, mHandle->output[0]->buffer_size );
}


VideoEncode::~VideoEncode()
{
}


int VideoEncode::SetupTunnel( Component* next, uint8_t port_input )
{
	return Component::SetupTunnel( 0, next, port_input );
}


int VideoEncode::SetState( const Component::State& st )
{
	int ret = MMAL::Component::SetState(st);
	if ( ret != 0 ) {
		return ret;
	}

	EnablePort( mHandle->output[0], (void (*)(MMAL_PORT_T*, MMAL_BUFFER_HEADER_T*))&VideoEncode::OutputBufferCallback );

	return FillPortBuffer( mHandle->output[0], mOutputPool );
}


int VideoEncode::setIDRPeriod( uint32_t period )
{
	MMAL_PARAMETER_UINT32_T param = { { MMAL_PARAMETER_INTRAPERIOD, sizeof(param) }, period };
	return mmal_port_parameter_set( mHandle->output[0], &param.hdr );
}


void VideoEncode::OutputBufferCallback( MMAL_PORT_T* port, MMAL_BUFFER_HEADER_T* buffer )
{
	printf( "OutputBufferCallback 1\n" );
	MMAL_BUFFER_HEADER_T* new_buffer;
	Buffer buf;

	mmal_buffer_header_mem_lock( buffer );
	std::unique_lock<std::mutex> locker( mDataAvailableMutex );
	buf.buf = (uint8_t*)malloc( buffer->length );
	buf.size = buffer->length;
	memcpy( buf.buf, buffer->data, buf.size );
	mBuffers.emplace_back( buf );
	mDataAvailableCond.notify_all();
	locker.unlock();
	mmal_buffer_header_mem_unlock( buffer );

	mmal_buffer_header_release( buffer );
	printf( "OutputBufferCallback 2\n" );
	if ( port->is_enabled ) {
		MMAL_STATUS_T status = MMAL_SUCCESS;
		if ( ( new_buffer = mmal_queue_get( mOutputPool->queue ) ) ) {
			status = mmal_port_send_buffer( port, new_buffer );
		}
		if ( !new_buffer or status != MMAL_SUCCESS ) {
			fprintf( stderr, "Unable to return a buffer to the video_encode port\n" );
		}
	}
	printf( "OutputBufferCallback 3\n" );
}


const bool VideoEncode::dataAvailable() const
{
	return ( mBuffers.size() > 0 );
}


uint32_t VideoEncode::getOutputData( uint8_t* pBuf, bool wait )
{
	uint32_t datalen = 0;

	std::unique_lock<std::mutex> locker( mDataAvailableMutex );

	if ( mBuffers.size() == 0 ) {
		if ( wait ) {
			mDataAvailableCond.wait( locker );
		} else {
			locker.unlock();
			return 0;
		}
	}

	Buffer buf = mBuffers.front();
	mBuffers.pop_front();
	memcpy( pBuf, buf.buf, buf.size );
	datalen = buf.size;
	free( buf.buf );

	locker.unlock();
	return datalen;
}
