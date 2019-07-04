#define F_CPU 8000000UL

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdlib.h>
#include <stdio.h>
#include <util/delay.h>

#define BAUDRATE 9600 // define baud
#define BAUD_PRESCALER ((F_CPU)/(BAUDRATE * 16UL) - 1) // set baud rate value for UBRR

#define TESTPROGRAM_LEN 12
unsigned char TESTPROGRAM[12] = {0x3e,0xaa,0x00,0xf1,0x32,0x00,0xf1,0x3e,0xbb,0xc3,0x00,0x00};

void uart_init (void)
{
	UBRRH = (uint8_t)(BAUD_PRESCALER>>8);
	UBRRL = (uint8_t)(BAUD_PRESCALER);
	UCSRB = (1<<RXEN)|(1<<TXEN);
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

void setAddressBus(long int addr)
{
	// top 4 bits reserved...
	PORTC = (addr >> 8) & 0x0F;
	PORTA = addr & 0xFF;
}

#define ASSERT_MREQ_LOW PORTD &= ~(1 << PORTD7)
#define ASSERT_MREQ_HIGH PORTD |= (1 << PORTD7)
#define ASSERT_WRITE_LOW PORTD &= ~(1 << PORTD5)
#define ASSERT_WRITE_HIGH PORTD |= (1 << PORTD5)
#define ASSERT_READ_LOW PORTD &= ~(1 << PORTD4)
#define ASSERT_READ_HIGH PORTD |= (1 << PORTD4)

#define ASSERT_CLK_LOW PORTD &= ~(1 << PORTD3)
#define ASSERT_CLK_HIGH PORTD |= (1 << PORTD3)
#define ASSERT_RESET_LOW PORTD &= ~(1 << PORTD2)
#define ASSERT_RESET_HIGH PORTD |= (1 << PORTD2)
#define ASSERT_INT_LOW PORTC &= ~(1 << PORTC7)
#define ASSERT_INT_HIGH PORTC |= (1 << PORTC7)

#define ASSERT_BUSRQ_LOW PORTC &= ~(1 << PORTC5)
#define ASSERT_BUSRQ_HIGH PORTC |= (1 << PORTC5)
#define ASSERT_WAIT_LOW PORTC &= ~(1 << PORTC6)
#define ASSERT_WAIT_HIGH PORTC |= (1 << PORTC6)

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

void writeAndVerify(long int addr, int bytes, char *data)
{
	printf("%d to %04x\r\n",bytes,addr);
	int i = 0;
	ASSERT_DATABUS_OUTPUT;
	ASSERT_MREQ_LOW;
	ASSERT_RWCTRL_OUTPUT;
	for(i = addr;i < bytes;i++)
	{
		setAddressBus(i);
		PORTB = data[i];
		ASSERT_WRITE_LOW;
		ASSERT_WRITE_HIGH;
	}
	ASSERT_DATABUS_INPUT;
	
	unsigned char dataCheck;
	for(i =addr;i < bytes;i++)
	{
		setAddressBus(i);
		ASSERT_READ_LOW;
		_delay_us(30);
		dataCheck = PINB;
		ASSERT_READ_HIGH;
		if(dataCheck != data[i])
		{
			printf("ERR: addr %d byte %d\r\n",i,dataCheck);
		}
	}
	ASSERT_MREQ_HIGH;
	ASSERT_RWCTRL_INPUT;
}

int main(void)
{
	DDRD |= (1 << PORTD2) | (1 << PORTD3) | (1 << PORTD4) | (1 << PORTD5) | (1 << PORTD7);
	DDRD &= ~(1 << PORTD0);
	// PORTD7 is ^MREQ (active low)
	// PORTD6 is ^IOREQ
	// PORTD5 is ^WRITE (active low)
	// PORTD4 is ^READ (active low)
	
	// PORTD2 is ^RESET (active low)
	// PORTD3 is CLK
	
	// PORT C6 is WAIT (output to z80, active low)
	// PORT C5 is BUSRQ (output to z80, active low)
	// PORT C4 is BUSACK (input to atmega, active low)
	// PORT C7 is INT (output to z80, active low)
	// PORTA is LOW address
	// PORTC is HIGH address
	
	ASSERT_WAIT_HIGH;
	ASSERT_BUSRQ_LOW;
	ASSERT_RESET_LOW;
	// ASSERT_RESET_HIGH;
	_delay_ms(1);
	// let buses become tristated
	DDRC = ~(1 << PORTC4);
	DDRA = 0xFF;
	DDRB = 0xFF;
	uart_init ();
	FILE mystdio = FDEV_SETUP_STREAM(uart_transmit, uart_receive, _FDEV_SETUP_RW);
	stdout = &mystdio;
	stdin = &mystdio;
	
	ASSERT_RWCTRL_INPUT;
	ASSERT_RWCTRL_PULLUPS;
	
	printf("EEPROM INIT\r\n");
	
	unsigned char c = 0;
	int           load_pktlength = 0;
	int           load_pkttype = 0;
	long int      load_address = 0;
	
	while(1)
	{
		c = getchar();
		if(c == ':')
		{
			load_pktlength = grabByte();
			load_address   = (grabByte() << 8) + grabByte();
			load_pkttype   = grabByte();
			char pktArray[32];
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
				while(1)
				{
					_delay_ms(100);
				}
			}
			else
			{
				printf("???\r\n");
				while(1)
				{
					_delay_ms(100);
				}
			}
		}
	}
	
	int i = 0;
	
	// setAddressBus(0);
	
	printf("RELEASE CONTROL\r\n");
	DDRC = (1 << PORTC7) | (1 << PORTC5) | (1 << PORTC6);
	PORTC = 0xFF;
	DDRA = 0x0;
	PORTA = 0xFF;
	DDRB = 0x0;
	PORTB = 0xFF;
	
	DDRD &= ~(1 << PORTD4); // READ
	DDRD &= ~(1 << PORTD5); // WRITE
	DDRD &= ~(1 << PORTD7); // MREQ
	DDRD &= ~(1 << PORTD6); // IORQ
	PORTD |= (1 << PORTD4) | (1 << PORTD5) | (1 << PORTD7) | (1 << PORTD6);
	
	_delay_ms(500);
	
	printf("PULSE RESET\r\n");
	ASSERT_INT_HIGH;
	ASSERT_BUSRQ_HIGH;
	ASSERT_WAIT_HIGH;
	ASSERT_RESET_LOW;
	unsigned char addrBusLow = 0;
	unsigned char addrBusHigh = 0;
	unsigned char dataBus = 0;
	unsigned char dataPin = 0;
	for(i = 0;i < 3; i++)
	{
		ASSERT_CLK_HIGH;
		_delay_ms(50);
		dataPin = PIND & (1 << PORTD7);
		addrBusLow = PINA;
		addrBusHigh = PINC & 0x0F;
		dataBus = PINB;
		if(dataPin == 0)
		{
			printf("%d %d %d : %d\r\n",dataPin,addrBusHigh,addrBusLow,dataBus);
		}
		ASSERT_CLK_LOW;
		dataPin = PIND & (1 << PORTD7);
		addrBusLow = PINA;
		addrBusHigh = PINC & 0x0F;
		dataBus = PINB;
		if(dataPin == 0)
		{
			printf("%d %d %d : %d\r\n",dataPin,addrBusHigh,addrBusLow,dataBus);
		}
		_delay_ms(50);
	}
	ASSERT_RESET_HIGH;
	printf("GO!\r\n");
	
	_delay_ms(1000);

	while(1)
	{
		ASSERT_CLK_HIGH;
		_delay_ms(50);
		dataPin = PIND & (1 << PORTD7);
		addrBusLow = PINA;
		addrBusHigh = PINC & 0x0F;
		dataBus = PINB;
		if(dataPin == 0)
		{
			printf("%d %d %d : %d\r\n",dataPin,addrBusHigh,addrBusLow,dataBus);
		}
		ASSERT_CLK_LOW;
		dataPin = PIND & (1 << PORTD7);
		addrBusLow = PINA;
		addrBusHigh = PINC & 0x0F;
		dataBus = PINB;
		if(dataPin == 0)
		{
			printf("%d %d %d : %d\r\n",dataPin,addrBusHigh,addrBusLow,dataBus);
		}
		_delay_ms(50);
	}
}

