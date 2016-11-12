#include "machine.h"
#include "alias.h"
#include "syscall.h"
//#include "param.h"
#include <stdio.h>

char *regTable[32] = {"zr", "ra", "sp", "gp", "tp", "t0", "t1", "t2", "s0",
	 "s1", "a0", "a1", "a2", "a3", "a4", "a5", "a6", "a7", "s2", "s3", "s4", "s5",
	 "s6", "s7", "s8", "s9", "s10", "s11", "t3", "t4", "t5", "t6"};

char *floatTable[32] = {"ft0", "ft1", "ft2", "ft3", "ft4", "ft5", "ft6", "ft7",
	"fs0", "fs1", "fa0", "fa1", "fa2", "fa3", "fa4", "fa5", "fa6", "fa7", "fs2", 
	"fs3", "fs4", "fs5", "fs6", "fs7", "fs8", "fs9", "fs10", "fs11", "ft8",
	"ft9", "ft10", "ft11"};


Machine::Machine(char *filename)
{
	registerFile = new RegisterFile();
	memory = new Memory(filename);
	instruction = NULL;
	registerFile->setPC(memory->entry);
	registerFile->setInteger(SP, memory->inisp);
	state = Running;
	verbose = false;
	debug = false;
}

Machine::~Machine()
{

}

void Machine::setPC(lint newPC)
{
	registerFile->setPC(newPC);
}

void Machine::Run()
{
	while(state == Running)
	{
		//printf("Run: %lx\n", registerFile->getInteger(8));
		if(debug) PrintReg();
		Fetch();
		Decode();
		Execute();
		MemoryAccess();
		WriteBack();
		registerFile->setPC(registerFile->getPC()+4);
		//if(verbose) printf("end ins\n");
	}
}

void Machine::Fetch()
{
	if(instruction != NULL)
	{
		delete instruction;
	}
	lint address = registerFile->getPC();
	//registerFile->setPC((lint)address + 4);
	unsigned int *ins = (unsigned int*)memory->Load(address, 4);
	instruction = new Instruction(*ins);
	//printf("ins: %u\n", instruction->opcode);
	//printf("Fetch: %lu\n", registerFile->getInteger(8));
	delete ins;
}

