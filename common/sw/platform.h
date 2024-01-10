#ifndef __SW_PLATFORM_H__
#define __SW_PLATFORM_H__

#include <vector>
#include <stdint.h>


#define PARAM_COUNT 4
#define BUFFER_COUNT 2

#ifdef SYNTH
// XRT includes
#include "experimental/xrt_bo.h"
#include "experimental/xrt_ip.h"
#include "experimental/xrt_device.h"
#include "experimental/xrt_kernel.h"
#include "experimental/xrt_xclbin.h"


#define IP_START 0x1
#define IP_IDLE 0x4
#define CSR_OFFSET 0x0
// two AXI buffers in arg
#define REGISTER_IDX_START 2

//xrt::xclbin::kernel g_kernel_used;
extern xrt::xclbin::mem g_mem_used;
extern xrt::device g_device;
extern xrt::ip g_ip;
extern std::vector<xrt::xclbin::ip> g_kernel_used;
extern std::vector<xrt::xclbin::arg> g_args;

#include "cmdlineparser.h"
#endif

typedef struct {
#ifdef SYNTH
	xrt::bo device_buffer;
#else
	uint8_t* device_buffer;
#endif
	uint8_t* host_buffer;
	size_t bytes;
} FPGABuffer;


#ifndef SYNTH
class BsimDeviceStatus {
public:
	static BsimDeviceStatus* getInstance();
	void set_param(int idx, uint32_t val);
	uint32_t get_param(int idx);
	void set_buffer(int idx, FPGABuffer* buffer) {buffer_array[idx]=buffer;};
	FPGABuffer* get_buffer(int idx) {return buffer_array[idx];};
	uint32_t read_device_buffer(int idx, size_t offset);
	void write_device_buffer(int idx, size_t offset, uint32_t value);

	void start() {started = true;};
	bool is_started() { return started;};
	bool is_done() {return done;};
	void set_done() {done=true;};
private:
	static BsimDeviceStatus* m_instance;
	BsimDeviceStatus();

	uint32_t param_array[PARAM_COUNT];
	FPGABuffer* buffer_array[BUFFER_COUNT];
	bool started;
	bool done;
};
#endif




FPGABuffer create_buffer(size_t bytes);
void sync_buffer(FPGABuffer buffer, size_t offset, size_t bytes, bool to_fpga);

void set_param(size_t idx, uint32_t value);
uint32_t get_param(size_t idx);

//void write_register32(size_t idx, uint32_t value);
//void write_register64(size_t idx, uint64_t value);

void set_fpga_buffer(size_t idx, FPGABuffer *buffer);

void send_start();
bool check_done();


#endif


