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

#include <iostream>
#include <cstring>


#include "platform.h"

#define DATA_SIZE (256*1024*1024)

//int main(int argc, char** argv) {
void* swmain(void* arg) {
	size_t vector_size_bytes = sizeof(int) * DATA_SIZE;

    std::cout << "Allocate Buffer in Global Memory\n";
	FPGABuffer b0 = create_buffer(vector_size_bytes);
	FPGABuffer b1 = create_buffer(vector_size_bytes);

    for (int i = 0; i < DATA_SIZE; ++i) {
        b0.host_buffer[i] = i;
        b1.host_buffer[i] = 0;
    }

    std::cout << "loaded the data" << std::endl;

    // Synchronize buffer content with device side
    std::cout << "synchronize input buffer data to device global memory\n";

    sync_buffer(b0, 0, b0.bytes, true);
    sync_buffer(b1, 0, b0.bytes, true);

    std::cout << "INFO: Setting IP Data" << std::endl;


    std::cout << "Setting the FPGA buffers" << std::endl;
	set_fpga_buffer(0, &b0);
	set_fpga_buffer(1, &b1);


    std::cout << "Setting the parameter registers" << std::endl;
	set_param(0, DATA_SIZE);

    std::cout << "INFO: IP Start" << std::endl;
	send_start();
	while (!check_done()) ;


    std::cout << "INFO: IP Done" << std::endl;

    // Get the output;
    std::cout << "Get the output data from the device" << std::endl;
	sync_buffer(b0, 0, b0.bytes, false);
	sync_buffer(b1, 0, b1.bytes, false);

	for ( int i = 0; i < 64; i++ ) {
		printf("%d -- %d\n", b0.host_buffer[i], b1.host_buffer[i]);
	}

//    // Validate our results
//    if (std::memcmp(bo0_map, bo1_map, 8)) { // DATA_SIZE)) {
//        throw std::runtime_error("Value read back does not match reference");
//        return;
//    }

    std::cout << "DONE\n";
    return NULL;
}
