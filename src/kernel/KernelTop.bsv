import Axi4LiteControllerXrt::*;
import Axi4MemoryMaster::*;

import Vector::*;
import Clocks :: *;

import KernelMain::*;

interface KernelTopIfc;
	(* always_ready *)
	interface Axi4MemoryMasterPinsIfc#(64,512) m00_axi;
	(* always_ready *)
	interface Axi4MemoryMasterPinsIfc#(64,512) m01_axi;
	(* always_ready *)
	interface Axi4LiteControllerXrtPinsIfc#(12,32) s_axi_control;
	(* always_ready *)
	method Bool interrupt;
endinterface

(* synthesize *)
(* default_reset="ap_rst_n", default_clock_osc="ap_clk" *)
module mkKernelTop (KernelTopIfc);
	Clock defaultClock <- exposeCurrentClock;
	Reset defaultReset <- exposeCurrentReset;

	Axi4LiteControllerXrtIfc#(12,32) axi4control <- mkAxi4LiteControllerXrt(defaultClock, defaultReset);
	Vector#(2, Axi4MemoryMasterIfc#(64,512)) axi4mem <- replicateM(mkAxi4MemoryMaster);
	//Axi4MemoryMasterIfc#(64,512) axi4file <- mkAxi4MemoryMaster;

	Reg#(Bit#(32)) cycleCounter <- mkReg(0);
	rule incCycle;
		cycleCounter <= cycleCounter + 1;
	endrule

	Reg#(Bool) started <- mkReg(False);

	KernelMainIfc kernelMain <- mkKernelMain;
	rule checkStart (!started);
		if ( axi4control.ap_start ) begin
			kernelMain.start;
			started <= True;
		end
	endrule
	Reg#(Bool) donesent <- mkReg(False);
	rule checkStatus;
		Bool done = kernelMain.done;
		axi4control.ap_done((started&&done));
		axi4control.ap_idle((!started)||done);
	endrule
	for ( Integer i = 0; i < valueOf(MemPortCnt); i=i+1 ) begin
		rule relayReadReq00 ( started);
			let r <- kernelMain.mem[i].readReq;
			if ( i == 0 ) axi4mem[i].readReq(axi4control.mem_addr+r.addr,zeroExtend(r.bytes));
			else axi4mem[i].readReq(axi4control.file_addr+r.addr,zeroExtend(r.bytes));
		endrule
		rule relayWriteReq ( started);
			let r <- kernelMain.mem[i].writeReq;
			if ( i == 0 ) axi4mem[i].writeReq(axi4control.mem_addr+r.addr,zeroExtend(r.bytes));
			else axi4mem[i].writeReq(axi4control.file_addr+r.addr,zeroExtend(r.bytes));
		endrule
		rule relayWriteWord ( started);
			let r <- kernelMain.mem[i].writeWord;
			axi4mem[i].write(r);
		endrule
		rule relayReadWord ( started);
			let d <- axi4mem[i].read;
			kernelMain.mem[i].readWord(d);
		endrule
	end
	rule relay_param_set;
		Vector#(ParamCnt, Maybe#(Bit#(32))) paramo <- kernelMain.update_param;
		if ( isValid(paramo[0]) ) axi4control.scalar00_w(fromMaybe(?,paramo[0]));
		if ( isValid(paramo[1]) ) axi4control.scalar01_w(fromMaybe(?,paramo[1]));
		if ( isValid(paramo[2]) ) axi4control.scalar02_w(fromMaybe(?,paramo[2]));
		if ( isValid(paramo[3]) ) axi4control.scalar03_w(fromMaybe(?,paramo[3]));
	endrule
	Vector#(ParamCnt, Reg#(Bit#(32))) lastepochs <- replicateM(mkReg(-1));
	rule relay_param_sync;
		Vector#(ParamCnt, Maybe#(Bit#(32))) params;
		Vector#(ParamCnt, Bit#(32)) scalars;
		Vector#(ParamCnt, Bit#(32)) epochs;
		scalars[0] = axi4control.scalar00;
		epochs[0] = axi4control.scalar00_epoch;
		scalars[1] = axi4control.scalar01;
		epochs[1] = axi4control.scalar01_epoch;
		scalars[2] = axi4control.scalar02;
		epochs[2] = axi4control.scalar02_epoch;
		scalars[3] = axi4control.scalar03;
		epochs[3] = axi4control.scalar03_epoch;

		
		for ( Integer i = 0; i < valueOf(ParamCnt); i=i+1 ) begin
			if ( lastepochs[i] != epochs[i] ) begin
				lastepochs[i] <= epochs[i];
				params[i] = tagged Valid scalars[0];
			end else params[i] = tagged Invalid;
		end
		kernelMain.sync_param(params);
	endrule



	interface m00_axi = axi4mem[0].pins;
	interface m01_axi = axi4mem[1].pins;
	interface s_axi_control = axi4control.pins;
	interface interrupt = axi4control.interrupt;
endmodule