void Machine::Decode()
{
	lint imms, immu;
	switch(instruction->opcode)
	{
	case OP_IMM:
		memFun = MNull;
		writeFun = WWrite;
		writeSize = WWords;
		rs1 = instruction->rs1;
		val1 = registerFile->getInteger(rs1);
		rd = instruction->rd;
		aluSrc = Imm;
		imms = (long long)((int)instruction->content >> 20);
		immu = (instruction->content >> 20);
		switch(instruction->funct3)
		{
		case ADDI:
			aluFun = Add;
			imm = imms;
			if(verbose) printf("addi $%s $%s %ld\n", regTable[rd], regTable[rs1], imm);
			break;
		case SLTI:
			aluFun = Less;
			imm = imms;
			if(verbose) printf("slti $%s $%s %ld\n", regTable[rd], regTable[rs1], imm);
			break;
		case SLTIU:
			aluFun = Lessu;
			imm = immu;
			if(verbose) printf("sltiu $%s $%s %lu\n", regTable[rd], regTable[rs1], imm);
			break;
		case ANDI:
			aluFun = And;
			imm = immu;
			if(verbose) printf("andi $%s $%s %lu\n", regTable[rd], regTable[rs1], imm);
			break;
		case ORI:
			aluFun = Or;
			imm = immu;
			if(verbose) printf("ori $%s $%s %lu\n", regTable[rd], regTable[rs1], imm);
			break;
		case XORI:
			aluFun = Xor;
			imm = immu;
			if(verbose) printf("xori $%s $%s %lu\n", regTable[rd], regTable[rs1], imm);
			break;
		case 1:
			switch(((instruction->content) >> 26))
			{
			case SLLI:
				aluFun = Shl;
				shamt = (((instruction->content) >> 20) & 63);
				if(verbose) printf("slli $%s $%s %u\n", regTable[rd], regTable[rs1], shamt);
				break;
			default:
				BadCode();
				break;
			}
			break;
		case 5:
			switch(((instruction->content) >> 26))
			{
			case SRLI:
				aluFun = Shrl;
				shamt = (((instruction->content) >> 20) & 63);
				if(verbose) printf("srli $%s $%s %u\n", regTable[rd], regTable[rs1], shamt);
				break;
			case SRAI:
				aluFun = Shra;
				shamt = (((instruction->content) >> 20) & 63);
				if(verbose) printf("srai $%s $%s %u\n", regTable[rd], regTable[rs1], shamt);
				break;
			default:
				BadCode();
				break;
			}
			break;
		default:
			BadCode();
			break;
		}
		break;
	case LUI:
		memFun = MNull;
		writeFun = WWrite;
		writeSize = WWords;
		aluFun = ANull;
		imm = (instruction->content & (~((1 << 12) - 1)));
		imm = (long long)((int)((unsigned int)imm));
		vald = imm;
		rd = instruction->rd;
		if(verbose) printf("lui $%s %lu\n", regTable[rd], imm);
		break;
	case AUIPC:
		memFun = MNull;
		writeFun = WWrite;
		writeSize = WWords;
		aluFun = Add;
		imm = (instruction->content & (~((1 << 12) - 1)));
		imm = (long long)((int)((unsigned int)imm));
		rd = instruction->rd;
		aluSrc = Imm;
		val1 = registerFile->getPC();
		if(verbose) printf("auipc $%s %lu\n", regTable[rd], imm);
		break;
	case OP:
		memFun = MNull;
		writeFun = WWrite;
		writeSize = WWords;
		rs1 = instruction->rs1;
		val1 = registerFile->getInteger(rs1);
		rs2 = instruction->rs2;
		val2 = registerFile->getInteger(rs2);
		rd = instruction->rd;
		aluSrc = Rs2;
		switch(instruction->funct3)
		{
		case 0:
			switch(instruction->funct7)
			{
			case ADD:
				aluFun = Add;
				if(verbose) printf("add $%s $%s $%s\n", regTable[rd], regTable[rs1], regTable[rs2]);
				break;
			case SUB:
				aluFun = Sub;
				if(verbose) printf("sub $%s $%s $%s\n", regTable[rd], regTable[rs1], regTable[rs2]);
				break;
			case MUL:
				aluFun = Mul;
				if(verbose) printf("mul $%s $%s $%s\n", regTable[rd], regTable[rs1], regTable[rs2]);
				break;
			default:
				BadCode();
				break;
			}
			break;
		case 1:
			switch(instruction->funct7)
			{
			case SLL:
				aluFun = Shl;
				shamt = (val2 & 63);
				if(verbose) printf("sll $%s $%s $%s\n", regTable[rd], regTable[rs1], regTable[rs2]);
				break;
			case MULH:
				aluFun = Mulh;
				if(verbose) printf("mulh $%s $%s $%s\n", regTable[rd], regTable[rs1], regTable[rs2]);
				break;
			default:
				BadCode();
				break;
			}
			break;
		case 2:
			switch(instruction->funct7)
			{
			case SLT:
				aluFun = Less;
				if(verbose) printf("slt $%s $%s $%s\n", regTable[rd], regTable[rs1], regTable[rs2]);
				break;
			case MULHSU:
				aluFun = Mulhsu;
				if(verbose) printf("mulhsu $%s $%s $%s\n", regTable[rd], regTable[rs1], regTable[rs2]);
				break;
			default:
				BadCode();
				break;
			}
			break;
		case 3:
			switch(instruction->funct7)
			{
			case SLTU:
				aluFun = Lessu;
				if(verbose) printf("sltu $%s $%s $%s\n", regTable[rd], regTable[rs1], regTable[rs2]);
				break;
			case MULHU:
				aluFun = Mulhu;
				if(verbose) printf("mulhu $%s $%s $%s\n", regTable[rd], regTable[rs1], regTable[rs2]);
				break;
			default:
				BadCode();
				break;
			}
			break;
		case 4:
			switch(instruction->funct7)
			{
			case XOR:
				aluFun = Xor;
				if(verbose) printf("xor $%s $%s $%s\n", regTable[rd], regTable[rs1], regTable[rs2]);
				break;
			case DIV:
				aluFun = Div;
				if(verbose) printf("div $%s $%s $%s\n", regTable[rd], regTable[rs1], regTable[rs2]);
				break;
			default:
				BadCode();
				break;
			}
			break;
		case 5:
			switch(instruction->funct7)
			{
			case SRL:
				aluFun = Shrl;
				shamt = (val2 & 63);
				if(verbose) printf("srl $%s $%s $%s\n", regTable[rd], regTable[rs1], regTable[rs2]);
				break;
			case SRA:
				aluFun = Shra;
				shamt = (val2 & 63);
				if(verbose) printf("sra $%s $%s $%s\n", regTable[rd], regTable[rs1], regTable[rs2]);
				break;
			case DIVU:
				aluFun = Divu;
				if(verbose) printf("divu $%s $%s $%s\n", regTable[rd], regTable[rs1], regTable[rs2]);
				break;
			default:
				BadCode();
				break;
			}
			break;
		case 7:
			switch(instruction->funct7)
			{
			case AND:
				aluFun = And;
				if(verbose) printf("and $%s $%s $%s\n", regTable[rd], regTable[rs1], regTable[rs2]);
				break;
			case REMU:
				aluFun = Remu;
				if(verbose) printf("remu $%s $%s $%s\n", regTable[rd], regTable[rs1], regTable[rs2]);
				break;
			default:
				BadCode();
				break;
			}
			break;
		case 6:
			switch(instruction->funct7)
			{
			case OR:
				aluFun = Or;
				if(verbose) printf("or $%s $%s $%s\n", regTable[rd], regTable[rs1], regTable[rs2]);
				break;
			case REM:
				aluFun = Rem;
				if(verbose) printf("rem $%s $%s $%s\n", regTable[rd], regTable[rs1], regTable[rs2]);
				break;
			default:
				BadCode();
				break;
			}
			break;
		default:
			BadCode();
			break;
		}
		break;
	case JAL:
		memFun = MNull;
		writeFun = WWrite;
		writeSize = WWords;
		aluFun = Add;
		aluSrc = Val4;
		val1 = registerFile->getPC();
		{lint content = instruction->content;
		imm = 0;
		imm += ((content >> 20) & ((1 << 11) - 2));
		imm += ((content >> 9) & (1 << 11));
		imm += (content & ((1 << 20) - (1 << 12)));
		imm += ((content >> 11) & (1 << 20));}
		imm = ((long long)imm << 43 >> 43);
		rd = instruction->rd;
		registerFile->setPC(val1 + imm - 4);
		if(verbose) printf("jal %ld $%s\n", imm, regTable[rd]);
		break;
	case JALR:
		memFun = MNull;
		writeFun = WWrite;
		writeSize = WWords;
		aluFun = Add;
		aluSrc = Val4;
		val1 = registerFile->getPC();
		imm = (long long)((int)instruction->content >> 20);
		rd = instruction->rd;
		rs1 = instruction->rs1;
		{lint address = imm + registerFile->getInteger(instruction->rs1);
		//address = (address & (~1));
		address = (address >> 1 << 1);
		registerFile->setPC(address - 4);}
		if(verbose) printf("jalr $%s %ld\n", regTable[rs1], imm);
		break;
	case BRANCH:
		memFun = MNull;
		writeFun = WNull;
		aluFun = ANull;
		imm = 0;
		{lint content = instruction->content;
		imm += ((content >> 7) & ((1 << 5) - 2));
		imm += ((content >> 20) & ((1 << 11) - (1 << 5)));
		imm += ((content << 4) & (1 << 11));
		imm += ((content >> 19) & (1 << 12));}
		imm = ((long long)imm << 51 >> 51);
		{lint nowPC = registerFile->getPC();
		rs1 = instruction->rs1;
		rs2 = instruction->rs2;
		val1 = registerFile->getInteger(instruction->rs1);
		val2 = registerFile->getInteger(instruction->rs2);
		switch(instruction->funct3)
		{
		case BEQ:
			if(val1 == val2)
			{
				registerFile->setPC(imm + nowPC - 4);
			}
			if(verbose) printf("beq $%s $%s %ld\n", regTable[rs1], regTable[rs2], imm);
			break;
		case BNE:
			if(val1 != val2)
			{
				registerFile->setPC(imm + nowPC - 4);
			}
			if(verbose) printf("bne $%s $%s %ld\n", regTable[rs1], regTable[rs2], imm);
			break;
		case BLT:
			if((long long)val1 < (long long)val2)
			{
				registerFile->setPC(imm + nowPC - 4);
			}
			if(verbose) printf("blt $%s $%s %ld\n", regTable[rs1], regTable[rs2], imm);
			break;
		case BLTU:
			if(val1 < val2)
			{
				//printf("bltu: %lu %lu\n", val1, val2);
				registerFile->setPC(imm + nowPC - 4);
			}
			if(verbose) printf("bltu $%s $%s %ld\n", regTable[rs1], regTable[rs2], imm);
			break;
		case BGE:
			if((long long)val1 >= (long long)val2)
			{
				registerFile->setPC(imm + nowPC - 4);
			}
			if(verbose) printf("bge $%s $%s %ld\n", regTable[rs1], regTable[rs2], imm);
			break;
		case BGEU:
			if(val1 >= val2)
			{
				registerFile->setPC(imm + nowPC - 4);
			}
			if(verbose) printf("bgeu $%s $%s %ld\n", regTable[rs1], regTable[rs2], imm);
			break;
		default:
			BadCode();
			break;
		}}
		break;
	case LOAD:
		//printf("LOAD: %lu\n", registerFile->getInteger(8));
		memFun = Read;
		writeFun = WWrite;
		aluFun = Add;
		aluSrc = Imm;
		imm = (long long)((int)instruction->content >> 20);
		rs1 = instruction->rs1;
		val1 = registerFile->getInteger(rs1);
		rd = instruction->rd;
		
		switch(instruction->funct3)
		{
		case LW:
			memSize = MWords;
			writeSize = WWords;
			if(verbose) printf("lw $%s %ld($%s)\n", regTable[rd], imm, regTable[rs1]);
			break;
		case LH:
			memSize = MHalfWords;
			writeSize = WHalfWords;
			if(verbose) printf("lh $%s %ld($%s)\n", regTable[rd], imm, regTable[rs1]);
			break;
		case LB:
			memSize = MBytes;
			writeSize = WBytes;
			if(verbose) printf("lb $%s %ld($%s)\n", regTable[rd], imm, regTable[rs1]);
			break;
		case LD:
			memSize = MDouble;
			writeSize = WDouble;
			if(verbose) printf("ld $%s %ld($%s)\n", regTable[rd], imm, regTable[rs1]);
			break;
		case LWU:
			memSize = MWordsu;
			writeSize = WWords;
			if(verbose) printf("lwu $%s %ld($%s)\n", regTable[rd], imm, regTable[rs1]);
			break;
		case LHU:
			memSize = MHalfWordsu;
			writeSize = WHalfWords;
			if(verbose) printf("lhu $%s %ld($%s)\n", regTable[rd], imm, regTable[rs1]);
			break;
		case LBU:
			memSize = MBytesu;
			writeSize = WBytes;
			if(verbose) printf("lbu $%s %ld($%s)\n", regTable[rd], imm, regTable[rs1]);
			break;
		default:
			BadCode();
			break;
		}
		break;
	case STORE:
		memFun = MWrite;
		writeFun = WNull;
		aluFun = Add;
		aluSrc = Imm;
		//imm = (long long)((int)instruction->content >> 20);
		//imm = (imm & (~((1 << 5) - 1)));
		//imm += ((instruction->content >> 7) & ((1 << 5) - 1));
		{
			lint content = instruction->content;
			imm = 0;
			imm += ((content >> 7) & ((1 << 5) - 1));
			imm += ((content >> 20) & ((1 << 12) - (1 << 5)));
			imm = ((long long)imm << 52 >> 52);
		}
		
		rs1 = instruction->rs1;
		val1 = registerFile->getInteger(rs1);
		rs2 = instruction->rs2;
		val2 = registerFile->getInteger(rs2);
		//printf("STORE: %u\n", instruction->funct3);
		switch(instruction->funct3)
		{
		case SW:
			memSize = MWords;
			if(verbose) printf("sw $%s %ld($%s)\n", regTable[rs2], imm, regTable[rs1]);
			break;
		case SH:
			memSize = MHalfWords;
			if(verbose) printf("sh $%s %ld($%s)\n", regTable[rs2], imm, regTable[rs1]);
			break;
		case SB:
			memSize = MBytes;
			if(verbose) printf("sb $%s %ld($%s)\n", regTable[rs2], imm, regTable[rs1]);
			break;
		case SD:
			memSize = MDouble;
			if(verbose) printf("sd $%s %ld($%s)\n", regTable[rs2], imm, regTable[rs1]);
			break;
		default:
			BadCode();
			break;
		}
		break;
	case OP_IMM_32:
		memFun = MNull;
		writeFun = WWrite;
		writeSize = WHalfWords;
		aluSrc = Imm;
		rs1 = instruction->rs1;
		val1 = registerFile->getInteger(rs1);
		val1 = (unsigned int)val1;
		rd = instruction->rd;
		imm = (long long)(((int)instruction->content >> 20));
		switch(instruction->funct3)
		{
		case ADDIW:
			aluFun = Add;
			val1 = ((long long)((int)((unsigned int)val1)));
			if(verbose) printf("addiw $%s $%s %ld\n", regTable[rd], regTable[rs1], imm);
			break;
		case 1:
			switch(instruction->funct7)
			{
			case SLLIW:
				aluFun = Shlw;
				shamt = ((instruction->content >> 20) & 31);
				if(verbose) printf("slliw $%s $%s %lu\n", regTable[rd], regTable[rs1], shamt);
				break;
			default:
				BadCode();
				break;
			}
			break;
		case 5:
			switch(instruction->funct7)
			{
			case SRLIW:
				aluFun = Shrlw;
				shamt = ((instruction->content >> 20) & 31);
				if(verbose) printf("srliw $%s $%s %lu\n", regTable[rd], regTable[rs1], shamt);
				break;
			case SRAIW:
				aluFun = Shraw;
				shamt = ((instruction->content >> 20) & 31);
				if(verbose) printf("sraiw $%s $%s %lu\n", regTable[rd], regTable[rs1], shamt);
				break;
			default:
				BadCode();
				break;
			}
			break;
		default:
			BadCode();
			break;
		}
		break;
	case OP_32:
		memFun = MNull;
		writeFun = WWrite;
		writeSize = WHalfWords;
		rs1 = instruction->rs1;
		rs2 = instruction->rs2;
		rd = instruction->rd;
		val1 = registerFile->getInteger(rs1);
		val2 = registerFile->getInteger(rs2);
		val1 = (unsigned int)val1;
		val2 = (unsigned int)val2;
		aluSrc = Rs2;
		switch(instruction->funct3)
		{
		case 0:
			switch(instruction->funct7)
			{
			case ADDW:
				aluFun = Add;
				val1 = ((long long)((int)((unsigned int)val1)));
				val2 = ((long long)((int)((unsigned int)val2)));
				if(verbose) printf("addw $%s $%s $%s\n", regTable[rd], regTable[rs1], regTable[rs2]);
				break;
			case SUBW:
				aluFun = Sub;
				val1 = ((long long)((int)((unsigned int)val1)));
				val2 = ((long long)((int)((unsigned int)val2)));
				if(verbose) printf("subw $%s $%s $%s\n", regTable[rd], regTable[rs1], regTable[rs2]);
				break;
			case MULW:
				aluFun = Mul;
				val1 = ((long long)((int)((unsigned int)val1)));
				val2 = ((long long)((int)((unsigned int)val2)));
				if(verbose) printf("mulw $%s $%s $%s\n", regTable[rd], regTable[rs1], regTable[rs2]);
				break;
			default:
				BadCode();
				break;
			}
			break;
		case 1:
			switch(instruction->funct7)
			{
			case SLLW:
				aluFun = Shlw;
				shamt = (val2 & 31);
				if(verbose) printf("sllw $%s $%s $%s\n", regTable[rd], regTable[rs1], regTable[rs2]);
				break;
			default:
				BadCode();
				break;
			}
			break;
		case 4:
			switch(instruction->funct7)
			{
			case DIVW:
				aluFun = Div;
				val1 = ((long long)((int)((unsigned int)val1)));
				val2 = ((long long)((int)((unsigned int)val2)));
				if(verbose) printf("divw $%s $%s $%s\n", regTable[rd], regTable[rs1], regTable[rs2]);
				break;
			default:
				BadCode();
				break;
			}
			break;
		case 5:
			switch(instruction->funct7)
			{
			case SRLW:
				aluFun = Shrlw;
				shamt = (val2 & 31);
				if(verbose) printf("srlw $%s $%s $%s\n", regTable[rd], regTable[rs1], regTable[rs2]);
				break;
			case SRAW:
				aluFun = Shraw;
				shamt = (val2 & 31);
				if(verbose) printf("sraw $%s $%s $%s\n", regTable[rd], regTable[rs1], regTable[rs2]);
				break;
			case DIVUW:
				aluFun = Divu;
				val1 = ((long long)((int)((unsigned int)val1)));
				val2 = ((long long)((int)((unsigned int)val2)));
				if(verbose) printf("divuw $%s $%s $%s\n", regTable[rd], regTable[rs1], regTable[rs2]);
				break;
			default:
				BadCode();
				break;
			}
			break;
		case 6:
			switch(instruction->funct7)
			{
			case REMW:
				aluFun = Rem;
				val1 = ((long long)((int)((unsigned int)val1)));
				val2 = ((long long)((int)((unsigned int)val2)));
				if(verbose) printf("remw $%s $%s $%s\n", regTable[rd], regTable[rs1], regTable[rs2]);
				break;
			default:
				BadCode();
				break;
			}
			break;
		case 7:
			switch(instruction->funct7)
			{
			case REMUW:
				aluFun = Remu;
				if(verbose) printf("remuw $%s $%s $%s\n", regTable[rd], regTable[rs1], regTable[rs2]);
				break;
			default:
				BadCode();
				break;
			}
			break;
		default:
			BadCode();
			break;
		}
		break;
	case LOAD_FP:
		memFun = Read;
		aluFun = Add;
		aluSrc = Imm;
		writeFun = WWrite;
		rs1 = instruction->rs1;
		val1 = registerFile->getInteger(rs1);
		rd = instruction->rd;
		imm = (long long)((int)instruction->content >> 20);
		switch(instruction->funct3)
		{
		case FLW:
			memSize = MWords;
			writeSize = FWords;
			if(verbose) printf("flw $%s %ld($%s)\n", floatTable[rd], imm, regTable[rs1]);
			break;
		case FLD:
			memSize = MDouble;
			writeSize = FDouble;
			if(verbose) printf("fld $%s %ld($%s)\n", floatTable[rd], imm, regTable[rs1]);
			break;
		default:
			BadCode();
			break;
		}
		break;
	case STORE_FP:
		memFun = MWrite;
		aluFun = Add;
		aluSrc = Imm;
		writeFun = WNull;
		rs1 = instruction->rs1;
		val1 = registerFile->getInteger(rs1);
		rs2 = instruction->rs2;
		val2 = registerFile->getFloat(rs2);
		imm = ((instruction->content >> 7) & 31);
		imm += ((instruction->content >> 20) & ((1 << 12) - (1 << 5)));
		imm = ((long long)imm << 52 >> 52);
		switch(instruction->funct3)
		{
		case FSW:
			memSize = MWords;
			if(verbose) printf("fsw $%s %ld($%s)\n", floatTable[rs2], imm, regTable[rs1]);
			break;
		case FSD:
			memSize = MDouble;
			if(verbose) printf("fsd $%s %ld($%s)\n", floatTable[rs2], imm, regTable[rs1]);
			break;
		default:
			BadCode();
			break;
		}
		break;
	case OP_FP:
		memFun = MNull;
		aluSrc = Rs2;
		writeFun = WWrite;
		writeSize = FDouble;
		rs1 = instruction->rs1;
		rs2 = instruction->rs2;
		rd = instruction->rd;
		val1 = registerFile->getFloat(rs1);
		val2 = registerFile->getFloat(rs2);
		switch(instruction->funct7)
		{
		case FADD_D:
			aluFun = Fadd;
			if(verbose) printf("fadd.d $%s $%s $%s\n", floatTable[rd], floatTable[rs1], regTable[rs2]);
			break;
		case FSUB_D:
			aluFun = Fsub;
			if(verbose) printf("fsub.d $%s $%s $%s\n", floatTable[rd], floatTable[rs1], regTable[rs2]);
			break;
		case FMUL_D:
			aluFun = Fmul;
			if(verbose) printf("fmul.d $%s $%s $%s\n", floatTable[rd], floatTable[rs1], regTable[rs2]);
			break;
		case FDIV_D:
			aluFun = Fdiv;
			if(verbose) printf("fdiv.d $%s $%s $%s\n", floatTable[rd], floatTable[rs1], regTable[rs2]);
			break;
		case FSQRT_D:
			aluFun = Fsqrt;
			if(verbose) printf("fsqrt.d $%s $%s\n", floatTable[rd], floatTable[rs1]);
			break;
		case 97:
			switch(instruction->rs2)
			{
			case 0:
				aluFun = Fcvtwd;
				writeSize = WDouble;
				if(verbose) printf("fcvt.w.d $%s $%s\n", regTable[rd], floatTable[rs1]);
				break;
			case 1:
				aluFun = Fcvtwud;
				writeSize = WDouble;
				if(verbose) printf("fcvt.wu.d $%s $%s\n", regTable[rd], floatTable[rs1]);
				break;
			case 2:
				aluFun = Fcvtld;
				writeSize = WDouble;
				if(verbose) printf("fcvt.l.d $%s $%s\n", regTable[rd], floatTable[rs1]);
				break;
			case 3:
				aluFun = Fcvtlud;
				writeSize = WDouble;
				if(verbose) printf("fcvt.lu.d $%s $%s\n", regTable[rd], floatTable[rs1]);
				break;
			default:
				BadCode();
				break;
			}
			break;
		case 105:
			switch(instruction->rs2)
			{
			case 0:
				aluFun = Fcvtdw;
				val1 = registerFile->getInteger(rs1);
				val1 = (long long)((int)((unsigned int)val1));
				if(verbose) printf("fcvt.d.w $%s $%s\n", floatTable[rd], regTable[rs1]);
				break;
			case 1:
				aluFun = Fcvtdwu;
				val1 = registerFile->getInteger(rs1);
				val1 = (unsigned int)val1;
				if(verbose) printf("fcvt.d.wu $%s $%s\n", floatTable[rd], regTable[rs1]);
				break;
			case 2:
				aluFun = Fcvtdl;
				val1 = registerFile->getInteger(rs1);
				if(verbose) printf("fcvt.d.l $%s $%s\n", floatTable[rd], regTable[rs1]);
				break;
			case 3:
				aluFun = Fcvtdlu;
				val1 = registerFile->getInteger(rs1);
				if(verbose) printf("fcvt.d.l $%s $%s\n", floatTable[rd], regTable[rs1]);
				break;
			default:
				BadCode();
				break;
			}
			break;
		case FMV_X_D:
			aluFun = ANull;
			vald = val1;
			writeSize = WDouble;
			if(verbose) printf("fmv.x.d $%s $%s\n", regTable[rd], floatTable[rs1]);
			break;
		case FMV_D_X:
			aluFun = ANull;
			vald = registerFile->getInteger(rs1);
			if(verbose) printf("fmv.d.x $%s $%s\n", floatTable[rd], regTable[rs1]);
			break;
		case FCVT_S_D:
			aluFun = Fcvtsd;
			if(verbose) printf("fcvt.s.d $%s $%s\n", floatTable[rd], floatTable[rs1]);
			break;
		case FCVT_D_S:
			aluFun = Fcvtds;
			if(verbose) printf("fcvt.d.s $%s $%s\n", floatTable[rd], floatTable[rs1]);
			break;
		case 17:
			switch(instruction->funct3)
			{
			case 0:
				aluFun = Fsgnjd;
				if(verbose) printf("fsgnj.d $%s $%s $%s\n", floatTable[rd], floatTable[rs1], floatTable[rs2]);
				break;
			case 1:
				aluFun = Fsgnjnd;
				if(verbose) printf("fsgnjn.d $%s $%s $%s\n", floatTable[rd], floatTable[rs1], floatTable[rs2]);
				break;
			case 2:
				aluFun = Fsgnjxd;
				if(verbose) printf("fsgnjx.d $%s $%s $%s\n", floatTable[rd], floatTable[rs1], floatTable[rs2]);
				break;
			default:
				BadCode();
				break;
			}
			break;
		case 81:
			switch(instruction->funct3)
			{
			case 2:
				aluFun = Feqd;
				writeSize = WDouble;
				if(verbose) printf("feq.d $%s $%s $%s\n", regTable[rd], floatTable[rs1], floatTable[rs2]);
				break;
			case 1:
				aluFun = Fltd;
				writeSize = WDouble;
				if(verbose) printf("flt.d $%s $%s $%s\n", regTable[rd], floatTable[rs1], floatTable[rs2]);
				break;
			case 0:
				aluFun = Fled;
				writeSize = WDouble;
				if(verbose) printf("fle.d $%s $%s $%s\n", regTable[rd], floatTable[rs1], floatTable[rs2]);
				break;
			default:
				BadCode();
				break;
			}
			break;
		default:
			BadCode();
			break;
		}
		break;
	case MADD:
		memFun = MNull;
		writeFun = WWrite;
		writeSize = FDouble;
		rs1 = instruction->rs1;
		rs2 = instruction->rs2;
		val1 = registerFile->getFloat(rs1);
		val2 = registerFile->getFloat(rs2);
		rd = instruction->rd;
		switch(((instruction->content) >> 25) & 3)
		{
		case 1:
			aluFun = Fmaddd;
			if(verbose) printf("fmadd.d $%s $%s $%s\n", regTable[rd], floatTable[rs1], floatTable[rs2]);
			break;
		default:
			BadCode();
			break;
		}
		break;
	case MSUB:
		memFun = MNull;
		writeFun = WWrite;
		writeSize = FDouble;
		rs1 = instruction->rs1;
		rs2 = instruction->rs2;
		val1 = registerFile->getFloat(rs1);
		val2 = registerFile->getFloat(rs2);
		rd = instruction->rd;
		switch(((instruction->content) >> 25) & 3)
		{
		case 1:
			aluFun = Fmsubd;
			if(verbose) printf("fmsub.d $%s $%s $%s\n", regTable[rd], floatTable[rs1], floatTable[rs2]);
			break;
		default:
			BadCode();
			break;
		}
		break;
	case NMADD:
		memFun = MNull;
		writeFun = WWrite;
		writeSize = FDouble;
		rs1 = instruction->rs1;
		rs2 = instruction->rs2;
		val1 = registerFile->getFloat(rs1);
		val2 = registerFile->getFloat(rs2);
		rd = instruction->rd;
		switch(((instruction->content) >> 25) & 3)
		{
		case 1:
			aluFun = Fnmaddd;
			if(verbose) printf("nfmadd.d $%s $%s $%s\n", regTable[rd], floatTable[rs1], floatTable[rs2]);
			break;
		default:
			BadCode();
			break;
		}
		break;
	case NMSUB:
		memFun = MNull;
		writeFun = WWrite;
		writeSize = FDouble;
		rs1 = instruction->rs1;
		rs2 = instruction->rs2;
		val1 = registerFile->getFloat(rs1);
		val2 = registerFile->getFloat(rs2);
		rd = instruction->rd;
		switch(((instruction->content) >> 25) & 3)
		{
		case 1:
			aluFun = Fnmsubd;
			if(verbose) printf("nfsub.d $%s $%s $%s\n", regTable[rd], floatTable[rs1], floatTable[rs2]);
			break;
		default:
			BadCode();
			break;
		}
		break;
	case SYSTEM:
		memFun = MNull;
		aluFun = ANull;
		writeFun = WNull;
		switch(instruction->funct3)
		{
		case PRIV:
			switch(instruction->funct12)
			{
			case ECALL:
				if(verbose) printf("ecall\n");
				{lint v10 = registerFile->getInteger(10);
				lint v11 = registerFile->getInteger(11);
				lint v12 = registerFile->getInteger(12);
				lint v13 = registerFile->getInteger(13);
				lint v17 = registerFile->getInteger(17);
				syscall(v10, v11, v12, v13, v17, memory, registerFile);}
				break;
			case EBREAK:
				if(verbose) printf("ebreak\n");
				break;
			default:
				BadCode();
				break;
			}
			break;
		default:
			BadCode();
			break;
		}
		break;
	default:
		BadCode();
		break;
	}
}

