#ifndef __SW_PLATFORM_H__
#define __SW_PLATFORM_H__

#include <vector>

#include <stdint.h>

#ifndef __BSIM__

// XRT includes
#include "experimental/xrt_bo.h"
#include "experimental/xrt_ip.h"
#include "experimental/xrt_device.h"
#include "experimental/xrt_kernel.h"
#include "experimental/xrt_xclbin.h"

#include "cmdlineparser.h"

#define IP_START 0x1
#define IP_IDLE 0x4
#define CSR_OFFSET 0x0

// two AXI buffers in arg
#define REGISTER_IDX_START 2

#endif




//xrt::xclbin::kernel g_kernel_used;
extern xrt::xclbin::mem g_mem_used;
extern xrt::device g_device;
extern xrt::ip g_ip;
extern std::vector<xrt::xclbin::ip> g_kernel_used;
extern std::vector<xrt::xclbin::arg> g_args;

typedef struct {
	xrt::bo xrt_buffer;
	uint32_t* host_buffer;
	size_t bytes;
} FPGABuffer;

FPGABuffer create_buffer(size_t bytes);
void sync_buffer(FPGABuffer buffer, size_t offset, size_t bytes, bool to_fpga);
void write_register32(size_t idx, uint32_t value, size_t byte_offset = 0);
void write_register64(size_t idx, uint64_t value);

void set_fpga_buffer(size_t idx, FPGABuffer buffer);

void send_start();
bool check_done();


#endif

