#include <ioport.h>

unsigned short const port_c = 0x3f8;
void setup(void) 
{
	out8(port_c + 1, (uint8_t) 0); /*отключаем контроллер прерываний */
	out8(port_c + 3, (uint8_t) 128); /*переключили в порте 3 на запись div 128 */ 
	out8(port_c, (uint8_t) 12);	/* младший байт div в порт +0 12 */
	out8(port_c + 1, (uint8_t) 0); /* старший байт в порт +1 */
	out8(port_c + 3, (uint8_t) 7); /* 00000111 формат кадра 7 */
	
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
