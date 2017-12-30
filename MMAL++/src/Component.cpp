#include "MMAL++/Component.h"
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <bcm_host.h>
#include <iostream>

using namespace MMAL;

bool Component::mCoreReady = false;
std::list< Component* > Component::mComponents;

Component::Component( const std::string& name, const std::vector< PortInit >& input_ports, const std::vector< PortInit >& output_ports, bool verbose )
	: mName( name )
	, mVerbose( verbose )
	, mHandle( nullptr )
{
	if ( not mCoreReady ) {
		mCoreReady = true;
		atexit( &Component::onexit );
	}

	mComponents.emplace_back( this );

	for ( PortInit n : input_ports ) {
		Port p;
		p.pParent = this;
		p.nPort = n.id;
		p.type = n.type;
		p.bEnabled = false;
		p.bTunneled = false;
		p.bDisableProprietary = false;
		mInputPorts.insert( std::make_pair( n.id, p ) );
	}
	for ( PortInit n : output_ports ) {
		Port p;
		p.pParent = this;
		p.nPort = n.id;
		p.type = n.type;
		p.bEnabled = false;
		p.bTunneled = false;
		p.bDisableProprietary = false;
		mOutputPorts.insert( std::make_pair( n.id, p ) );
	}

	InitComponent();
}


Component::~Component()
{
}


int Component::InitComponent()
{
	MMAL_STATUS_T status;

	if ( ( status = mmal_component_create( mName.c_str(), &mHandle ) ) < 0 ) {
		return status;
	}

	for ( uint32_t port = 0; port < mHandle->input_num; port++ ) {
		if ( ( status = mmal_port_parameter_set_boolean( mHandle->input[port], MMAL_PARAMETER_ZERO_COPY, MMAL_TRUE ) ) != MMAL_SUCCESS ) {
			printf( "Error : mmal_port_parameter_set_boolean( input[%d], MMAL_PARAMETER_ZERO_COPY, true ) failed\n", port );
		}
		status = mmal_port_disable( mHandle->input[port] );
	}
	for ( uint32_t port = 0; port < mHandle->output_num; port++ ) {
		if ( ( status = mmal_port_parameter_set_boolean( mHandle->output[port], MMAL_PARAMETER_ZERO_COPY, MMAL_TRUE ) ) != MMAL_SUCCESS ) {
			printf( "Error : mmal_port_parameter_set_boolean( output[%d], MMAL_PARAMETER_ZERO_COPY, true ) failed\n", port );
		}
		status = mmal_port_disable( mHandle->output[port] );
	}

	return 0;
}


int Component::SetupTunnel( uint16_t port_output, Component* next, uint16_t port_input, Port* port_copy )
{
	MMAL_STATUS_T status;
	MMAL_CONNECTION_T* connection;

	if ( mHandle == nullptr or next == nullptr or next->mHandle == nullptr ) {
		return -1;
	}

	mmal_format_copy( mHandle->output[port_output]->format, next->mHandle->input[port_input]->format );

	status = mmal_connection_create( &connection, mHandle->output[port_output], next->mHandle->input[port_input], MMAL_CONNECTION_FLAG_TUNNELLING | MMAL_CONNECTION_FLAG_ALLOCATION_ON_INPUT );
	if ( status == MMAL_SUCCESS ) {
		if ( mVerbose ) {
			printf( "Enable connection..." );
			printf( "buffer size is %d bytes, num %d\n", mHandle->output[port_output]->buffer_size, mHandle->output[port_output]->buffer_num );
		}
		status = mmal_connection_enable( connection );
		if ( status == MMAL_SUCCESS ) {
			mOutputPorts[port_output].bTunneled = true;
			mOutputPorts[port_output].bEnabled = true;
			next->mInputPorts[port_input].bTunneled = true;
			next->mInputPorts[port_input].bEnabled = true;
		} else {
			printf( "Error while enabling connection..." );
			mmal_connection_destroy( connection );
		}
	} else {
		printf( "Failed to connect" );
		return -1;
	}

	return 0;
}


MMAL_STATUS_T Component::EnablePort( MMAL_PORT_T* port, void (*cb)(MMAL_PORT_T*, MMAL_BUFFER_HEADER_T*) )
{
	typedef struct param {
		Component* self;
		void (*cb)(MMAL_PORT_T*, MMAL_BUFFER_HEADER_T*);
	} param;
	if ( cb ) {
		param* p = new param;
		p->self = this;
		p->cb = cb;
		port->userdata = (struct MMAL_PORT_USERDATA_T *)p;
		return mmal_port_enable( port, BufferCallback );
	} else {
		port->userdata = nullptr;
	}
	return mmal_port_enable( port, nullptr );
}


void Component::BufferCallback( MMAL_PORT_T* port, MMAL_BUFFER_HEADER_T* buffer )
{
	typedef struct param {
		Component* self;
		void (*cb)(Component*, MMAL_PORT_T*, MMAL_BUFFER_HEADER_T*);
	} param;
	param* p = (param*)port->userdata;
	if ( p != nullptr ) {
		p->cb( p->self, port, buffer );
	}
}


MMAL_STATUS_T Component::FillPortBuffer( MMAL_PORT_T* port, MMAL_POOL_T* pool )
{
	for ( uint32_t q = 0; q < mmal_queue_length(pool->queue); q++ ) {
		MMAL_BUFFER_HEADER_T* buffer = mmal_queue_get( pool->queue );
		if ( not buffer ) {
			fprintf( stderr, "Unable to get a required buffer %d from pool queue\n", q );
		}
		if ( mmal_port_send_buffer( port, buffer ) != MMAL_SUCCESS ) {
			fprintf( stderr, "Unable to send a buffer to port (%d)\n", q );
		}
	}

	return MMAL_SUCCESS;
}


int Component::SetState( const Component::State& st, bool wait )
{
	MMAL_STATUS_T status;
	if ( st == StateExecuting ) {
		for ( auto p : mInputPorts ) {
			if ( p.second.bEnabled == false and p.second.bTunneled == true ) {
				mmal_port_enable( mHandle->input[p.first], nullptr );
				p.second.bEnabled = true;
			}
		}
		for ( auto p : mOutputPorts ) {
			if ( p.second.bEnabled == false and p.second.bTunneled == true ) {
				mmal_port_enable( mHandle->output[p.first], nullptr );
				p.second.bEnabled = true;
			}
		}
		status = mmal_component_enable( mHandle );
	} else {
		status = mmal_component_disable( mHandle );
		for ( auto p : mInputPorts ) {
			if ( p.second.bEnabled == true and p.second.bTunneled == true ) {
				mmal_port_disable( mHandle->input[p.first] );
				p.second.bEnabled = false;
			}
		}
		for ( auto p : mOutputPorts ) {
			if ( p.second.bEnabled == true and p.second.bTunneled == true ) {
				mmal_port_disable( mHandle->output[p.first] );
				p.second.bEnabled = false;
			}
		}
	}
	if ( status != MMAL_SUCCESS ) {
		printf( "Error while enabling component %s : %X\n", mName.c_str(), status );
	}
	return status;
}


void Component::onexit()
{
	printf( "Component::onexit()\n" );
}


const std::string& Component::name() const
{
	return mName;
}


uint64_t Component::ticks64()
{
	struct timespec now;
	clock_gettime( CLOCK_MONOTONIC, &now );
	return (uint64_t)now.tv_sec * 1000000ULL + (uint64_t)now.tv_nsec / 1000ULL;
}
