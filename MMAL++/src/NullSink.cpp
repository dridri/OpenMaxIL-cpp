#include "MMAL++/NullSink.h"

using namespace MMAL;


NullSink::NullSink( bool verbose )
	: Component( "vc.ril.null_sink", { PortInit( 0, Video ), PortInit( 1, Image ), PortInit( 2, Audio ) }, std::vector< PortInit >(), verbose )
{
}


NullSink::~NullSink()
{
}
