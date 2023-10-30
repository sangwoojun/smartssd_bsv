VIVADO := $(XILINX_VIVADO)/bin/vivado

KERNELDIR=./src/kernel/

bsv_kernel: $(KERNELDIR)/*.bsv $(KERNELDIR)/*.v
	cd $(KERNELDIR) && $(MAKE)


$(TEMP_DIR)/kernel.xo: scripts/package_kernel.tcl scripts/gen_xo.tcl bsv_kernel
	mkdir -p $(TEMP_DIR)
	$(VIVADO) -mode batch -source scripts/gen_xo.tcl -tclargs $(TEMP_DIR)/kernel.xo mkKernel $(TARGET) $(PLATFORM) $(XSA)
