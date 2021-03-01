#define F_CPU 8000000UL

// #define DEBUG_MEMORY 1

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdlib.h>
#include <stdio.h>
#include <util/delay.h>
#include <avr/pgmspace.h>

#define BAUDRATE 9600 // define baud
#define BAUD_PRESCALER ((F_CPU)/(BAUDRATE * 16UL) - 1) // set baud rate value for UBRR

#include "out_pmem.h"

// CP/M Bootloader
const unsigned char tiny_bl[] PROGMEM = {0xf3,0xc3,0x04,0x00,0x21,0x00,0x00,0x11,0x00,0x40,0x01,0x24,0x00,0xed,0xb0,0xc3,0x12,0x40,0x21,LADDR_LOW,LADDR_HIGH,0x01,LLEN_LOW,LLEN_HIGH,0xdb,0xff,0x77,0x23,0x0b,0x78,0xb1,0x20,0xf7,0xc3,JMP_LOW,JMP_HIGH};
long int bl_ctr = 0;

#define IOTRAP_HUSH 

int bios_dsk = 0;
int bios_trk = 0;
int bios_sec = 0;

void uart_init (void)
{
	UBRRH = (uint8_t)(BAUD_PRESCALER>>8);
	UBRRL = (uint8_t)(BAUD_PRESCALER);
	UCSRB = (1<<RXEN)|(1<<TXEN)|(1 << RXCIE);
	UCSRC = (1<<UCSZ0)|(1<<UCSZ1)|(1<<URSEL);
}

int uart_transmit(char data  )
{
	while(!(UCSRA & (1<<UDRE)));
	UDR = data;
	return 0;
}

unsigned char uart_receive(void){
	while(!(UCSRA & (1 << RXC)));
	return UDR;
}

void setAddressBus(unsigned char addr)
{
	#ifdef DEBUG_MEMORY
		printf("DBG:ADDR:%02x\r\n",addr);
	#endif
	PORTA = addr;
	/*
	if((addr >> 8 & 0b111) != (addr >> 8))
	{
		printf("ERR:OOB WR %04lx\r\n",addr);
		while(1)
		{
			_delay_ms(100);
		}
	}
	*/
}

#define ASSERT_MREQ_LOW PORTD &= ~(1 << PORTD7)
#define ASSERT_MREQ_HIGH PORTD |= (1 << PORTD7)
#define ASSERT_WRITE_LOW PORTD &= ~(1 << PORTD5)
#define ASSERT_WRITE_HIGH PORTD |= (1 << PORTD5)
#define ASSERT_READ_LOW PORTD &= ~(1 << PORTD4)
#define ASSERT_READ_HIGH PORTD |= (1 << PORTD4)

#define ASSERT_LED_HIGH PORTC |= (1 << PORTC0)
#define ASSERT_LED_LOW PORTC &= ~(1 << PORTC0)

#define ASSERT_CLK_LOW PORTD &= ~(1 << PORTD3)
#define ASSERT_CLK_HIGH PORTD |= (1 << PORTD3)
#define ASSERT_RESET_LOW PORTD &= ~(1 << PORTD2)
#define ASSERT_RESET_HIGH PORTD |= (1 << PORTD2)
#define ASSERT_INT_LOW PORTC &= ~(1 << PORTC7)
#define ASSERT_INT_HIGH PORTC |= (1 << PORTC7)

#define ASSERT_BUSRQ_LOW PORTC &= ~(1 << PORTC5)
#define ASSERT_BUSRQ_HIGH PORTC |= (1 << PORTC5)

#define ASSERT_NMI_LOW PORTC &= ~(1 << PORTC2)
#define ASSERT_NMI_HIGH PORTC |= (1 << PORTC2)

#define ASSERT_MASKUART_LOW PORTC &= ~(1 << PORTC1)
#define ASSERT_MASKUART_HIGH PORTC |= (1 << PORTC1)

#define ASSERT_DATABUS_INPUT DDRB = 0x00
#define ASSERT_DATABUS_OUTPUT DDRB = 0xFF

#define ASSERT_RWCTRL_OUTPUT DDRD |= (1 << PORTD4) | (1 << PORTD5)
#define ASSERT_RWCTRL_INPUT DDRD &= ~((1 << PORTD4) | (1 << PORTD5))
#define ASSERT_RWCTRL_PULLUPS PORTD |= (1 << PORTD4) | (1 << PORTD5)

int grabNibble()
{
	unsigned char c = getchar();
	if(c >= '0' && c <= '9')
	{
		return c - '0';
	}
	else if(c >= 'A' && c <= 'F')
	{
		return c - 'A' + 10;
	}
	else
	{
		return c - 'a' + 10;
	}
}

int grabByte()
{
	return (grabNibble() << 4) + grabNibble();
}

