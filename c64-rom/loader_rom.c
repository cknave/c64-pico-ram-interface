#include <stdint.h>

const uint8_t loader_rom[157] = {
	0x09, 0x80, 0x09, 0x80, 0xC3, 0xC2, 0xCD, 0x38,
	0x30, 0x20, 0x81, 0xFF, 0x20, 0x84, 0xFF, 0xA9,
	0x00, 0x8D, 0x20, 0xD0, 0x8D, 0x21, 0xD0, 0xA9,
	0x20, 0xA2, 0x00, 0x9D, 0x00, 0x04, 0x9D, 0x00,
	0x05, 0x9D, 0x00, 0x06, 0x9D, 0x00, 0x07, 0xCA,
	0xD0, 0xF1, 0xA2, 0x16, 0xA9, 0x00, 0x85, 0xF8,
	0x85, 0xFA, 0x85, 0xFC, 0x85, 0xFE, 0xA9, 0x20,
	0x85, 0xF9, 0xA9, 0x21, 0x85, 0xFB, 0xA9, 0x22,
	0x85, 0xFD, 0xA9, 0x23, 0x85, 0xFF, 0xA0, 0x00,
	0xAD, 0x00, 0x9E, 0x8D, 0x20, 0xD0, 0xD0, 0xF8,
	0xB9, 0x00, 0x84, 0x91, 0xF8, 0xB9, 0x00, 0x85,
	0x91, 0xFA, 0xB9, 0x00, 0x86, 0x91, 0xFC, 0xB9,
	0x00, 0x87, 0x91, 0xFE, 0xC8, 0xD0, 0xE1, 0xAD,
	0x01, 0x9E, 0x18, 0xA9, 0x04, 0x65, 0xF9, 0x85,
	0xF9, 0xA9, 0x04, 0x65, 0xFB, 0x85, 0xFB, 0xA9,
	0x04, 0x65, 0xFD, 0x85, 0xFD, 0xA9, 0x04, 0x65,
	0xFF, 0x85, 0xFF, 0xCA, 0xD0, 0xC2, 0xA2, 0x00,
	0xBD, 0x00, 0x84, 0x9D, 0x00, 0x78, 0xE8, 0xD0,
	0xF7, 0xBD, 0x00, 0x85, 0x9D, 0x00, 0x79, 0xE8,
	0xD0, 0xF7, 0x4C, 0x00, 0x30,
};