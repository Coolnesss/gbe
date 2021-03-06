#pragma once

#include <fstream>
#include <string>
#include <cstring>
#include <map>
#include <cassert>

using namespace std;

class Cart {
public:
	enum mbc_type { NONE, MBC1, MBC2, MBC3, MBC5 };
	mbc_type bank_controller;

	uint8_t *ROM;
	uint8_t *RAM;

	unsigned rom_size;
	unsigned ram_size;

	unsigned rom_bank;
	unsigned ram_bank;

	unsigned rom_banks;
	unsigned ram_banks;

	uint8_t *romBank(unsigned bank) {
		assert(bank < rom_banks);
		rom_bank = bank;
		return &ROM[0x4000 * bank]; 
	}

	uint8_t *ramBank(unsigned bank) {
		assert(bank < ram_banks);
		ram_bank = bank;
		return &RAM[0x2000 * bank];
	}

	Cart(string &filename) {
		ifstream romfile(filename, ios::binary);
		if (!romfile.good()) {
			printf("could not open ROM file\n");
			exit(1);
		}

		romfile.seekg(0, romfile.end);
		rom_size = romfile.tellg();

		romfile.seekg(0, romfile.beg);
		ROM = (uint8_t*)malloc(rom_size);

		romfile.read((char*)ROM, rom_size);
		romfile.close();

  	// cart data reference http://www.devrs.com/gb/files/gbspec.txt

  	char rom_name[16];
  	memcpy(rom_name, &ROM[0x0134], 16);
  	printf("NAME:\t%-16s\n", rom_name);
  	printf("TYPE:\t");
  	if (ROM[0x0143] == 0x80) printf("GBC ");
  	if (ROM[0x0146] == 0x00) printf("GB ");
  	if (ROM[0x0146] == 0x03) printf("SGB ");
  	printf("\n");

  	printf("SIZE:\t%dK\n", rom_size / 1024);

		uint8_t cart_type = ROM[0x0147];
		if (cart_types.count(cart_type)) {
			printf("CART:\t%s\n", cart_types[cart_type].c_str());
		} else {
			printf("Unknown type 0x%02X\n", cart_type);
		}

		switch (cart_type) {
			default:
			case 0x00:
				bank_controller = mbc_type::NONE;
				break;
			case 0x01: case 0x02: case 0x03:
				bank_controller = mbc_type::MBC1;
				break;
			case 0x05: case 0x06:
				bank_controller = mbc_type::MBC2;
				break;			
			case 0x0F: case 0x10: case 0x11:
			case 0x12: case 0x13:
				bank_controller = mbc_type::MBC3;
				break;
			case 0x19: case 0x1A: case 0x1B:
			case 0x1C: case 0x1D: case 0x1E:
				bank_controller = mbc_type::MBC5;
				break;
		}	

		uint8_t rom_type = ROM[0x0148];
		if (rom_types.count(rom_type)) {
			printf("ROM:\t%s\n", rom_types[rom_type].first.c_str());
			rom_banks = rom_types[rom_type].second;
			assert(rom_banks == rom_size / 0x4000);
		} else {
			printf("Unknown cart ROM type 0x%02X\n", rom_type);
			exit(1);
		}

	  uint8_t ram_type = ROM[0x0149];
		if (ram_types.count(ram_type)) {
			printf("RAM:\t%s\n", ram_types[ram_type].first.c_str());
			ram_banks = ram_types[ram_type].second;
			ram_size = 0x2000 * ram_banks;
			RAM = (uint8_t*)malloc(ram_size);
		} else {
			printf("Unknown cart RAM type 0x%02X\n", ram_type);
			exit(1);
		}

		// TODO: compute checksum
		printf("CPL:\t0x%02X\n", ROM[0x014D]);
		printf("CHK:\t0x%02X 0x%02X\n", ROM[0x014E], ROM[0x014F]);
	}

private:
	map<uint8_t, string> cart_types{
		{0x00, "ROM ONLY"},
		{0x01, "ROM+MBC1"},
		{0x02, "ROM+MBC1+RAM"},
		{0x03, "ROM+MBC1+RAM+BATT"},
		{0x05, "ROM+MBC2"},
		{0x06, "ROM+MBC2+BATTERY"},
		{0x08, "ROM+RAM"},
		{0x09, "ROM+RAM+BATTERY"},
		{0x0B, "ROM+MMM01"},
		{0x0C, "ROM+MMM01+SRAM"},
		{0x0D, "ROM+MMM01+SRAM+BATT"},
		{0x0F, "ROM+MBC3+TIMER+BATT"},
		{0x10, "ROM+MBC3+TIMER+RAM+BATT"},
		{0x11, "ROM+MBC3"},
		{0x12, "ROM+MBC3+RAM"},
		{0x13, "ROM+MBC3+RAM+BATT"},
		{0x19, "ROM+MBC5"},
		{0x1A, "ROM+MBC5+RAM"},
		{0x1B, "ROM+MBC5+RAM+BATT"},
		{0x1C, "ROM+MBC5+RUMBLE"},
		{0x1D, "ROM+MBC5+RUMBLE+SRAM"},
		{0x1E, "ROM+MBC5+RUMBLE+SRAM+BATT"},
		{0x1F, "Pocket Camera"},
		{0xFD, "Bandai TAMA5"},
		{0xFE, "Hudson HuC-3"},
		{0xFF, "Hudson HuC-1"}
	};

	map<uint8_t, pair<string, unsigned>> rom_types{
		{0x00, {"32KB", 2}},
		{0x01, {"64KB", 4}},
		{0x02, {"128KB", 8}},
		{0x03, {"256KB", 16}},
		{0x04, {"512KB", 32}},
		{0x05, {"1MB", 64}},
		{0x06, {"2MB", 128}},
		{0x52, {"1.1MB", 72}},
		{0x53, {"1.2MB", 80}},
		{0x54, {"1.5MB", 96}}
	};

	map<uint8_t, pair<string, unsigned>> ram_types{
		{0x00, {"None", 0}},
	  {0x01, {"2KB", 1}},
	  {0x02, {"8KB", 1}},
	  {0x03, {"32KB", 4}},
	  {0x04, {"128KB", 16}}
	};
};