void writeAndVerifyPattern(unsigned char addr, int byteCount)
{
	printf("DBG:PATTERN\r\n");
	unsigned char i = 0;
	unsigned char tempByte;
	ASSERT_DATABUS_OUTPUT;
	ASSERT_MREQ_LOW;
	ASSERT_RWCTRL_OUTPUT;
	for(i = addr;i < byteCount;i++)
	{
		setAddressBus(i);
		tempByte = byteCount % 255;
		PORTB = tempByte;
		#ifdef DEBUG_MEMORY
		printf("DBG:DATA:%02x\r\n",tempByte);
		#endif
		ASSERT_WRITE_LOW;
		_delay_us(25);
		ASSERT_WRITE_HIGH;
	}
	PORTB = 0x00;
	ASSERT_DATABUS_INPUT;
	unsigned char failMemCheck = 0;
	
	unsigned char dataCheck = 0;
	for(i =addr;i < byteCount;i++)
	{
		setAddressBus(i);
		ASSERT_READ_LOW;
		_delay_us(25);
		dataCheck = PINB;
		ASSERT_READ_HIGH;
		unsigned char lol = byteCount % 255;
		if(dataCheck != lol)
		{
			printf("ERR:%02x:EXPECT %02x GOT %02x\r\n",i,lol,dataCheck);
			failMemCheck = 1;
		}
	}
	if(failMemCheck == 1)
	{
		while(1)
		{
			_delay_ms(100);
		}
	}
	ASSERT_MREQ_HIGH;
	ASSERT_RWCTRL_INPUT;
}

void writeAndVerify(unsigned char addr, int bytes, const unsigned char *data)
{
	unsigned char i = 0;
	unsigned char tempByte;
	ASSERT_DATABUS_OUTPUT;
	ASSERT_MREQ_LOW;
	ASSERT_RWCTRL_OUTPUT;
	for(i = addr;i < bytes;i++)
	{
		setAddressBus(i);
		tempByte = pgm_read_byte(data + i);
		PORTB = tempByte;
		#ifdef DEBUG_MEMORY
		printf("DBG:DATA:%02x\r\n",tempByte);
		#endif
		ASSERT_WRITE_LOW;
		_delay_us(25);
		ASSERT_WRITE_HIGH;
	}
	PORTB = 0x00;
	ASSERT_DATABUS_INPUT;
	unsigned char failMemCheck = 0;
	
	unsigned char dataCheck = 0;
	for(i =addr;i < bytes;i++)
	{
		setAddressBus(i);
		ASSERT_READ_LOW;
		_delay_us(25);
		dataCheck = PINB;
		ASSERT_READ_HIGH;
		unsigned char lol = pgm_read_byte(data + i);
		if(dataCheck != lol)
		{
			printf("ERR:%02x:EXPECT %02x GOT %02x\r\n",i,lol,dataCheck);
			failMemCheck = 1;
		}
	}
	if(failMemCheck == 1)
	{
		while(1)
		{
			_delay_ms(100);
		}
	}
	ASSERT_MREQ_HIGH;
	ASSERT_RWCTRL_INPUT;
}

#define PORT_MREQ PORTD7
#define PORT_IOREQ PORTD6
#define PORT_WRITE PORTD5
#define PORT_READ PORTD4

#define LOAD_STATIC_PROGRAM 1

// no input buffering - if the guest misses it, tough cookies.
int  receivedFlag = 1;
char receivedBuffer = 0;
int  interruptFlag = 0;
long int interruptVector = 0;

void ioreqClockPulse()
{
	ASSERT_CLK_LOW; // fake a clock cycle
	_delay_us(25);
	ASSERT_CLK_HIGH;
	_delay_us(25);
}

void ioreqRead(int address)
{
	unsigned char dataBus = 0;
	/*
	if(address != 0xFF && address != 0xFE)
	{
		printf("IOREQ READ:%x\r\n",address);		
	}
	*/
	switch(address)
	{
		case 0x80:
			#ifdef USE_BASIC
				dataBus = 0xFF;
			#else
				if(receivedFlag == 0)
				{
					dataBus = 0x00;
				}
				else
				{
					dataBus = 0xFF;
				}
			#endif
			break;
		case 0x81:
			while(receivedFlag == 0)
			{
				#ifndef IOTRAP_HUSH
				printf("Help, I'm trapped!\r\n");
				#endif
				_delay_us(25);
			}
			receivedFlag = 0;
			dataBus = receivedBuffer;
			break;
		case 0xFE:
			#ifdef USE_BASIC
				dataBus = 0xE5;
			#else
				printf("!%02x!00!00!\r\n",0xFE); // read a byte.
				dataBus = getchar();
			#endif
			break;
		case 0xFF:
			if(bl_ctr % 0x100 == 0)
			{
				printf("MONITOR:BL_CTR %04lx\r\n",bl_ctr);
			}
			// printf("IOREQ RD %02x\r\n",address);
			if(bl_ctr > sizeof(bl_rom))
			{
				dataBus = 0xFF;
			}
			else
			{
				dataBus = pgm_read_byte(bl_rom + bl_ctr);
			}
			bl_ctr += 1;
			break;
		default:
			printf("IOREQ?RD %02x\r\n",address);
			dataBus = 0;
			break;
	}
	DDRB = 0xFF;
	PORTB = dataBus;
	ioreqClockPulse();
	DDRB = 0x0;
}

