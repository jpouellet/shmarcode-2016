/*
 * Copyright (C) 2009 Uwe Hermann <uwe@hermann-uwe.de>
 * Copyright (C) 2011 Stephen Caudle <scaudle@doceme.com>
 * Copyright (C) 2012 Daniel Serpell <daniel.serpell@gmail.com>
 * Copyright (C) 2015 Piotr Esden-Tempski <piotr@esden.net>
 * Copyright (C) 2015 Chuck McManis <cmcmanis@mcmanis.com>
 * Copyright (C) 2016 Jean-Philippe Ouellet <jpo@vt.edu>
 *
 * This library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <ctype.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/usart.h>
#include <libopencm3/cm3/systick.h>
#include "clock.h"
#include "sdram.h"
#include "lcd.h"

/* utility functions */
void uart_putc(char c);
int _write(int fd, char *ptr, int len);

static void
gpio_setup(void)
{
	/* Setup GPIO pin GPIO13 on GPIO port G for LED. */
	rcc_periph_clock_enable(RCC_GPIOG);
	gpio_mode_setup(GPIOG, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO13);

	/* Setup GPIO D pins for the USART2 function */
	rcc_periph_clock_enable(RCC_GPIOD);
	rcc_periph_clock_enable(RCC_USART2);
	gpio_mode_setup(GPIOD, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO5);
	gpio_set_af(GPIOD, GPIO_AF7, GPIO5);

	usart_set_baudrate(USART2, 115200);
	usart_set_databits(USART2, 8);
	usart_set_stopbits(USART2, USART_STOPBITS_1);
	usart_set_mode(USART2, USART_MODE_TX_RX);
	usart_set_parity(USART2, USART_PARITY_NONE);
	usart_set_flow_control(USART2, USART_FLOWCONTROL_NONE);
	usart_enable(USART2);
}

static void
update_frame(void)
{
	int x, y;
	uint16_t pixel;

	for (x = 0; x < LCD_WIDTH; x++) {
		for (y = 0; y < LCD_HEIGHT; y++) {
			pixel = 0;
			if (!(x & 0xf) || !(y & 0xf))
				pixel = 0xffff;
			lcd_draw_pixel(x, y, pixel);
		}
	}
}

int
main(void)
{
	/* Clock setup */
	clock_setup();
	/* USART and GPIO setup */
	gpio_setup();
	/* Enable the SDRAM attached to the board */
	sdram_init();
	/* Enable the LCD attached to the board */
	lcd_init();

	printf("System initialized.\n");

	for (;;) {
		/* Blink the LED (PG13) on the board with each fractal drawn. */
		gpio_toggle(GPIOG, GPIO13);	/* LED on/off */
		update_frame();
		lcd_show_frame();		/* show it */
	}

	return 0;
}

/*
 * uart_putc
 *
 * This pushes a character into the transmit buffer for
 * the channel and turns on TX interrupts (which will fire
 * because initially the register will be empty.) If the
 * ISR sends out the last character it turns off the transmit
 * interrupt flag, so that it won't keep firing on an empty
 * transmit buffer.
 */
void
uart_putc(char c)
{
	while ((USART_SR(USART2) & USART_SR_TXE) == 0);
	USART_DR(USART2) = c;
}

/*
 * Called by libc stdio functions
 */
int
_write(int fd, char *ptr, int len)
{
	int i = 0;

	/*
	 * Write "len" of char from "ptr" to file id "fd"
	 * Return number of char written.
	 */
	if (fd > 2) {
		return -1;  /* STDOUT, STDIN, STDERR */
	}
	while (*ptr && (i < len)) {
		uart_putc(*ptr);
		if (*ptr == '\n') {
			uart_putc('\r');
		}
		i++;
		ptr++;
	}
	return i;
}
