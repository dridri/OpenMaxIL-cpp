#include "MMAL++/VideoDecode.h"
#include <unistd.h>
#include <iostream>

using namespace MMAL;

VideoDecode::VideoDecode( uint32_t fps, const CodingType& coding_type, bool verbose )
	: Component( "vc.ril.video_decode", { PortInit( 0, Video ) }, { PortInit( 0, Video ) }, verbose )
	, mNeedData( false )
{
	MMAL_ES_FORMAT_T* format_in = mHandle->input[0]->format;
	format_in->type = MMAL_ES_TYPE_VIDEO;
	format_in->encoding = (MMAL_FOURCC_T)coding_type;
	format_in->es->video.width = format_in->es->video.crop.width = 0;
	format_in->es->video.height = format_in->es->video.crop.height = 0;
	format_in->es->video.frame_rate.num = fps;
	format_in->es->video.frame_rate.den = 1;
	format_in->es->video.par.num = 0;
	format_in->es->video.par.den = 1;
// 	format_in->flags |= MMAL_ES_FORMAT_FLAG_FRAMED; // Nope
	mHandle->input[0]->buffer_num = mHandle->input[0]->buffer_num_recommended;
	mHandle->input[0]->buffer_size = mHandle->input[0]->buffer_size_recommended;
	if ( mmal_port_format_commit( mHandle->input[0] ) != MMAL_SUCCESS ) {
		vcos_log_error( "Unable to set format on video decoder input port" );
	}

	MMAL_ES_FORMAT_T* format_out = mHandle->output[0]->format;
	format_out->type = MMAL_ES_TYPE_VIDEO;
	format_out->encoding = MMAL_ENCODING_OPAQUE;
	format_out->encoding_variant = MMAL_ENCODING_I420;
	format_out->es->video.width = format_in->es->video.crop.width = 0;
	format_out->es->video.height = format_in->es->video.crop.height = 0;
	if ( mmal_port_format_commit( mHandle->output[0] ) != MMAL_SUCCESS ) {
		vcos_log_error( "Unable to set format on video decoder output port" );
	}

	mmal_port_parameter_set_boolean( mHandle->input[0], MMAL_PARAMETER_VIDEO_DECODE_ERROR_CONCEALMENT, false );
	mmal_port_parameter_set_boolean( mHandle->input[0], MMAL_PARAMETER_VIDEO_INTERPOLATE_TIMESTAMPS, false );
// 	mmal_port_parameter_set_uint32( mHandle->input[0], MMAL_PARAMETER_VIDEO_MAX_NUM_CALLBACKS, -5 );

	MMAL_PARAMETER_VIDEO_INTERLACE_TYPE_T interlace_type = { { MMAL_PARAMETER_VIDEO_INTERLACE_TYPE, sizeof( interlace_type ) } };
	mmal_port_parameter_get( mHandle->output[0], &interlace_type.hdr );
	interlace_type.eMode = MMAL_InterlaceFieldsInterleavedLowerFirst;
	mmal_port_parameter_set( mHandle->output[0], &interlace_type.hdr );

	mInputPool = mmal_port_pool_create( mHandle->input[0], mHandle->input[0]->buffer_num, mHandle->input[0]->buffer_size );
}


VideoDecode::~VideoDecode()
{
}


int VideoDecode::SetupTunnel( Component* next, uint8_t port_input )
{
	return Component::SetupTunnel( 0, next, port_input );
}


int VideoDecode::SetState( const Component::State& st )
{
	int ret = MMAL::Component::SetState(st);
	if ( ret != 0 ) {
		return ret;
	}

	EnablePort( mHandle->control, (void (*)(MMAL_PORT_T*, MMAL_BUFFER_HEADER_T*))&VideoDecode::ControlCallback );
	EnablePort( mHandle->input[0], (void (*)(MMAL_PORT_T*, MMAL_BUFFER_HEADER_T*))&VideoDecode::InputBufferCallback );

	mNeedData = true;
	return FillPortBuffer( mHandle->input[0], mInputPool );
}


void VideoDecode::ControlCallback( MMAL_PORT_T* port, MMAL_BUFFER_HEADER_T* buffer )
{
	char s[5] = "";
	strncpy( s, (char*)&buffer->cmd, 4 );
	printf( "ControlCallback : %s\n", s ); fflush(stdout);
}


void VideoDecode::InputBufferCallback( MMAL_PORT_T* port, MMAL_BUFFER_HEADER_T* buffer )
{
	mmal_buffer_header_release( buffer );
}


const bool VideoDecode::needData() const
{
	return mNeedData;
}


void VideoDecode::fillInput( uint8_t* pBuf, uint32_t len, bool corrupted )
{
	MMAL_STATUS_T status;
	MMAL_BUFFER_HEADER_T* buffer;
	(void)corrupted; // TODO : try to use it

	if ( ( buffer = mmal_queue_get( mInputPool->queue ) ) != nullptr ) {
		char s[5] = "";
		strncpy( s, (char*)&buffer->cmd, 4 );
// 		printf( "Copy %d bytes to %p [%d, %s, %d, %08X]\n", len, buffer->data, buffer->alloc_size, s, buffer->offset, buffer->flags );
		mmal_buffer_header_mem_lock( buffer );
// 		printf( "locked\n" );
// 		memcpy( buffer->data, pBuf, len );
		uint32_t* start = (uint32_t*)buffer->data;
		uint32_t* end = start + ( len >> 2 ) + 1;
		uint32_t* copy = (uint32_t*)pBuf;
		while ( start < end ) {
			*(start++) = *(copy++);
		}
		buffer->length = len;
		buffer->flags = ( corrupted ? MMAL_BUFFER_HEADER_FLAG_CORRUPTED : 0 );
		mmal_buffer_header_mem_unlock( buffer );
		if ( ( status = mmal_port_send_buffer( mHandle->input[0], buffer ) ) != MMAL_SUCCESS ) {
			printf( "VideoDecode::fillInput error : mmal_port_send_buffer failed (%d)\n", status ); fflush(stdout);
		}
	} else {
		printf( "VideoDecode::fillInput error : No input buffer available in queue\n" ); fflush(stdout);
	}
}