void ioreqWrite(int addrBusLow, unsigned char dataBus)
{
	// printf("IOREQ WRITE:%d,%d\r\n",addrBusLow,dataBus);
	switch(addrBusLow)
	{
		case 0x80:
			break;
		case 0x81:
			if(dataBus == '!')
			{
				printf("!!!");
			}
			else
			{
				printf("%c",dataBus);	
			}
			break;
		case 0xF0:
			printf("!%02x!%02x!%02x!\r\n",0xFF,0xF0,dataBus);
			// getchar();
			break;
		case 0xF1:
			printf("!%02x!%02x!%02x!\r\n",0xFF,0xF1,dataBus);
			// getchar();
			break;
		case 0xF2:
			printf("!%02x!%02x!%02x!\r\n",0xFF,0xF1,dataBus);
		    // getchar();
			break;
		case 0xFE:
			printf("MONITOR:DISK_WR (NOT IMPL.)\r\n");
			while(1)
			{
				_delay_ms(1000);
			}
			break;
		case 0xFF:
			switch(dataBus)
			{
				case 0x00:
					printf("MON:HC00:COLDBOOT\r\n");
					break;
				case 0x01:
					printf("MON:HC01:WARMBOOT\r\n");
					break;
				case 0x02:
					printf("MON:HC02:DISK_RD\r\n");
					break;
				case 0x03:
					printf("MON:HC03:DISK_WR\r\n");
					break;
				default:
					printf("MON:UNKNOWN HOSTCALL\r\n");
					break;
			}
			break;
		default:
			printf("IOREQ?WR %02x:%02x\r\n",addrBusLow,dataBus);
			break;
	}
	ioreqClockPulse();
}

