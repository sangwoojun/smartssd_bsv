import FIFO::*;
import Vector::*;
import KernelMain::*;


//import "BDPI" function Action bdpi_write_word(Bit#(32) buffer, Bit#(64) addr, Bit#(64) data0, Bit#(64) data1, Bit#(64) data2, Bit#(64) data3, Bit#(64) data4, Bit#(64) data5, Bit#(64) data6, Bit#(64) data7, Bit#(32) tag);
import "BDPI" function Action bdpi_write_word(Bit#(32) buffer, Bit#(64) addr, Bit#(32) data, Bit#(32) tag);
import "BDPI" function Bit#(64) bdpi_read_word(Bit#(32) buffer, Bit#(64) addr);
import "BDPI" function Action bdpi_set_param(Bit#(32) idx, Bit#(32) val);
import "BDPI" function Bit#(32) bdpi_get_param(Bit#(32) idx);
import "BDPI" function Bit#(32) bdpi_check_started();
import "BDPI" function Action bdpi_set_done(Bit#(32) done);

module mkSimTop(Empty);
	KernelMainIfc kernelMain <- mkKernelMain;
	rule checkStart;
		if ( bdpi_check_started != 0 ) kernelMain.start;
	endrule
	rule checkDone;
		if ( kernelMain.done ) begin
			bdpi_set_done(1);
		end
	endrule
	rule relay_param_sync;
		Vector#(ParamCnt, Bit#(32)) params;

		for ( Integer i = 0; i < valueOf(ParamCnt); i=i+1 ) begin
			params[i] = bdpi_get_param(fromInteger(i));
		end
		kernelMain.sync_param(params);
	endrule
	rule relay_param_set;
		Vector#(ParamCnt, Bit#(32)) paramo = kernelMain.update_param;
		for ( Integer i = 0; i < valueOf(ParamCnt); i=i+1 ) begin
			bdpi_set_param(fromInteger(i), paramo[i]);
		end
	endrule
	for ( Integer i = 0; i < valueOf(MemPortCnt); i=i+1 ) begin
		Reg#(Bit#(64)) memReadAddr <- mkReg(0);
		Reg#(Bit#(32)) memReadBytesLeft <- mkReg(0);
		rule relayReadReq00 ( memReadBytesLeft == 0 );
			let r <- kernelMain.mem[i].readReq;
			memReadAddr <= r.addr;
			memReadBytesLeft <= r.bytes;
		endrule

		Reg#(Bit#(64)) memWriteAddr <- mkReg(0);
		Reg#(Bit#(32)) memWriteBytesLeft <- mkReg(-1);
		rule relayWriteReq (memWriteBytesLeft == 0);
			let r <- kernelMain.mem[i].writeReq;
			memWriteAddr <= r.addr;
			memWriteBytesLeft <= r.bytes;
		endrule


		Reg#(Bit#(32)) memWriteTag <- mkReg(0);
		rule relayWriteWord ( memWriteBytesLeft > 0 );
			let r <- kernelMain.mem[i].writeWord;
			for ( Integer j = 0; j < 512/32; j=j+1 ) begin
				Bit#(32) w = truncate(r>>(j*32));
				bdpi_write_word(fromInteger(i), memWriteAddr + fromInteger(j*4), w, memWriteTag);
			end
			/*
			bdpi_write_word(fromInteger(i), memWriteAddr, 
				wvec[0], wvec[1], wvec[2], wvec[3], 
				wvec[4], wvec[5], wvec[6], wvec[7]
				, memWriteTag);
			*/
			memWriteTag <= memWriteTag + 1;
			memWriteAddr <= memWriteAddr + 64;
			if ( memWriteBytesLeft >= 64 ) begin
				memWriteBytesLeft <= memWriteBytesLeft - 64;
			end else begin
				memWriteBytesLeft <= 0;
			end
		endrule
		rule relayReadWord ( memReadBytesLeft > 0 );
			Bit#(512) rword = 0;
			for ( Integer j = 0; j < 512/32; j=j+1 ) begin
				let d = bdpi_read_word(fromInteger(i), memReadAddr+fromInteger(j*4));
				rword = rword | (zeroExtend(d)<<(j*32));
			end
			memReadAddr <= memReadAddr + 64;
			if ( memReadBytesLeft >= 64 ) begin
				memReadBytesLeft <= memReadBytesLeft - 64;
			end else begin
				memReadBytesLeft <= 0;
			end
			kernelMain.mem[i].readWord(rword);
			//$write( "Mem read from %x %x\n", memReadAddr, rword );
		endrule
	end

endmodule	
