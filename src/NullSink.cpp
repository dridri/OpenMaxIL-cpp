#include "NullSink.h"

using namespace IL;

NullSink::NullSink( bool verbose )
	: Component( "OMX.broadcom.null_sink", { 240, 241, 242 }, std::vector< uint8_t >(), verbose )
{
}


NullSink::~NullSink()
{
}
