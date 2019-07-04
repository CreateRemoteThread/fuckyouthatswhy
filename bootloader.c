#define F_CPU 8000000UL

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdlib.h>
#include <stdio.h>
#include <util/delay.h>

#define BAUDRATE 9600 // define baud
#define BAUD_PRESCALER ((F_CPU)/(BAUDRATE * 16UL) - 1) // set baud rate value for UBRR

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

unsigned char uart_receive( void )
{
	while(!(UCSRA & (1<<RXC)));
	return UDR;
}

void setAddressBus(long int addr)
{
	PORTC = (addr >> 8) & 0x7F;
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

#define ASSERT_DATABUS_INPUT DDRB = 0x00
#define ASSERT_DATABUS_OUTPUT DDRB = 0xFF

int main(void)
{
	DDRC = 0xFF;
	DDRA = 0xFF;
	DDRB = 0xFF;
	DDRD |= (1 << PORTD2) | (1 << PORTD3) | (1 << PORTD4) | (1 << PORTD5) | (1 << PORTD7);
	
	// PORTD7 is ^MREQ (active low)
	// PORTD6 is ^IOREQ
	// PORTD5 is ^WRITE (active low)
	// PORTD4 is ^READ (active low)
	
	// PORTD2 is ^RESET (active low)
	// PORTD3 is CLK
	
	// PORTC7 is INT
	// PORTA is LOW address
	// PORTC is HIGH address
	
	ASSERT_RESET_LOW;
	ASSERT_CLK_LOW;
	
	uart_init ();
	FILE mystdio = FDEV_SETUP_STREAM(uart_transmit, uart_receive, _FDEV_SETUP_RW);
	stdout = &mystdio;
	stdin = &mystdio;
	
	int i = 0;
	printf("Boot OK, flashing virtual EEPROM...\n");
	ASSERT_DATABUS_OUTPUT;
	ASSERT_WRITE_LOW;
	for(i = 0;i < 0xFF;i++)
	{
		setAddressBus(i);
		PORTB = 0x0;
		ASSERT_MREQ_LOW;
		_delay_ms(5);
		ASSERT_MREQ_HIGH;
		_delay_ms(5);
	}
	ASSERT_WRITE_HIGH;
	printf("Verifying virtual EEPROM...\n");
	ASSERT_DATABUS_INPUT;
	ASSERT_READ_LOW;
	unsigned char dataCheck;
	for(i = 0;i < 0xFF;i++)
	{
		setAddressBus(i);
		ASSERT_MREQ_LOW;
		_delay_ms(5);
		dataCheck = PINB;
		if(dataCheck != 0x00)
		{
			printf("ERR: addr %d byte %d\r\n",i,dataCheck);
		}
		ASSERT_MREQ_HIGH;
		_delay_ms(5);
	}
	ASSERT_READ_HIGH;
	
	setAddressBus(0);
	PORTB = 0;
	
	printf("Releasing pin control\r\n");
	// all with pullups. source: https://cdn.hackaday.io/files/19000812896000/S221116_R100218_Z80.ino
	DDRC = (1 << PORTC7);   // leave C7 for interrupt
	PORTC = ~(1 << PORTC7);
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
	
	printf("Pulsing reset\r\n");
	ASSERT_INT_HIGH;
	ASSERT_RESET_LOW;
	unsigned char addrBusLow = 0;
	unsigned char addrBusHigh = 0;
	unsigned char dataBus = 0;
	unsigned char dataPin = 0;
	
	printf("Generating 4 clock cycles\r\n");
	for(i = 0;i < 3; i++)
	{
		ASSERT_CLK_HIGH;
		_delay_ms(50);
		dataPin = PIND & (1 << PORTD7);
		addrBusLow = PINA;
		addrBusHigh = PINC & 0x7F;
		dataBus = PINB;
		printf("%d %d %d : %d\r\n",dataPin,addrBusHigh,addrBusLow,dataBus);
		ASSERT_CLK_LOW;
		dataPin = PIND & (1 << PORTD7);
		addrBusLow = PINA;
		addrBusHigh = PINC & 0x7F;
		dataBus = PINB;
		printf("%d %d %d : %d\r\n",dataPin,addrBusHigh,addrBusLow,dataBus);
		_delay_ms(50);
	}
	ASSERT_RESET_HIGH;
	printf("It's alive!\r\n");

	while(1)
	{
		ASSERT_CLK_HIGH;
		_delay_ms(50);
		dataPin = PIND & (1 << PORTD7);
		addrBusLow = PINA;
		addrBusHigh = PINC & 0x7F;
		dataBus = PINB;
		printf("%d %d %d : %d\r\n",dataPin,addrBusHigh,addrBusLow,dataBus);
		ASSERT_CLK_LOW;
		dataPin = PIND & (1 << PORTD7);
		addrBusLow = PINA;
		addrBusHigh = PINC & 0x7F;
		dataBus = PINB;
		printf("%d %d %d : %d\r\n",dataPin,addrBusHigh,addrBusLow,dataBus);
		_delay_ms(50);
	}
}

