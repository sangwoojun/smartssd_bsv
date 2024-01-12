import FIFO::*;
import Vector::*;

typedef 4 ParamCnt;
interface Reg2W1RIfc;
	method Action write1(Bit#(32) data);
	method Action write2(Bit#(32) data);
	method ActionValue#(Maybe#(Bit#(32))) read;
endinterface
interface ParamFileIfc#(numeric type cnt);
	interface Vector#(cnt, Reg2W1RIfc) regs;
	method Action display;
endinterface
module mkParamFile(ParamFileIfc#(cnt));
	Vector#(cnt, Reg#(Bit#(32))) params <- replicateM(mkReg(0));
	Vector#(cnt, Reg#(Bit#(1))) param_write_epoch <- replicateM(mkReg(0));
	Vector#(cnt, Reg#(Bit#(1))) param_read_epoch <- replicateM(mkReg(0));
	
	Vector#(cnt, RWire#(Bit#(32))) write1reqs <- replicateM(mkRWire());
	Vector#(cnt, RWire#(Bit#(32))) write2reqs <- replicateM(mkRWire());

	Vector#(cnt, Reg2W1RIfc) regs_;
	for (Integer i = 0; i < valueOf(cnt); i=i+1) begin
		rule applywrite;
			let a = write1reqs[i].wget;
			let b = write2reqs[i].wget;
			if ( isValid(a) ) begin
				params[i] <= fromMaybe(?,a);
				if ( param_read_epoch[i] == param_write_epoch[i] ) param_write_epoch[i] <= ~param_write_epoch[i];
			end else if ( isValid(b) ) params[i] <= fromMaybe(?,b);
		endrule
		 
		regs_[i] = interface Reg2W1RIfc;
			method Action write1(Bit#(32) data);
				write1reqs[i].wset(data);
			endmethod
			method Action write2(Bit#(32) data);
				write2reqs[i].wset(data);
			endmethod
			method ActionValue#(Maybe#(Bit#(32))) read;
				if ( param_write_epoch[i] == param_read_epoch[i] ) return tagged Invalid;
				else begin
					param_read_epoch[i] <= ~param_read_epoch[i];
					return tagged Valid params[i];
				end
			endmethod
		endinterface;
	end
	interface regs = regs_;
	method Action display;
		$write( "%x %x %x %x\n", params[0], params[1], params[2], params[3] );
	endmethod
endmodule

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

	method Action sync_param(Vector#(ParamCnt, Maybe#(Bit#(32))) data);
	method ActionValue#(Vector#(ParamCnt, Maybe#(Bit#(32)))) update_param;
endinterface

typedef struct {
	Bit#(64) addr;
	Bit#(32) bytes;
} MemPortReq deriving (Eq,Bits);



module mkKernelMain(KernelMainIfc);
	ParamFileIfc#(ParamCnt) params <- mkParamFile;
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
		//$write( "%x\n", d );
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
		//params.display;
		//$display( "%x %x %x %x\n", params.regs[0].read, params.regs[1].read, params.regs[2].read, params.regs[3].read );
	endmethod
	method Bool done;
		return kernelDone;
	endmethod
	interface mem = mem_;
	method Action sync_param(Vector#(ParamCnt, Maybe#(Bit#(32))) data);
		for (Integer i = 0; i < valueOf(ParamCnt); i=i+1) begin
			if ( isValid(data[i]) )params.regs[i].write2(fromMaybe(?,data[i]));
			//$write( ">> %d %x\n", i, data[i] );
		end
	endmethod
	method ActionValue#(Vector#(ParamCnt, Maybe#(Bit#(32)))) update_param if (kernelStarted);
		Vector#(ParamCnt, Maybe#(Bit#(32))) ret;
		for (Integer i = 0; i < valueOf(ParamCnt); i=i+1) begin
			ret[i] <- params.regs[i].read();
		end
		return ret;
	endmethod
endmodule

