import FIFO::*;
import Vector::*;

typedef 2 MemPortCnt;

interface MemPortIfc;
	method ActionValue#(MemPortReq) readReq;
	method ActionValue#(MemPortReq) writeReq;
	method ActionValue#(Bit#(512)) writeWord;
	method Action readWord(Bit#(512) word);
endinterface

interface KernelMainIfc;
	method Action start(Bit#(32) param);
	method Bool done;
	interface Vector#(MemPortCnt, MemPortIfc) mem;
endinterface

typedef struct {
	Bit#(64) addr;
	Bit#(32) bytes;
} MemPortReq deriving (Eq,Bits);


module mkKernelMain(KernelMainIfc);
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
	method Action start(Bit#(32) param);
		kernelStarted <= True;
	endmethod
	method Bool done;
		return kernelDone;
	endmethod
	interface mem = mem_;
endmodule

