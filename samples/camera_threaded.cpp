#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include "../include/Camera.h"
#include "../include/VideoEncode.h"
#include "../include/VideoDecode.h"
#include "../include/VideoRender.h"
#include <fstream>
#include <iostream>

using namespace IL;


struct State {
	Camera* camera;
	VideoEncode* preview_encode;
	VideoEncode* record_encode;
	bool recording; // TODO : set this variable to true from an external thread to activate recording
	bool running;
	pthread_t preview_thid;
	pthread_t record_thid;
} state = { nullptr, nullptr, nullptr, false, false };


void terminate( int sig )
{
	state.running = false;
	pthread_join( state.record_thid, nullptr );
	pthread_join( state.preview_thid, nullptr );
	exit(0);
}


void process_image( uint8_t* data )
{
	uint8_t* y_plane = data;
	uint8_t* u_plane = &data[ 1280 * 720 ];
	uint8_t* v_plane = &data[ 1280 * 720 + ( 1280 / 2 ) * ( 720 / 2 ) ];

	// TODO : process image here (i.e. pass it to OpenCV, export to external program, ...)
}


void* preview_thread( void* argp )
{
#if 0
	VideoDecode* decode = new VideoDecode( 1280, 720, 0, VideoDecode::CodingMJPEG, true );
	VideoRender* render = new VideoRender( true );
	decode->SetupTunnel( render );
	decode->SetState( Component::StateIdle );
	render->SetState( Component::StateIdle );
	decode->SetState( Component::StateExecuting );
	render->SetState( Component::StateExecuting );
#endif

	bool zero_copy = false;
	uint8_t* data = nullptr;
	uint8_t* mjpeg_data = nullptr;
	if ( not zero_copy ) {
		// Allocate space for image
		data = new uint8_t[1280*720*sizeof(uint16_t)];
		mjpeg_data = new uint8_t[(int)(1280*720*1.5)];
	}

	// ATTENTION : Each loop must take less time than it takes to the camera to take one frame
	// otherwise it will cause underflow which can lead to camera stalling
	// a good solution is to implement frame skipping (measure time between to loops, if this time
	// is too big, just skip image processing and MJPEG sendout)
	while ( state.running ) {
		if ( zero_copy ) {
			// Retrieve OMX buffer
			data = state.camera->outputPorts()[70].buffer->pBuffer;
			mjpeg_data = state.preview_encode->outputPorts()[201].buffer->pBuffer;
		}
		// Get YUV420 image from preview port, this is a blocking call
		// If zero-copy is activated, we don't pass any buffer
		int32_t datalen = state.camera->getOutputData( 70, zero_copy ? nullptr : data );

		if ( datalen > 0 ) {
			// Send it to the MJPEG encoder
			state.preview_encode->fillInput( 200, data, datalen, false, true );
			// Process the image
			process_image( data );
		}

		while ( ( datalen = state.preview_encode->getOutputData( zero_copy ? nullptr : mjpeg_data, false ) ) > 0 ) {
			// TODO : send MJPEG data to a client, 'data' contains exactly one coded frame which is 'datalen' long
#if 0
			decode->fillInput( mjpeg_data, datalen );
#endif
		}
	}

	return nullptr;
}


void* record_thread( void* argp )
{
	uint8_t* data = new uint8_t[65536*4];

	while ( state.running ) {
		// Consume h264 data, this is a blocking call
		int32_t datalen = state.record_encode->getOutputData( data );
		if ( datalen > 0 && state.recording ) {
			// TODO : save data somewhere
		}
	}

	return nullptr;
}


int main( int ac, char** av )
{
	// We register signal handler to properly destroy OpenMAX context (by calling exit() in the handler)
	static int sig_list[] = { 2, 3, 6, 7, 9, 11, 15 };
	uint32_t i;
	for ( i = 0; i <= sizeof(sig_list)/sizeof(int); i++ ) {
		struct sigaction sa;
		memset( &sa, 0, sizeof(sa) );
		sa.sa_handler = &terminate;
		sigaction( sig_list[i], &sa, NULL );
	}

	// Initialize hardware
	bcm_host_init();

	// Create components
	state.camera = new Camera( 1280, 720, 0, false, 0, false );
	state.preview_encode = new VideoEncode( 8192, VideoEncode::CodingMJPEG, false, false );
	state.record_encode = new VideoEncode( 4096, VideoEncode::CodingAVC, false, false );

	// Setup camera
	state.camera->setFramerate( 30 );

	// Copy preview port definition to the encoder to help it handle incoming data
	Component::CopyPort( &state.camera->outputPorts()[70], &state.preview_encode->inputPorts()[200] );

	// Tunnel video port to AVC encoder
	state.camera->SetupTunnelVideo( state.record_encode );

	// Prepare components for next step
	state.camera->SetState( Component::StateIdle );
	state.preview_encode->SetState( Component::StateIdle );
	state.record_encode->SetState( Component::StateIdle );

	// Allocate buffers that will be processed manually
	state.camera->AllocateOutputBuffer( 70 );
	state.preview_encode->AllocateInputBuffer( 200 );

	// Start components
	state.camera->SetState( Component::StateExecuting );
	state.preview_encode->SetState( Component::StateExecuting );
	state.record_encode->SetState( Component::StateExecuting );

	// Start capturing
	state.camera->SetCapturing( true );
	state.running = true;

	// Start threads
	pthread_create( &state.preview_thid, nullptr, &preview_thread, nullptr );
	pthread_create( &state.record_thid, nullptr, &record_thread, nullptr );

	while ( 1 ) {
		usleep( 1000 * 1000 );
	}
	return 0;
}
