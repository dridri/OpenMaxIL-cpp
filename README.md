# OpenMaxIL-cpp
OpenMaxIL-cpp is a library specifically designed for Raspberry Pi, making easier C++ access to camera board and encoders / decoders.

## Building
There are no external dependencies except VCOS and IL headers and libraries already pre-shipped in Raspbian images (should be in `/opt/vc` folder)
```bash
mkdir build && cd build
cmake ..
make
sudo make install
```

## Cross compile
It should be compatible with any raspberry cross compilation toolchain.
VC headers and libraries are needed on the host system, the preferred way is to copy them from a Raspberry Pi image (i.e. copy from rPi `/opt/vc` to PC `/opt/vc`).

The `make install` command will also install OpenMaxIL-cpp files in this directory.
```bash
mkdir build && cd build
cmake -Dcross=1 ..
make
sudo make install
```
The library is statically linked, so there is no need to install anything on the target Raspberry image.


## Usage
### Components
Underlying components and port numbers can be found here :
- http://www.jvcref.com/files/PI/documentation/ilcomponents/
- https://github.com/raspberrypi/firmware/tree/master/documentation/ilcomponents

### Basic usage
For best performance and usability, OpenMaxIL-cpp is built around OpenMAX components system.
Most of the components have one or more input and output ports which can be connected either to another component or to a buffer for manual handling.

Here is a simple example using **Camera** and **VideoRender** components connected together :
```cpp
  // Setup camera 0 (can be 1 on Compute Module, which have two camera ports) with 1280x720 resolution
  Camera* camera = new Camera( 1280, 720, 0 );
  // Create video renderer
  VideoRender* render = new VideoRender();

  // Connect camera continuous video output to renderer input
  camera->SetupTunnelVideo( render );

  // Set both components in idle state, this tells OpenMAX that they should be ready to use
  camera->SetState( Component::StateIdle );
  render->SetState( Component::StateIdle );

  // Start both components, then start camera capture
  camera->SetState( Component::StateExecuting );
  render->SetState( Component::StateExecuting );
  camera->SetCapturing( true );
```
Complete example can be found in [`samples`](https://github.com/dridri/OpenMaxIL-cpp/tree/master/samples)/[`camera_live.cpp`](https://github.com/dridri/OpenMaxIL-cpp/tree/master/samples/camera_live.cpp) file

### Buffers
Any input or output port can also be manually handled instead of being connected to another component's port.

#### Input buffers
It is possible to feed a components with custom data (for example, manually giving video frames to a **VideoEncode** component), this is done with the following functions :
* `AllocateInputBuffer( port = 0 )`
  - This function tells that the specified port will be fed manually, needs to be called once and before setting the component to `StateIdle` state.
  - `port` can be omitted and will be set to the first input port of the component.
* `bool needData( uint16_t port )` :
  - This function can be called at anytime when the component is in the `StateExecuting` state, and returns *true* if the component is ready to get input data, or *false* otherwise.
* `void fillInput( uint16_t port, uint8_t* pBuf, uint32_t len, bool corrupted = false, bool eof = false )` :
  - This function will actually feed the component with input data pointed by `pBuf`, and `len` bytes length.
  - `corruped` can be set to *true* to tell the component that the data is known to be corrupted and that it should not stop operating if errors occur.
  - `eof` should be set to *true* if the data contains a complete frame or is the last part of a frame (in case the component is fed with partial frames data)
