#include "Clock.h"
#include <IL/OMX_Video.h>
#include <IL/OMX_Core.h>
#include <IL/OMX_Types.h>
#include <IL/OMX_Component.h>
#include <IL/OMX_Broadcom.h>

using namespace IL;

Clock::Clock(  bool verbose )
	: Component( "OMX.broadcom.clock", std::vector< PortInit >(), { PortInit( 80, PortType::Clock ), PortInit( 81, PortType::Clock ), PortInit( 82, PortType::Clock ), PortInit( 83, PortType::Clock ), PortInit( 84, PortType::Clock ), PortInit( 85, PortType::Clock ) }, verbose )
{
	OMX_TIME_CONFIG_CLOCKSTATETYPE state;
	OMX_INIT_STRUCTURE( state );
	state.eState = OMX_TIME_ClockStateRunning;
	state.nStartTime = { 0UL, 0UL };
	state.nOffset = { 0UL, 0UL };
	state.nWaitMask = 0;
	SetConfig( OMX_IndexConfigTimeClockState, &state );
}


Clock::~Clock()
{
}


OMX_ERRORTYPE Clock::SetupTunnel( Component* next, uint8_t port_input )
{
	for ( uint32_t i = 80; i <= 85; i++ ) {
		if ( mOutputPorts[i].bTunneled == false ) {
			return Component::SetupTunnel( mOutputPorts[i].nPort, next, port_input );
		}
	}
	return OMX_ErrorNoMore;
}
