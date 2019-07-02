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

void setDataBus(char data)
{
		
}

void readDataBus(char data)
{
	
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
	ASSERT_CLK_HIGH;
	
	uart_init ();
	FILE mystdio = FDEV_SETUP_STREAM(uart_transmit, uart_receive, _FDEV_SETUP_RW);
	stdout = &mystdio;
	stdin = &mystdio;
	
		ASSERT_DATABUS_OUTPUT;
		ASSERT_WRITE_LOW;
		setAddressBus(0x0000);
		PORTB = 0x41;
		ASSERT_MREQ_LOW;
		_delay_ms(50);
		ASSERT_MREQ_HIGH;
		
		setAddressBus(0xFF00);
		PORTB = 0x42;
		ASSERT_MREQ_LOW;
		_delay_ms(50);
		ASSERT_MREQ_HIGH;
		ASSERT_WRITE_HIGH;
		_delay_ms(50);
		
		ASSERT_DATABUS_INPUT;
		ASSERT_READ_LOW;
		setAddressBus(0xFF00);
		ASSERT_MREQ_LOW;
		_delay_ms(50);
		unsigned char a = PINB;
		ASSERT_MREQ_HIGH;
		setAddressBus(0x0000);
		ASSERT_MREQ_LOW;
		_delay_ms(50);
		unsigned char b = PINB;
		ASSERT_MREQ_HIGH;
		ASSERT_READ_HIGH;
	
	printf("Releasing pin control\r\n");
	DDRC = (1 << PORTC7);   // leave C7 for interrupt
	DDRA = 0x0;
	DDRB = 0x0;
	
	DDRD &= ~(1 << PORTD4); // READ
	DDRD &= ~(1 << PORTD5); // WRITE
	DDRD &= ~(1 << PORTD7); // MREQ
	DDRD &= ~(1 << PORTD6); // IORQ
	
	_delay_ms(5000);
	
	printf("Pulsing reset\r\n");
	ASSERT_RESET_HIGH;
	_delay_ms(100);
	ASSERT_RESET_LOW;
	printf("Generating 4 clock cycles\r\n");
	int i = 0;
	for(i = 0;i < 4; i++)
	{
		ASSERT_CLK_LOW;
		_delay_ms(200);
		ASSERT_CLK_HIGH;
		_delay_ms(200);
	}
	ASSERT_RESET_HIGH;
	while(1)
	{
		ASSERT_CLK_HIGH;
		_delay_ms(100);
		ASSERT_CLK_LOW;	
		_delay_ms(100);		
	}
}

