#include <ioport.h>

#define SERIAL_BASE_PORT	0x3f8
#define SERIAL_PORT(x)		(SERIAL_BASE_PORT + (x))
#define REG_DATA		SERIAL_PORT(0)
#define REG_DLL			SERIAL_PORT(0)
#define REG_IER			SERIAL_PORT(1)
#define REG_DLH			SERIAL_PORT(1)
#define REG_LCR			SERIAL_PORT(3)
#define REG_LSR			SERIAL_PORT(5)

#define LCR_8BIT		(3 << 0)
#define LCR_DLAB		(1 << 7)
#define LSR_TX_READY		(1 << 5)



unsigned short const port_c = 0x3f8;
void setup(void) 
{
	out8(port_c + 1, (uint8_t) 0); /*отключаем контроллер прерываний */
	out8(port_c + 3, (uint8_t) 128); /*переключили в порте 3 на запись div 128 */ 
	out8(port_c, (uint8_t) 12);	/* младший байт div в порт +0 12 */
	out8(port_c + 1, (uint8_t) 0); /* старший байт в порт +1 */
	out8(port_c + 3, (uint8_t) 7); /* 00000111 формат кадра 7 */
	
}

void serial_putchar(int c)
{
	while (!(in8(REG_LSR) & LSR_TX_READY));
	out8(REG_DATA, c);
}

void serial_write(const char *buf, size_t size)
{
	for (size_t i = 0; i != size; ++i)
		serial_putchar(buf[i]);
}
void send_info(char data[]) 
{	int i = 0;
	while (data[i] != '\0') {
	while ((in8(port_c +  5) >> 5 ) % 2 == 0) {
	}
	out8(port_c, data[i]);
	i += 1;
	}
}
void send_int_info(uint64_t var) {/*TODO*/
	while (var > 0) {
	const char c [] = {(var % 10) + '0'};
	var = var / 10;
	out8(port_c, c[0]);
}
}
