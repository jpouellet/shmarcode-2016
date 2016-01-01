BINARY = shmarcode
OBJS = sdram.o lcd.o clock.o
OPENCM3_DIR = libopencm3
LDSCRIPT = stm32f429i-discovery.ld
include libopencm3.target.mk
