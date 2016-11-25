#include "NullSink.h"

using namespace IL;

NullSink::NullSink( bool verbose )
	: Component( "OMX.broadcom.null_sink", { PortInit( 240, Video ), PortInit( 241, Image ), PortInit( 242, Audio ) }, std::vector< PortInit >(), verbose )
{
	SendCommand( OMX_CommandStateSet, OMX_StateIdle, nullptr );
}


NullSink::~NullSink()
{
}
