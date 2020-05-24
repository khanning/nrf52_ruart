void usbserial_init(void);
uint8_t ugetc(void);
void uputc(uint8_t);
void print_num(uint16_t);
void upoll(void);
uint8_t uavail(void);
void print_string(char *buffer, ...);
void print_buffer(uint8_t *, int16_t);
