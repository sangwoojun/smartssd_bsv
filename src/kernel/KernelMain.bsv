import FIFO::*;
import Vector::*;

typedef 4 ParamCnt;

typedef 2 MemPortCnt;

interface MemPortIfc;
	method ActionValue#(MemPortReq) readReq;
	method ActionValue#(MemPortReq) writeReq;
	method ActionValue#(Bit#(512)) writeWord;
	method Action readWord(Bit#(512) word);
endinterface

interface KernelMainIfc;
	method Action start;
	method Bool done;
	interface Vector#(MemPortCnt, MemPortIfc) mem;

	method Action sync_param(Vector#(ParamCnt, Bit#(32)) data);
	method Vector#(ParamCnt, Bit#(32)) update_param;
endinterface

typedef struct {
	Bit#(64) addr;
	Bit#(32) bytes;
} MemPortReq deriving (Eq,Bits);


module mkKernelMain(KernelMainIfc);
	Vector#(ParamCnt, Reg#(Bit#(32))) params <- replicateM(mkReg(0));
	Vector#(MemPortCnt, FIFO#(MemPortReq)) readReqQs <- replicateM(mkFIFO);
	Vector#(MemPortCnt, FIFO#(MemPortReq)) writeReqQs <- replicateM(mkFIFO);
	Vector#(MemPortCnt, FIFO#(Bit#(512))) writeWordQs <- replicateM(mkFIFO);
	Vector#(MemPortCnt, FIFO#(Bit#(512))) readWordQs <- replicateM(mkFIFO);
	Reg#(Bit#(32)) cycleCounter <- mkReg(0);
	rule incCycle;
		cycleCounter <= cycleCounter + 1;
	endrule
	
	Reg#(Bool) kernelStarted <- mkReg(False);
	Reg#(Bool) kernelDone <- mkReg(False);

	//////////////////////////////////////////////////////////////////////////


	Reg#(Bit#(32)) fileReadLeft <- mkReg(256);
	rule sendFileReadReq (kernelStarted && fileReadLeft > 0 );
		fileReadLeft <= fileReadLeft - 1;
		readReqQs[0].enq(MemPortReq{addr:zeroExtend(fileReadLeft)<<6,bytes:64});
	endrule
	Reg#(Bit#(32)) memWriteLeft <- mkReg(256);
	rule sendMemWriteReq(kernelStarted && memWriteLeft > 0);
		memWriteLeft <= memWriteLeft - 1;
		writeReqQs[1].enq(MemPortReq{addr:zeroExtend(memWriteLeft)<<6, bytes:64});
	endrule

	Reg#(Bit#(32)) writeWordCnt <- mkReg(0);
	rule relayMemData;
		let d = readWordQs[0].first;
		readWordQs[0].deq;
		writeWordQs[1].enq(d);
		writeWordCnt <= writeWordCnt + 1;
		if ( writeWordCnt == 255 ) kernelDone <= True;
	endrule
	

	//////////////////////////////////////////////////////////////////////////



	
	Vector#(MemPortCnt, MemPortIfc) mem_;
	for (Integer i = 0; i < valueOf(MemPortCnt); i=i+1) begin
		mem_[i] = interface MemPortIfc;
			method ActionValue#(MemPortReq) readReq;
				readReqQs[i].deq;
				return readReqQs[i].first;
			endmethod
			method ActionValue#(MemPortReq) writeReq;
				writeReqQs[i].deq;
				return writeReqQs[i].first;
			endmethod
			method ActionValue#(Bit#(512)) writeWord;
				writeWordQs[i].deq;
				return writeWordQs[i].first;
			endmethod
			method Action readWord(Bit#(512) word);
				readWordQs[i].enq(word);
			endmethod
		endinterface;
	end
	method Action start;
		kernelStarted <= True;
		//$display( "%x %x %x %x\n", params[0], params[1], params[2], params[3] );
	endmethod
	method Bool done;
		return kernelDone;
	endmethod
	interface mem = mem_;
	method Action sync_param(Vector#(ParamCnt, Bit#(32)) data);
		for (Integer i = 0; i < valueOf(ParamCnt); i=i+1) begin
			params[i] <= data[i];
			//$write( ">> %d %x\n", i, data[i] );
		end
	endmethod
	method Vector#(ParamCnt, Bit#(32)) update_param if (kernelStarted);
		Vector#(ParamCnt, Bit#(32)) ret;
		for (Integer i = 0; i < valueOf(ParamCnt); i=i+1) begin
			ret[i] = params[i];
		end
		return ret;
	endmethod
endmodule

