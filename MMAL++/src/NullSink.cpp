#include "MMAL++/NullSink.h"

using namespace MMAL;

NullSink::NullSink( bool verbose )
	: Component( "vc.ril.null_sink", { 0, 1, 2 }, std::vector<uint8_t>(), verbose )
{
}


NullSink::~NullSink()
{
}
