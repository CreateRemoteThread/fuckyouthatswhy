#include <avr/pgmspace.h>
#include "disk_emu.h"

/*
read:
ld a,(sekdsk)
out ($F0),a
ld a,(sektrk)
out ($F1),a
ld a,(seksec)
out ($F2),a
ld a,2
out ($FF),a
ld hl,(dmaadr)
ld b,128
rx_nextbyte:
in a,($FE)
ld (hl),a
inc hl
djnz rx_nextbyte
ld a,0 ; TODO: ERROR HANDLING
ret
*/

unsigned char eepromBuffer[128];
unsigned char ioHeader = 0;
unsigned char save_sec = 0;
// CPM's 0xFF,2
void setupSector(unsigned char disk,unsigned char trk,unsigned char sec) 
{
	save_sec = sec;
	if(sec >= 0 && sec <= 7)
	{
		eeprom_read_block(eepromBuffer,sec * 128,128);
	}
	else
	{
		printf("FORGING SECTOR %d\r\n",sec);
		memset(eepromBuffer,0xE5,128);
	}
	ioHeader = 0;
}

// 0xFE
unsigned char fetchByte()
{
	unsigned char temp = eepromBuffer[ioHeader];
	ioHeader += 1;
	if(ioHeader == 128)
	{
		printf("RESETTING READ HEAD\r\n");
		ioHeader = 0;
	}
	ioHeader =0;
	return temp;
}

// 0xFE
void putByte(unsigned char data)
{
	eepromBuffer[ioHeader] = data;
	ioHeader += 1;
	if(ioHeader == 128)
	{
		if(save_sec >= 0 && save_sec <= 7)
		{
			printf("COMMITTING CHANGE\r\n");
			eeprom_write_block(eepromBuffer,save_sec * 128,128);
		}
		else
		{
			printf("WRITE TO >1KB, IGNORING\r\n");
		}
		ioHeader = 0;
	}
}