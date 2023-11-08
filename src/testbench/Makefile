ROOTDIR=../../
BSVPATH=$(ROOTDIR)/src/kernel/

BSIM_DIR=./bsim/
BSIM_CPPFILES=bdpi.cpp $(wildcard $(ROOTDIR)/common/sw/*.cpp)

BSCFLAGS = -show-schedule -aggressive-conditions  -print-flags
BSCFLAGS_BSIM = -bdir $(BSIM_DIR) -vdir $(BSIM_DIR) -simdir $(BSIM_DIR) -info-dir $(BSIM_DIR) -fdir $(BSIM_DIR) -l pthread -I $(ROOTDIR)/common/sw/ -D __BSIM__

bsim: $(wildcard ./$(ROOTDIR)/src/kernel/*.bsv) $(wildcard ./*.bsv) Makefile

	mkdir -p $(BSIM_DIR)
	bsc $(BSCFLAGS) $(BSCFLAGS_BSIM) -p +:$(BSVPATH) -sim -u -g mkSimTop SimTop.bsv 
	bsc $(BSCFLAGS) $(BSCFLAGS_BSIM) -sim -e mkSimTop -o $(BSIM_DIR)/bsim $(BSIM_DIR)/*.ba $(BSIM_CPPFILES)

runsim:
	./bsim/bsim | tee -a output.log

clean:
	rm -rf bsim
	rm -rf bdpi.o
	rm -rf output.log
