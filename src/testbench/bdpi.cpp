#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>

#include <queue>

#include "platform.h"


// temporary, 8 buffers of size 32MB
#define MEM_PORT_CNT 2
#define MEM_BUF_SIZE (1024*1024*512)

bool g_initialized = false;
uint32_t* g_buffers[MEM_PORT_CNT];

pthread_t g_swmain_thread;
extern void *swmain(void* arg);

BsimDeviceStatus* g_device;


void init() {
	if ( g_initialized ) return;

	pthread_create(&g_swmain_thread, NULL, swmain, NULL);
	g_initialized = true;
}

extern "C" uint32_t bdpi_read_word(int bufidx, uint64_t addr) {
	init();
	BsimDeviceStatus* device_status = BsimDeviceStatus::getInstance ();
	return device_status->read_device_buffer(bufidx, addr);
}

uint32_t write_tag = 0xffffffff;
extern "C" void bdpi_write_word(int bufidx, uint64_t addr, uint32_t data, uint32_t tag) {
	init();
	if ( write_tag == tag ) return;
	write_tag = tag;

	BsimDeviceStatus* device_status = BsimDeviceStatus::getInstance ();
	device_status->write_device_buffer(bufidx, addr, data);
}
extern "C" void bdpi_set_param(uint32_t idx, uint32_t val) {
	init();
	BsimDeviceStatus* device_status = BsimDeviceStatus::getInstance ();
	device_status->set_param(idx, val);
}
extern "C" uint32_t bdpi_get_param(uint32_t idx) {
	init();
	BsimDeviceStatus* device_status = BsimDeviceStatus::getInstance ();
	return device_status->get_param(idx);
}

extern "C" uint32_t bdpi_check_started() {
	init();
	BsimDeviceStatus* device_status = BsimDeviceStatus::getInstance ();
	return device_status->is_started()?1:0;
}

extern "C" void bdpi_set_done(uint32_t done) {
	init();
	BsimDeviceStatus* device_status = BsimDeviceStatus::getInstance ();
	if ( done != 0 ) device_status->set_done();
}
