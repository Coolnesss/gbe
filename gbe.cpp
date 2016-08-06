#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string>
#include "gbe.h"
#include "gpu.h"
#include "window.h"

void readROMFile(const char * filename) {
	FILE* romfile; 
	romfile = fopen(filename, "rb");

	fread(MEM.ROM0,0x3fff,1,romfile);
	fread(MEM.ROM1,0x3fff,1,romfile);
	fclose(romfile);
}

void readBIOSFile(const char * filename) {
	FILE* biosfile; 
	biosfile = fopen(filename, "rb");

	assert(biosfile);

	fseek(biosfile, 0, SEEK_END);
	size_t n = ftell(biosfile);
	assert(n == 256);
	fseek(biosfile, 0, SEEK_SET);
	fread(MEM.BIOS,n,1,biosfile);

	fclose(biosfile);
}

int main(int argc, char ** argv) {
	readBIOSFile("rom.bin");
	readROMFile("tetris.gb");

	bool log_register_bytes = false, 
			 log_register_words = false, 
			 log_flags = false,
			 log_mem = false,
			 log_gpu = false,
			 silent = false;

  uint16_t log_mem_addr = 0;

	printf("%d\n", argc);
	for (int i = 1; i < argc; ++i) {
		std::string flag(argv[i]);
		if (flag == "-rb") log_register_bytes = true;
		if (flag == "-rw") log_register_words = true;
		if (flag == "-f") log_flags = true;
		if (flag == "-g") log_gpu = true;
		if (flag == "-s") silent = true;
		if (flag.find("-m") == 0) {
			log_mem = true;
			log_mem_addr = 0xff50;
		}
	}

	WINDOW.init();

	while (1) {
		uint8_t opcode = MEM.readByte(REG.PC);
		instruction instr = instructions[opcode];

		if (log_register_bytes)
		printf("A %02X F %02X B %02X C %02X D %02X E %02X H %02X L %02X\n",
					 REG.A, REG.F, REG.B, REG.C, REG.D, REG.E, REG.H, REG.L);
		if (log_register_words)
		printf("AF %04X BC %04X DE %04X HL %04X SP %04X PC %04X\n",
					 REG.AF, REG.BC, REG.DE, REG.HL, REG.SP, REG.PC);
		if (log_flags)
		printf("Z %1d N %1d H %1d C %1d\n",
						get_flag(FLAG_Z), get_flag(FLAG_N), get_flag(FLAG_H), get_flag(FLAG_C));
		if (log_mem) {
			printf("0x%02X\n", MEM.readByte(log_mem_addr));
		}
		if (log_gpu) {
			printf("GPU CLK: 0x%04X  MODE: %d  LINE: 0x%02X\n", 
						  GPU.clk, GPU.mode, *MEM.SCAN_LN);
		}

		if (!silent) {
			printf("Instruction 0x%02X at 0x%04X: ", opcode, REG.PC);
			if (instr.argw == 0)
				printf(instr.name);
			else if (instr.argw == 1)
				printf(instr.name, MEM.readByte(REG.PC+1));
			else if (instr.argw == 2)
				printf(instr.name, MEM.readWord(REG.PC+1));
			printf("\n");

			if (opcode == 0xCB) {
				uint8_t ext_opcode = MEM.readByte(REG.PC+1);
				printf("        Ext 0x%02X at 0x%04X: ", ext_opcode, REG.PC+1);
				printf(ext_instructions[MEM.readByte(REG.PC+1)].name);
				printf("\n");
			}
		}

		instr.fn();
		GPU.update();

		if (REG.IME && (*MEM.IE & *MEM.IF)) {

			// TODO: handle interrupt
		}

	}
}