void Machine::Execute()
{
	//if(verbose) printf("Execute\n");
	if(aluFun == ANull)
		return;

	lint v1 = val1;
	lint v2 = val2;
	
	switch(aluSrc)
	{
	case Rs2:
		v2 = val2;
		break;
	case Imm:
		v2 = imm;
		break;
	case Val4:
		v2 = 4;
		break;
	}

	//printf("Execute: %lu %lu\n", v1, v2);

	int neg;
	double f1 = *(double *)(&v1);
	double f2 = *(double *)(&v2);
	lint v3 = registerFile->getFloat(instruction->rs3);
	double f3 = *(double *)(&v3);
	double fd;
	float a1 = *(float *)(&v1);
	float a2 = *(float *)(&v2);
	float ad;

	switch(aluFun)
	{
	case Add:
		vald = v1 + v2;
		//printf("vald: %lu\n", vald);
		break;
	case Sub:
		vald = v1 - v2;
		break;
	case Mul:
		vald = (long long)v1 * (long long)v2;
		break;
	case Div:
		vald = (long long)v1 / (long long)v2;
		break;
	case Fadd:
		fd = f1 + f2;
		vald = *(lint *)(&fd);
		break;
	case Fsub:
		fd = f1 - f2;
		vald = *(lint *)(&fd);
		break;
	case Fmul:
		fd = f1 * f2;
		vald = *(lint *)(&fd);
		break;
	case Fdiv:
		fd = f1 / f2;
		vald = *(lint *)(&fd);
		break;
	case Fsqrt:
		fd = sqrt(f1);
		vald = *(lint *)(&fd);
		break;
	case And:
		vald = v1 & v2;
		break;
	case Or:
		vald = v1 | v2;
		break;
	case Xor:
		vald = v1 ^ v2;
		break;
	case Less:
		vald = ((long long)v1 < (long long)v2) ? 1 : 0;
		break;
	case Lessu:
		vald = (v1 < v2) ? 1 : 0;
		break;
	case Shl:
		vald = (v1 << shamt);
		break;
	case Shrl:
		vald = (v1 >> shamt);
		break;
	case Shra:
		vald = ((long long)v1 >> shamt);
		break;
	case Shlw:
		{unsigned int s1 = v1;
		s1 = (s1 << shamt);
		vald = (long long)((int)s1);}
		break;
	case Shrlw:
		{unsigned int s1 = v1;
		s1 = (s1 >> shamt);
		vald = (long long)((int)s1);}
		break;
	case Shraw:
		{int s1 = (int)((unsigned int)v1);
		s1 = (s1 >> shamt);
		vald = (long long)s1;}
		break;
	case Mulh:
		{neg = 1;
		if((long long)v1 < 0) 
		{
			neg = -neg;
			v1 = -v1;
		}
		if((long long)v2 < 0) 
		{
			neg = -neg;
			v2 = -v2;
		}
		unsigned int x1 = v1;
		unsigned int x2 = (v1 >> 32);
		unsigned int y1 = v2;
		unsigned int y2 = (v2 >> 32);
		unsigned int z1 = 0, z2 = 0, z3 = 0, z4 = 0;
		lint tmp = (lint)x1 * (lint)y1;
		z1 += tmp;
		z2 += (tmp >> 32);
		tmp = (lint)x1 * (lint)y2;
		if(z2 + (unsigned int)tmp < z2)
		{
			z3 ++;
		}
		z2 += tmp;
		if(z3 + (tmp >> 32) < z3)
		{
			z4 ++;
		}
		z3 += (tmp >> 32);
		tmp = (lint)x2 * (lint)y1;
		if(z2 + (unsigned int)tmp < z2)
		{
			z3 ++;
		}
		z2 += tmp;
		if(z3 + (tmp >> 32) < z3)
		{
			z4 ++;
		}
		z3 += (tmp >> 32);
		tmp = (lint)x2 * (lint)y2;
		if(z3 + (unsigned int)tmp < z3)
		{
			z4 ++;
		}
		z3 += tmp;
		z4 += (tmp >> 32);
		vald = z3 + ((lint)z4 << 32);
		vald = (long long)vald * neg;}
		break;
	case Mulhu:
		{unsigned int x1 = v1;
		unsigned int x2 = (v1 >> 32);
		unsigned int y1 = v2;
		unsigned int y2 = (v2 >> 32);
		unsigned int z1 = 0, z2 = 0, z3 = 0, z4 = 0;
		lint tmp = (lint)x1 * (lint)y1;
		z1 += tmp;
		z2 += (tmp >> 32);
		tmp = (lint)x1 * (lint)y2;
		if(z2 + (unsigned int)tmp < z2)
		{
			z3 ++;
		}
		z2 += tmp;
		if(z3 + (tmp >> 32) < z3)
		{
			z4 ++;
		}
		z3 += (tmp >> 32);
		tmp = (lint)x2 * (lint)y1;
		if(z2 + (unsigned int)tmp < z2)
		{
			z3 ++;
		}
		z2 += tmp;
		if(z3 + (tmp >> 32) < z3)
		{
			z4 ++;
		}
		z3 += (tmp >> 32);
		tmp = (lint)x2 * (lint)y2;
		if(z3 + (unsigned int)tmp < z3)
		{
			z4 ++;
		}
		z3 += tmp;
		z4 += (tmp >> 32);
		vald = z3 + ((lint)z4 << 32);}
		break;
	case Mulhsu:
		{neg = 1;
		if((long long)v1 < 0)
		{
			neg = -neg;
			v1 = -v1;
		}
		unsigned int x1 = v1;
		unsigned int x2 = (v1 >> 32);
		unsigned int y1 = v2;
		unsigned int y2 = (v2 >> 32);
		unsigned int z1 = 0, z2 = 0, z3 = 0, z4 = 0;
		lint tmp = (lint)x1 * (lint)y1;
		z1 += tmp;
		z2 += (tmp >> 32);
		tmp = (lint)x1 * (lint)y2;
		if(z2 + (unsigned int)tmp < z2)
		{
			z3 ++;
		}
		z2 += tmp;
		if(z3 + (tmp >> 32) < z3)
		{
			z4 ++;
		}
		z3 += (tmp >> 32);
		tmp = (lint)x2 * (lint)y1;
		if(z2 + (unsigned int)tmp < z2)
		{
			z3 ++;
		}
		z2 += tmp;
		if(z3 + (tmp >> 32) < z3)
		{
			z4 ++;
		}
		z3 += (tmp >> 32);
		tmp = (lint)x2 * (lint)y2;
		if(z3 + (unsigned int)tmp < z3)
		{
			z4 ++;
		}
		z3 += tmp;
		z4 += (tmp >> 32);
		vald = z3 + ((lint)z4 << 32);
		vald = (long long)vald * neg;}
		break;
	case Divu:
		vald = v1 / v2;
		break;
	case Rem:
		vald = (long long)v1 % (long long)v2;
		break;
	case Remu:
		vald = v1 % v2;
		break;
	case Fcvtwd:
		vald = (long long)((int)f1);
		break;
	case Fcvtwud:
		vald = (lint)((unsigned int)f1);
		break;
	case Fcvtdw:
		fd = (double)((int)v1);
		vald = *(lint *)(&fd);
		break;
	case Fcvtdwu:
		fd = (double)((unsigned int)v1);
		vald = *(lint *)(&fd);
		break;
	case Fcvtld:
		vald = (long long)f1;
		break;
	case Fcvtlud:
		vald = (lint)f1;
		break;
	case Fcvtdl:
		fd = (double)((long long)v1);
		vald = *(lint *)(&fd);
		break;
	case Fcvtdlu:
		fd = (double)v1;
		vald = *(lint *)(&fd);
		break;
	case Fcvtsd:
		ad = (float)f1;
		vald = (lint)(*(unsigned int*)(&ad));
		break;
	case Fcvtds:
		fd = (double)a1;
		vald = *(lint *)(&fd);
		break;
	case Fsgnjd:
		vald = (v1 & ((1 << 63) - 1)) + (v2 & (1 << 63));
		break;
	case Fsgnjnd:
		vald = (v1 & ((1 << 63) - 1)) + ((~v2) & (1 << 63));
		break;
	case Fsgnjxd:
		vald = (v1 & ((1 << 63) - 1)) + ((v2 & (1 << 63)) ^ (v1 & (1 << 63)));
		break;
	case Feqd:
		vald = (f1 == f2) ? 1 : 0;
		break;
	case Fltd:
		vald = (f1 < f2) ? 1 : 0;
		break;
	case Fled:
		vald = (f1 <= f2) ? 1 : 0;
		break;
	case Fmaddd:
		fd = f1*f2+f3;
		vald = *(lint *)(&fd);
		break;
	case Fmsubd:
		fd = f1*f2-f3;
		vald = *(lint *)(&fd);
		break;
	case Fnmaddd:
		fd = -(f1*f2+f3);
		vald = *(lint *)(&fd);
		break;
	case Fnmsubd:
		fd = -(f1*f2-f3);
		vald = *(lint *)(&fd);
		break;
	}
}

