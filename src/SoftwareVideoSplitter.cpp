#include <unistd.h>
#include "SoftwareVideoSplitter.h"

using namespace IL;


SoftwareVideoSplitter::SoftwareVideoSplitter( bool verbose )
	: Component( "software_video_splitter", { PortInit( 400, Video ) }, std::vector< PortInit >(), verbose )
	, mPaused( true )
	, mThread( new std::thread( [this]() { this->ThreadRun(); } ) )
	, mLastOutput( 400 )
{
}


SoftwareVideoSplitter::~SoftwareVideoSplitter()
{
}


OMX_ERRORTYPE SoftwareVideoSplitter::SetState( const Component::State& st )
{

	if ( st == StateIdle ) {
		mPaused = true;
		mInputPorts[400].bEnabled = false;
		for ( auto p : mOutputPorts ) {
			p.second.bEnabled = false;
		}
	} else if ( st == StateExecuting ) {
		mInputPorts[400].bEnabled = true;
		for ( auto p : mOutputPorts ) {
			p.second.bEnabled = true;
		}
		mPaused = false;
	}

	return OMX_ErrorNone;
}


bool SoftwareVideoSplitter::CustomTunnelInput( Component* prev, uint16_t port_output, uint16_t port_input )
{
	if ( mVerbose ) {
		mDebugCallback( 1, "%p[%s] SoftwareVideoSplitter::CustomTunnelInput( %p, %d, %d )\n", this, mName.c_str(), prev, port_output, port_input );
	}

	prev->AllocateOutputBuffer( port_output );
	prev->outputPorts()[ port_output ].pTunnel = this;
	prev->outputPorts()[ port_output ].nTunnelPort = port_input;
	prev->outputPorts()[ port_output ].bTunneled = true;
	mInputPorts[ 400 ].pTunnel = prev;
	mInputPorts[ 400 ].nTunnelPort = port_output;
	mInputPorts[ 400 ].bTunneled = true;

	return true;
}


OMX_ERRORTYPE SoftwareVideoSplitter::SetupTunnel( Component* next, uint8_t port_input )
{
	uint16_t port_output = ( ++mLastOutput );
	Port p;
	p.pParent = this;
	p.nPort = port_output;
	p.type = Video;
	p.bEnabled = false;
	p.bTunneled = false;
	p.buffer = nullptr;
	p.bufferNeedsData = false;
	p.bufferDataAvailable = false;
	mOutputPorts.insert( std::make_pair( port_output, p ) );

	if ( mVerbose ) {
		mDebugCallback( 1, "%p[%s] SoftwareVideoSplitter::SetupTunnel( %d, %p, %d )\n", this, mName.c_str(), port_output, next, port_input );
	}

	if ( port_input == 0 ) {
		for ( auto p : next->inputPorts() ) {
			Port port = p.second;
			if ( ( (uint32_t)port.type & (uint32_t)Video ) != 0 ) {
				port_input = port.nPort;
				break;
			}
		}
		if ( mVerbose ) {
			mDebugCallback( 1, "%p[%s] SoftwareVideoSplitter::SetupTunnel detected input port : %d\n", mHandle, mName.c_str(), port_input );
		}
	}

	next->AllocateInputBuffer( port_input );
	next->inputPorts()[ port_input ].pTunnel = this;
	next->inputPorts()[ port_input ].nTunnelPort = port_output;
	next->inputPorts()[ port_input ].bTunneled = true;
	mOutputPorts[ port_output ].pTunnel = next;
	mOutputPorts[ port_output ].nTunnelPort = port_input;
	mOutputPorts[ port_output ].bTunneled = true;

	return OMX_ErrorNone;
}


void SoftwareVideoSplitter::ThreadRun()
{
	uint32_t datasize = 0;
	uint8_t* data = new uint8_t[1 * 1024 * 1024];

	while ( true )
	{
		if ( mPaused ) {
			usleep( 1000 * 250 );
		}
		if ( mInputPorts[400].bTunneled == false or mInputPorts[400].bEnabled == false ) {
			usleep( 1000 * 250 );
			continue;
		}

		mDebugCallback( 1, " =================> SoftwareVideoSplitter waiting data on port %d...\n", mInputPorts[400].nTunnelPort );
		datasize = mInputPorts[400].pTunnel->getOutputData( mInputPorts[400].nTunnelPort, data, true );
		bool eof = ( mInputPorts[400].pTunnel->outputPorts()[mInputPorts[400].nTunnelPort].buffer->nFlags & OMX_BUFFERFLAG_ENDOFFRAME );
		mDebugCallback( 1, " =================> SoftwareVideoSplitter produced %d bytes\n", datasize );

		if ( datasize > 0 ) {
			for ( auto x : mOutputPorts ) {
				Port p = x.second;
				uint32_t block = p.pTunnel->inputPorts()[p.nTunnelPort].buffer->nAllocLen;
				for ( uint32_t i = 0; i < datasize; i += block ) {
					bool is_eof = ( i + block >= datasize );
					if ( i + block >= datasize ) {
						block = ( datasize - i);
					}
					p.pTunnel->fillInput( p.nTunnelPort, data + i, block, is_eof );
				}
			}
		}
	}

	delete data;
}
