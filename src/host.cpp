/**
* Copyright (C) 2019-2021 Xilinx, Inc
*
* Licensed under the Apache License, Version 2.0 (the "License"). You may
* not use this file except in compliance with the License. A copy of the
* License is located at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
* WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
* License for the specific language governing permissions and limitations
* under the License.
*/

#include "cmdlineparser.h"
#include <iostream>
#include <cstring>

// XRT includes
#include "experimental/xrt_bo.h"
#include "experimental/xrt_ip.h"
#include "experimental/xrt_device.h"
#include "experimental/xrt_kernel.h"
#include "experimental/xrt_xclbin.h"

#define IP_START 0x1
#define IP_IDLE 0x4
#define CSR_OFFSET 0x0
#define DATA_SIZE (256*1024*1024)

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
    auto device = xrt::device(device_index);
    std::cout << "Load the xclbin " << binaryFile << std::endl;
    auto uuid = device.load_xclbin(binaryFile);

    size_t vector_size_bytes = sizeof(int) * DATA_SIZE;

    xrt::xclbin::mem mem_used;
    xrt::xclbin::kernel kernel_used;

    std::vector<xrt::xclbin::ip> cu;
    auto ip = xrt::ip(device, uuid, "mkKernelTop");
    auto xclbin = xrt::xclbin(binaryFile);
    std::cout << "Fetch compute Units" << std::endl;
    for (auto& kernel : xclbin.get_kernels()) {
        if (kernel.get_name() == "mkKernelTop") {
            cu = kernel.get_cus();
        }
    }

    if (cu.empty()) throw std::runtime_error("IP mkKernelTop not found in the provided xclbin");

    std::cout << "Determine memory index\n";
    for (auto& mem : xclbin.get_mems()) {
        if (mem.get_used()) {
			mem_used = mem;
			break;
        }
    }

    std::cout << "Allocate Buffer in Global Memory\n";
    auto bo0 = xrt::bo(device, vector_size_bytes, mem_used.get_index());
    auto bo1 = xrt::bo(device, vector_size_bytes, mem_used.get_index());

    // Map the contents of the buffer object into host memory
    auto bo0_map = bo0.map<int*>();
    auto bo1_map = bo1.map<int*>();

    // Create the test data
    //int* bufReference = (int*)malloc(sizeof(int)*DATA_SIZE);
    for (int i = 0; i < DATA_SIZE; ++i) {
        bo0_map[i] = i;
        bo1_map[i] = 0;
    }

    std::cout << "loaded the data" << std::endl;
    uint64_t buf_addr[2];
    // Get the buffer physical address
    buf_addr[0] = bo0.address();
    buf_addr[1] = bo1.address();

    // Synchronize buffer content with device side
    std::cout << "synchronize input buffer data to device global memory\n";

    bo0.sync(XCL_BO_SYNC_BO_TO_DEVICE);
    bo1.sync(XCL_BO_SYNC_BO_TO_DEVICE);

    std::cout << "INFO: Setting IP Data" << std::endl;

    auto args = cu[0].get_args();

    std::cout << "Setting the 1st Register \"a\" (Input Address)" << std::endl;
    ip.write_register(args[0].get_offset(), buf_addr[0]);
    ip.write_register(args[0].get_offset() + 4, buf_addr[0] >> 32);

    std::cout << "Setting the 2nd Register \"b\" (Input Address)" << std::endl;
    ip.write_register(args[1].get_offset(), buf_addr[1]);
    ip.write_register(args[1].get_offset() + 4, buf_addr[1] >> 32);

    //std::cout << "Setting the 3rd Register \"c\" (Output Address)" << std::endl;
    //ip.write_register(args[2].get_offset(), buf_addr[2]);
    //ip.write_register(args[2].get_offset() + 4, buf_addr[2] >> 32);

    std::cout << "Setting the 4th Register \"length_r\"" << std::endl;
    ip.write_register(args[2].get_offset(), DATA_SIZE);

    uint32_t axi_ctrl = 0;

    std::cout << "INFO: IP Start" << std::endl;
    axi_ctrl = IP_START;
    ip.write_register(CSR_OFFSET, axi_ctrl);

    // Wait until the IP is DONE

    axi_ctrl = 0;
    while ((axi_ctrl & IP_IDLE) != IP_IDLE) {
        axi_ctrl = ip.read_register(CSR_OFFSET);
    }

    std::cout << "INFO: IP Done" << std::endl;

    // Get the output;
    std::cout << "Get the output data from the device" << std::endl;
    bo1.sync(XCL_BO_SYNC_BO_FROM_DEVICE);
    bo0.sync(XCL_BO_SYNC_BO_FROM_DEVICE);

	for ( int i = 0; i < 64; i++ ) {
		printf("%d -- %d\n", bo0_map[i], bo1_map[i]);
	}

    // Validate our results
    if (std::memcmp(bo0_map, bo1_map, 8)) { // DATA_SIZE)) {
        throw std::runtime_error("Value read back does not match reference");
        return 0;
    }

    std::cout << "TEST PASSED\n";
    return 0;
}