void Machine::MemoryAccess()
{
	//void *address = (void *)vald;
	//printf("MemoryAccess\n");
	if(memFun == MNull)
		return;
	void *content = NULL;
	switch(memFun)
	{
	case Read:
		switch(memSize)
		{
		case MDouble:
			//printf("Debug0 %lu\n", vald);
			content = memory->Load(vald, 8);
			//printf("Debug1\n");
			vald = (*(lint *)(content));
			break;
		case MWords:
			content = memory->Load(vald, 4);
			vald = (long long)(*(int *)(content));
			break;
		case MHalfWords:
			content = memory->Load(vald, 2);
			vald = (long long)(*(short *)(content));
			break;
		case MBytes:
			content = memory->Load(vald, 1);
			vald = (long long)(*(char *)(content));
			break;
		case MWordsu:
			content = memory->Load(vald, 4);
			vald = (*(unsigned int *)(content));
			break;
		case MHalfWordsu:
			content = memory->Load(vald, 2);
			vald = (*(unsigned short *)(content));
			break;
		case MBytesu:
			content = memory->Load(vald, 1);
			vald = (*(unsigned char *)(content));
			break;

		}
		break;
	case MWrite:
		switch(memSize)
		{
		case MDouble:
			memory->Store(vald, 8, (char*)(&val2));
			break;
		case MWords:
			memory->Store(vald, 4, (char*)(&val2));
			break;
		case MHalfWords:
			memory->Store(vald, 2, (char*)(&val2));
			break;
		case MBytes:
			memory->Store(vald, 1, (char*)(&val2));
			break;
		}
		break;
	}
	if(content != NULL) delete content;
}

void Machine::WriteBack()
{
	//printf("WriteBack\n");
	if(writeFun == WNull)
		return;

	switch(writeSize)
	{
	case WDouble:
	case WWords:
	case WHalfWords:
	case WBytes:
		if(rd != ZERO) registerFile->setInteger(rd, vald);
		break;
	case FDouble:
	case FWords:
	case FHalfWords:
	case FBytes:
		registerFile->setFloat(rd, vald);
		break;
	}
}

void Machine::BadCode()
{
	printf("It is a bad code! The machine halts!\n");
	state = Halt;
	PrintReg();
}	

void Machine::PrintReg()
{
	printf("Debug:\n");
	for(int i = 0; i < 32; i++)
	{
		printf("$%s: %lx\t", regTable[i], registerFile->getInteger(i));
		if((i+1)%4 == 0)
		{
			printf("\n");
		}
	}
	printf("$%s: %lx\n", "pc", registerFile->getPC());
}