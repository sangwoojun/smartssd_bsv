#include <iostream>
#include <cstring>


#include "platform.h"

#ifndef __BSIM__

xrt::xclbin::mem g_mem_used;
xrt::device g_device;
xrt::ip g_ip;
std::vector<xrt::xclbin::ip> g_kernel_used;
std::vector<xrt::xclbin::arg> g_args;
extern void swmain(void* arg);

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
#endif



FPGABuffer create_buffer(size_t bytes) {
FPGABuffer rbuf;
#ifndef __BSIM__
	rbuf.xrt_buffer = xrt::bo(g_device, bytes, g_mem_used.get_index());
	rbuf.host_buffer = rbuf.xrt_buffer.map<uint32_t*>();
	rbuf.bytes = bytes;
#else
#endif
	return rbuf;
}

void sync_buffer(FPGABuffer buffer, size_t offset, size_t bytes, bool to_fpga) {
#ifndef __BSIM__
	//FIXME
    buffer.xrt_buffer.sync(to_fpga?XCL_BO_SYNC_BO_TO_DEVICE:XCL_BO_SYNC_BO_FROM_DEVICE);
#else
#endif
}

void write_register32(size_t idx, uint32_t value, size_t byte_offset) {
#ifndef __BSIM__
    g_ip.write_register(g_args[REGISTER_IDX_START+idx].get_offset()+byte_offset, value);
#else
#endif
}
void write_register64(size_t idx, uint64_t value) {
	write_register32(idx, (uint32_t)value, 0);
	write_register32(idx, (uint32_t)(value>>32), 4);
}

void set_fpga_buffer(size_t idx, FPGABuffer buffer) {
#ifndef __BSIM__
    g_ip.write_register(g_args[idx].get_offset(), buffer.xrt_buffer.address());
    g_ip.write_register(g_args[idx].get_offset()+4, buffer.xrt_buffer.address()>>32);
#else
#endif
}

void send_start() {
#ifndef __BSIM__
    uint32_t axi_ctrl = 0;

    axi_ctrl = IP_START;
    g_ip.write_register(CSR_OFFSET, axi_ctrl);
#else
#endif
}

bool check_done() {
#ifndef __BSIM__
    uint32_t axi_ctrl = 0;
	axi_ctrl = g_ip.read_register(CSR_OFFSET);
    return ((axi_ctrl & IP_IDLE) == IP_IDLE);
#else
#endif
}