int main(void)
{
	DDRD |= (1 << PORTD2) | (1 << PORTD3) | (1 << PORTD4) | (1 << PORTD5) | (1 << PORTD7);
	DDRC = (1 << PORTC7) | (1 << PORTC5) | (1 << PORTC2) |  (1 << PORTC1) | (1 << PORTC0);
	// DDRD &= ~(1 << PORTD0);
	// PORTD7 is ^MREQ (active low)
	// PORTD6 is ^IOREQ
	// PORTD5 is ^WRITE (active low)
	// PORTD4 is ^READ (active low)
	
	// PORTD2 is ^RESET (active low)
	// PORTD3 is CLK
	
	// PORT C0 is LED
	// PORT C3 is M1. (input to atmega, active low - used for interrupt acknowledge.)
	// PORT C6 is WAIT (output to z80, active low)
	// PORT C5 is BUSRQ (output to z80, active low)
	// PORT C4 is BUSACK (input to atmega, active low)
	// PORT C7 is INT (output to z80, active low)
	// PORTA is LOW address
	// PORTC is HIGH address
	ASSERT_BUSRQ_LOW;
	ASSERT_INT_HIGH;
	ASSERT_RESET_LOW;
	_delay_us(25);
	ASSERT_CLK_HIGH;
	_delay_us(25);
	ASSERT_CLK_LOW;
	_delay_us(25);
	ASSERT_CLK_HIGH;
	_delay_us(25);
	ASSERT_CLK_LOW;
	_delay_us(25);
	ASSERT_CLK_HIGH;
	_delay_us(25);
	ASSERT_CLK_LOW;
	
	ASSERT_RWCTRL_INPUT;
	ASSERT_RWCTRL_PULLUPS;
	
	cli();
	
	/*
	DDRC = ~(1 << PORTC4);
	DDRC = ~(1 << PORTC3);
	DDRC = ~(1 << PORTC2);
	*/
	ASSERT_MASKUART_LOW;
	DDRA = 0xFF;
	DDRB = 0xFF;
	uart_init ();
	FILE mystdio = FDEV_SETUP_STREAM(uart_transmit, uart_receive, _FDEV_SETUP_RW);
	stdout = &mystdio;
	stdin = &mystdio;
	
	printf("BOOTLOADER OK\r\n");
	
	unsigned char c = 0;
	int           load_pktlength = 0;
	int           load_pkttype = 0;
	long int      load_address = 0;
	
	if(LOAD_STATIC_PROGRAM)
	{
		writeAndVerifyPattern(0,0xFE);
		printf("Z80 BOOTLOADER WR\r\n");
		writeAndVerify(0,sizeof(tiny_bl),tiny_bl);
	}
	
	while(LOAD_STATIC_PROGRAM == 0)
	{
		c = getchar();
		if(c == ':')
		{
			load_pktlength = grabByte();
			load_address   = (grabByte() << 8) + grabByte();
			load_pkttype   = grabByte();
			unsigned char pktArray[32];
			if(load_pkttype == 0)
			{
				if(load_pktlength > 32)
				{
					printf("ERR:PKTLEN>32\r\n");
					while(1)
					{
						_delay_ms(100);
					}
				}
				int la = 0;
				for(la = 0;la < load_pktlength;la++)
				{
					pktArray[la] = grabByte();
				}
				if(grabByte() != 0)
				{
					printf("ERR:CSUM<>0\r\n");
					while(1)
					{
						_delay_ms(100);
					}
				}
				writeAndVerify(load_address,load_pktlength,pktArray);
			}
			else if(load_pkttype == 1)
			{
				printf("SEND\r\n");
				break;
			}
			else
			{
				printf("ERR:PKTTYPE\r\n");
				while(1)
				{
					_delay_ms(100);
				}
			}
		}
	}
	
	int i = 0;
	
	printf("INT ENABLE\r\n");

	sei();
	// set the bus to 0.
	setAddressBus(0);
	printf("Z80 RESET\r\n");
	
	PORTC = 0xFF;
	DDRA = 0x0;
	PORTA = 0xFF;
	DDRB = 0x0;
	
	DDRD &= ~(1 << PORTD4); // READ
	DDRD &= ~(1 << PORTD5); // WRITE
	DDRD &= ~(1 << PORTD7); // MREQ
	DDRD &= ~(1 << PORTD6); // IORQ
	PORTD |= (1 << PORTD4) | (1 << PORTD5) | (1 << PORTD7) | (1 << PORTD6);
	
	_delay_us(50);
	ASSERT_BUSRQ_HIGH;
	// ASSERT_WAIT_HIGH;
	ASSERT_RESET_LOW;
	unsigned char addrBusLow = 0;
	unsigned char dataBus = 0;
	unsigned char mreqPin = 0;
	unsigned char ioreqPin = 0;
	unsigned char m1Pin = 0;
	for(i = 0;i < 3; i++)
	{
		ASSERT_CLK_HIGH;
		_delay_us(25);
		ASSERT_CLK_LOW;
		_delay_ms(25);
	}
	ASSERT_RESET_HIGH;
	printf("Z80 GO\r\n");
	
	#define MEM_DEBUG 0
	#define IO_DEBUG 1

	while(1)
	{
		if(interruptFlag > 0)
		{
			// printf("INT ASSERTED\r\n");
			ASSERT_INT_LOW;
			interruptFlag -= 1;
		}
		else
		{
			ASSERT_INT_HIGH;
		}
		ASSERT_CLK_HIGH;
		unsigned char statusRegister = PIND;
		unsigned char pincBuffer = PINC;
		
		_delay_us(25);
		ioreqPin = statusRegister & (1 << PORTD6);
		mreqPin = statusRegister & (1 << PORTD7);
		addrBusLow = PINA;
		m1Pin = pincBuffer & (1 << PORTC3);
		dataBus = PINB;

		if(interruptFlag > 0 && m1Pin == 0 && ioreqPin == 0)
		{
			interruptFlag = 0;
			// printf("MONITOR: IOACK, BUSSING\r\n");
			DDRB = 0xFF;
			PORTB = interruptVector;
			ioreqClockPulse();
			/*
			while(m1Pin == 0 && ioreqPin == 0)
			{
				printf("MONITOR: CLK DELAY\r\n");
				ioreqClockPulse();
				unsigned char statusRegister = PIND;
				ioreqPin = statusRegister & (1 << PORTD6);
				mreqPin = statusRegister & (1 << PORTD7);
			}
			*/
			DDRB = 0x00;
		}
		
		if(mreqPin == 0)
		{
			if(MEM_DEBUG)
			{
				printf("%d %d : %d\r\n",mreqPin,addrBusLow,dataBus);	
			}
		}
		else if(ioreqPin == 0)
		{
			if((statusRegister & (1 << PORTD4)) == 0) // io read
			{
				ioreqRead(addrBusLow);
			}
			else if((statusRegister & (1 << PORTD5)) == 0) // io write
			{
				ioreqWrite(addrBusLow,dataBus);
			}
		}
		// if WAIT.
		ASSERT_CLK_LOW;
		_delay_us(25);
	}
}

ISR(USART_RXC_vect)
{
	interruptFlag = 23;
	interruptVector = 4;
	receivedFlag = 1;
	receivedBuffer = UDR; // Fetch the received byte value into the variable "ByteReceived"
}
