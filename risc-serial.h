#ifndef RISC_SERIAL_H
#define RISC_SERIAL_H

#include <stdint.h>

struct Serial;

struct Serial *serial_new();
void serial_free(struct Serial *serial);

uint32_t serial_stat(struct Serial *serial);
uint32_t serial_receive(struct Serial *serial);
void serial_send(struct Serial *serial, uint32_t value);

#endif  // RISC_SERIAL_H
