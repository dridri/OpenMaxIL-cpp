#include <unistd.h>
#include "../include/Camera.h"
#include "../include/VideoRender.h"

using namespace IL;

#define SENSOR_MODE 4
#define WIDTH 1640
#define HEIGHT 1232
#define FPS 30


void process_image( uint8_t* data )
{
	uint8_t* y_plane = data;
	uint8_t* u_plane = &data[ WIDTH * HEIGHT ];
	uint8_t* v_plane = &data[ WIDTH * HEIGHT + ( WIDTH / 2 ) * ( HEIGHT / 2 ) ];

	// YUV420
	// Y plane is [WIDTH * HEIGHT] unsigned 8 bits luminance
	// U plane is [WIDTH/2 * HEIGHT/2] (stretched when displaying) signed 8 bits Cb
	// U plane is [WIDTH/2 * HEIGHT/2] (stretched when displaying) signed 8 bits Cr

	// TODO : process image here (i.e. pass it to OpenCV, export to external program, ...)
	// For best performances, try to avoid use of memcpy() and YUV<->RGB conversion
}


int main( int ac, char** av )
{
	bcm_host_init();

	// Create camera and on-screen renderer components
	Camera* camera = new Camera( WIDTH, HEIGHT, 0, false, SENSOR_MODE, true );
	VideoRender* render = new VideoRender( true );

	// Setup camera
	camera->setFramerate( FPS );
	camera->DisableProprietaryTunnels( 71 ); // This disables image slicing, without it we would receive only WIDTH*16 image slices in getOutputData

	// Tunnel preview port to on-screen renderer
	camera->SetupTunnelPreview( render );

	// Allocate buffer that will be processed manually
	camera->AllocateOutputBuffer( 71 );

	// Tell OpenMAX that components are ready
	camera->SetState( Component::StateIdle );
	render->SetState( Component::StateIdle );

	// Start capturing
	camera->SetState( Component::StateExecuting );
	render->SetState( Component::StateExecuting );
	camera->SetCapturing( true );

	// ATTENTION : Each loop must take less time than one camera frame (33ms in 30fps configuration)
	// otherwise it will cause underflow which can lead to camera stalling
	// a good solution is to implement frame skipping (measure time between two loops, if this time
	// is too big, just skip image processing)
	while ( 1 ) {
		// Retrieve OMX buffer
		uint8_t* data = camera->outputPorts()[71].buffer->pBuffer;

		// Get YUV420 image from video port, this is a blocking call
		// We don't pass any buffer, as we got it's internal pointer just above, which avoids an additionnal memcpy
		int32_t datalen = camera->getOutputData( 71, nullptr );

		if ( datalen > 0 ) {
			// Process the image
			process_image( data );
		}
	}

	return 0;
}




