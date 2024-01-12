#include <iostream>
#include <cstring>


#include "platform.h"

extern void swmain(void* arg);
uint32_t* g_param_array = {0,};

#ifdef SYNTH
xrt::xclbin::mem g_mem_used;
xrt::device g_device;
xrt::ip g_ip;
std::vector<xrt::xclbin::ip> g_kernel_used;
std::vector<xrt::xclbin::arg> g_args;

int main(int argc, char** argv) {
    // Command Line Parser
    sda::utils::CmdLineParser parser;

    // Switches
    //**************//"<Full Arg>",  "<Short Arg>", "<Description>", "<Default>"
    parser.addSwitch("--xclbin_file", "-x", "input binary file string", "");
    parser.addSwitch("--device_id", "-d", "device index", "0");
    parser.parse(argc, argv);

    // Read settings
    std::string binaryFile = parser.value("xclbin_file");
    int device_index = stoi(parser.value("device_id"));

    if (argc < 3) {
        parser.printHelp();
        return EXIT_FAILURE;
    }

    std::cout << "Open the device" << device_index << std::endl;
    g_device = xrt::device(device_index);
    std::cout << "Load the xclbin " << binaryFile << std::endl;
    auto uuid = g_device.load_xclbin(binaryFile);



    g_ip = xrt::ip(g_device, uuid, "mkKernelTop");
    auto xclbin = xrt::xclbin(binaryFile);
    std::cout << "Fetch compute Units" << std::endl;
    for (auto& kernel : xclbin.get_kernels()) {
        if (kernel.get_name() == "mkKernelTop") {
            g_kernel_used = kernel.get_cus();
        }
    }

	
    if (g_kernel_used.empty()) throw std::runtime_error("IP mkKernelTop not found in the provided xclbin");
    
	std::cout << "Determine memory index\n";
    for (auto& mem : xclbin.get_mems()) {
        if (mem.get_used()) {
			g_mem_used = mem;
			break;
        }
    }
    
	g_args = g_kernel_used[0].get_args();

	swmain(NULL);

}
#else
BsimDeviceStatus* BsimDeviceStatus::m_instance = NULL;
BsimDeviceStatus* BsimDeviceStatus::getInstance() {
	if ( m_instance == NULL ) m_instance = new BsimDeviceStatus();
	return m_instance;
}
BsimDeviceStatus::BsimDeviceStatus() {
	memset(param_array, 0, sizeof(uint32_t)*PARAM_COUNT);
	memset(buffer_array, 0, sizeof(FPGABuffer*)*BUFFER_COUNT);
}
void BsimDeviceStatus::set_param(int idx, uint32_t val) {
	if ( idx < PARAM_COUNT ) {
		param_array[idx] = val;
		param_updated[idx] = true;
	}
}
uint64_t BsimDeviceStatus::get_param(int idx) {
	uint64_t r = 0;
	if ( idx >= PARAM_COUNT ) return r;

	uint64_t v = (uint64_t)param_array[idx];
	r =  (v<<32|(param_updated[idx]?1:0));
	param_updated[idx] = false;
	return r;
};
uint32_t BsimDeviceStatus::read_device_buffer(int idx, size_t offset) {
	if ( idx >= BUFFER_COUNT ) return 0xffffffff;
	if ( buffer_array[idx] == NULL ) return 0xffffffff;
	if ( offset +4 >= buffer_array[idx]->bytes ) return 0xffffffff;

	uint32_t ret = ((uint32_t*)buffer_array[idx]->device_buffer)[offset/sizeof(uint32_t)];
	//printf( "%d %x %x\n", idx, offset, ret );
	return ret;
}
void BsimDeviceStatus::write_device_buffer(int idx, size_t offset, uint32_t val) {
	if ( idx >= BUFFER_COUNT ) return;
	if ( buffer_array[idx] == NULL ) return;
	if ( offset +4 >= buffer_array[idx]->bytes ) return;

	((uint32_t*)buffer_array[idx]->device_buffer)[offset/sizeof(uint32_t)] = val;
}
#endif




FPGABuffer create_buffer(size_t bytes) {
FPGABuffer rbuf;
#ifdef SYNTH
	rbuf.device_buffer = xrt::bo(g_device, bytes, g_mem_used.get_index());
	rbuf.host_buffer = rbuf.device_buffer.map<uint8_t*>();
	rbuf.bytes = bytes;
#else
	rbuf.device_buffer = (uint8_t*)malloc(bytes);
	rbuf.host_buffer = (uint8_t*)malloc(bytes);
	rbuf.bytes = bytes;
#endif
	return rbuf;
}

void sync_buffer(FPGABuffer buffer, size_t offset, size_t bytes, bool to_fpga) {
#ifdef SYNTH
	if ( to_fpga ) buffer.device_buffer.write(buffer.host_buffer, bytes, offset);
	else buffer.device_buffer.read(buffer.host_buffer, bytes, offset);
    //buffer.device_buffer.sync(to_fpga?XCL_BO_SYNC_BO_TO_DEVICE:XCL_BO_SYNC_BO_FROM_DEVICE);
#else
	if ( to_fpga ) memcpy(buffer.device_buffer+offset, buffer.host_buffer+offset, bytes);
	else memcpy(buffer.host_buffer+offset, buffer.device_buffer+offset, bytes);
#endif
}

void set_param(size_t idx, uint32_t value) {
#ifdef SYNTH
    g_ip.write_register(g_args[REGISTER_IDX_START+idx].get_offset(), value);
#else
	BsimDeviceStatus* bsim_device = BsimDeviceStatus::getInstance();
	bsim_device->set_param(idx,value);
#endif
}

uint64_t get_param(size_t idx) {
#ifdef SYNTH
	return g_ip.read_register(g_args[REGISTER_IDX_START+idx].get_offset());
#else
	BsimDeviceStatus* bsim_device = BsimDeviceStatus::getInstance();
	return bsim_device->get_param(idx);
#endif
}

/*
void write_register32(size_t idx, uint32_t value) {
#ifdef SYNTH
    g_ip.write_register(g_args[REGISTER_IDX_START+idx].get_offset(), value);
#else
	BsimDeviceStatus* bsim_device = BsimDeviceStatus::getInstance();
	bsim_device->set_param(idx,value);
#endif
}
*/
/*
void write_register64(size_t idx, uint64_t value) {
	write_register32(idx, (uint32_t)value);
	write_register32(idx +1, (uint32_t)(value>>32));
}
*/

void set_fpga_buffer(size_t idx, FPGABuffer *buffer) {
#ifdef SYNTH
    g_ip.write_register(g_args[idx].get_offset(), buffer->device_buffer.address());
    g_ip.write_register(g_args[idx].get_offset()+4, buffer->device_buffer.address()>>32);
#else
	BsimDeviceStatus* bsim_device = BsimDeviceStatus::getInstance();
	bsim_device->set_buffer(idx,buffer);
	
#endif
}

void send_start() {
#ifdef SYNTH
    uint32_t axi_ctrl = 0;

    axi_ctrl = IP_START;
    g_ip.write_register(CSR_OFFSET, axi_ctrl);
#else
	BsimDeviceStatus* bsim_device = BsimDeviceStatus::getInstance();
	bsim_device->start();
#endif
}

bool check_done() {
#ifdef SYNTH
    uint32_t axi_ctrl = 0;
	axi_ctrl = g_ip.read_register(CSR_OFFSET);
    return ((axi_ctrl & IP_IDLE) == IP_IDLE);
#else
	BsimDeviceStatus* bsim_device = BsimDeviceStatus::getInstance();
	return bsim_device->is_done();
#endif
}
