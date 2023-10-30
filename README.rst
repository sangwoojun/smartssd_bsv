Bluespec Kernel Boilerplate for the Samsung SmartSSD
============================

Environment
-----------
Tested with Ubuntu 20.04 and Vitis 21.2.
Vitis 21.2 is the last version with SmartSSD support, so this won't work with any newer environments.

Vitis platform: xilinx_u2_gen3x4_xdma_gc_2_202110_1

How to build
------------

``make all'' to build for HW
``make cleanall'' to clean everything

Bluesim simulation will be included shortly


Editing the Kernel
------------------
src/kernel/

Notes
-----

This uses the Vitis managed kernel approach, so kernel state is maintained across software executions regardless of IP_START or IP_DONE, etc


