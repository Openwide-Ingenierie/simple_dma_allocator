## Presentation

This is the example code shown in the article : [here](https://linuxembedded.fr/2025/05/implementation-dun-module-dallocation-memoire-coherente-avec-lapi-dma-sous-linux).

The module is a simple wrapper around the DMA API's allocation method : dma_alloc_coherent, meant to show how the API can be used in the kernel space.  

The test app is a user space binary that interacts with the module's sysfs, asks for an allocation of a given size, then communicates the physical adress of the allocated buffer to an auxiliary microcontroller through RPMSG.   

The firmware on that microntroller is not shared here, as it is very hardware dependent. The one I used for this example was based on the rpmsg_lite_str_echo_rtos from the MCUXpresso SDK, slightly modified to be able to read the adress from a message, then fill the buffer through a DMA transfer.  

## Usage

You should first use insmod or modprobe to insert the module in your kernel, then launch the test app.  

## Expected behaviour on the Linux app side
- Open sysfs files
- Write hardcoded size in dma_size, thus starting an allocation
- Get the physical adress and the real, page-aligned, buffer size
- Send RPMSG message though **imx_tty_rpmsg** driver
- Sleep for 10 seconds to let the microcontroller do its job
- Once done, liberate the allocated buffer
