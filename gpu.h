#pragma once

#include "reg.h"
#include "mem.h"
#include "window.h"
#include "interrupts.h"

#define MODE_OAM 2
#define MODE_VRAM 3
#define MODE_HBLANK 0
#define MODE_VBLANK 1

#define INT_HBLANK 0x04
#define INT_VBLANK 0x10
#define INT_OAM    0x20
#define INT_LYC    0x40

#define MODE_MASK 0x03

#define STAT_LYC 0x04

void update_lcd_status(uint8_t mode) {
	*MEM.LCD_STAT &= ~MODE_MASK;
	*MEM.LCD_STAT |= mode;
}

typedef struct {

	uint16_t clk;

	void update() {
		clk += REG.TCLK;
		switch (*MEM.LCD_STAT & MODE_MASK) {
			case (MODE_OAM):
				if (clk >= 80) {
					clk = 0;
					update_lcd_status(MODE_VRAM);
				}
				break;
			case (MODE_VRAM):
				if (clk >= 172) {
					clk = 0;
					update_lcd_status(MODE_HBLANK);
					WINDOW.render_buffer_line();
				}
				break;
			case (MODE_HBLANK):
				if (clk >= 204) {
					clk = 0;
					*MEM.SCAN_LN += 1;
					if (*MEM.SCAN_LN == 143) {
						update_lcd_status(MODE_VBLANK);
						WINDOW.draw_buffer();
					} else {
						update_lcd_status(MODE_OAM);
					}
				}
				break;
			case (MODE_VBLANK):
				if (clk >= 456) {
					clk = 0;
					if (*MEM.SCAN_LN == 143)
						*MEM.IF |= FLAG_IF_VBLANK;
					*MEM.SCAN_LN += 1;
					if (*MEM.SCAN_LN == 153) {
						update_lcd_status(MODE_OAM);
						*MEM.SCAN_LN = 0;
					}
				}
				break;
		}

		if (*MEM.SCAN_LN == *MEM.LN_CMP) {
			*MEM.LCD_STAT |= STAT_LYC;
		}	else {
			*MEM.LCD_STAT &= ~STAT_LYC;
		}

		// Trigger LCD interrupt
		if (((*MEM.LCD_STAT & STAT_LYC) && (*MEM.LCD_STAT & INT_LYC)) ||
			  (((*MEM.LCD_STAT & MODE_MASK) == MODE_OAM) && (*MEM.LCD_STAT & INT_OAM)) ||
			  (((*MEM.LCD_STAT & MODE_MASK) == MODE_VBLANK) && (*MEM.LCD_STAT & INT_VBLANK)) ||
			  (((*MEM.LCD_STAT & MODE_MASK) == MODE_HBLANK) && (*MEM.LCD_STAT & INT_HBLANK))) {
			*MEM.IF |= FLAG_IF_LCD;
		}
	}
} gpu;

gpu GPU;