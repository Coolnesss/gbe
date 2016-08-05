#pragma once

#include "reg.h"

#define FLAG_IF_VBLANK 0x01
#define FLAG_IF_LCD    0x02
#define FLAG_IF_TIMER  0x04
#define FLAG_IF_SERIAL 0x08
#define FLAG_IF_JOYPAD 0x10

#define FLAG_GPU_BG     0x01
#define FLAG_GPU_SPR    0x02
#define FLAG_GPU_SPR_SZ 0x04
#define FLAG_GPU_BG_TM  0x08
#define FLAG_GPU_BG_TS  0x10
#define FLAG_GPU_WIN    0x20
#define FLAG_GPU_WIN_TM 0x40
#define FLAG_GPU_DISP   0x80

typedef struct {
	uint8_t ROM0[16384];  // [0000-3FFF] 
	uint8_t ROM1[16384];  // [4000-7FFF]
	uint8_t grRAM[8192];  // [8000-9FFF] 
	uint8_t extRAM[8192]; // [A000-BFFF]
	uint8_t RAM[8192];    // [C000-DFFF]
	uint8_t _RAM[7680];   // [E000-FDFF]
	uint8_t SPR[255];     // [FE00-FE9F] // 160 bytes
	uint8_t IO[128];      // [FF00-FF7F] 
	uint8_t ZERO[128];    // [FF80-FFFF]

	bool bios = true;
	uint8_t BIOS[256]; 

	uint8_t *IE       = &ZERO[0xFF];
	uint8_t *IF       = &IO[0x0F];
	uint8_t *GPU_CTRL = &IO[0x40];
	uint8_t *SCRL_Y   = &IO[0x42];
	uint8_t *SCRL_X   = &IO[0x43];
	uint8_t *SCAN_LN  = &IO[0x44]; // TODO: readonly
	uint8_t *BG_PLT   = &IO[0x47]; // TODO: writeonly

	uint8_t* getPtr(uint16_t addr) {
		// switch by 8192 byte segments
		switch(addr >> 13) {
			case 0:
				if (bios) {
					if (REG.PC < 0x100) 
						return &BIOS[addr];
					else
						bios = false;
				}
			case 1:
				return &ROM0[addr];
			case 2:
			case 3:
				return &ROM1[addr & 0x3FFF];
			case 4:
				return &grRAM[addr & 0x1FFF];
			case 5:
				return &extRAM[addr & 0x1FFF];
			case 6:
				return &RAM[addr & 0x1FFF];
			case 7:
				switch (addr & 0xFF80) {
					case 0xFE00:
					case 0xFE80:
						if (addr < 0xFEA0)
							return &SPR[addr & 0x000F];
						else 
							return nullptr;
					case 0xFF00:
						return &IO[addr & 0x007F];
					case 0xFF80:
						return &ZERO[addr & 0x007F];
					default:
						return &RAM[addr & 0x1FFF];
				}
		}
	}

	uint8_t readByte(uint16_t addr) {
		uint8_t *ptr = getPtr(addr);
		if (ptr != nullptr)
			return *ptr;
		else
			return 0;
	}

	uint16_t readWord(uint16_t addr) {
		uint8_t *ptr = getPtr(addr);
		if (ptr != nullptr)
			return *reinterpret_cast<uint16_t*>(ptr); 
		else
			return 0;
	}

	void writeByte(uint16_t addr, uint8_t val) {
		uint8_t *ptr = getPtr(addr);
		if (ptr == nullptr) return;
		*ptr = val;
	}

	void writeWord(uint16_t addr, uint16_t val) {
		uint8_t *ptr = getPtr(addr);
		if (ptr == nullptr) return;
		uint16_t *wptr = reinterpret_cast<uint16_t*>(getPtr(addr)); 
		*wptr = val;
	}
} memory;

memory MEM;

#define ARGBYTE (MEM.readByte(REG.PC + 1))
#define ARGWORD (MEM.readWord(REG.PC + 1))