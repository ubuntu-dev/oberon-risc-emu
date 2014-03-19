#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

#include "risc-serial.h"

#define RISC_SERIAL_INPUT "/tmp/risc-serial-input"
#define RISC_SERIAL_OUTPUT "/tmp/risc-serial-output"

struct Serial {
    bool rx_available;
    bool tx_ready;
    int ch;
    int input;
    int output;
};

struct Serial *serial_new() {
    struct Serial *serial = calloc(1, sizeof(*serial));
    serial->tx_ready = true;
    serial->rx_available = false;
    serial->input = open(RISC_SERIAL_INPUT, O_RDONLY | O_NONBLOCK);
    if (serial->input == -1) {
        if (mknod(RISC_SERIAL_INPUT, S_IFIFO | 0660, 0) == -1) {
            fprintf(stderr, "Can't make named pipe \"%s\": %s\n", RISC_SERIAL_INPUT, strerror(errno));
        }
        serial->input = open(RISC_SERIAL_INPUT, O_RDONLY | O_NONBLOCK);
        if (serial->input == -1) {
            fprintf(stderr, "Can't open file \"%s\": %s\n", RISC_SERIAL_INPUT, strerror(errno));
            exit(1);
        }
    }

    serial->output = open(RISC_SERIAL_OUTPUT, O_RDWR | O_NONBLOCK);
    if (serial->output == -1) {
        if (mknod(RISC_SERIAL_OUTPUT, S_IFIFO | 0660, 0) == -1) {
            fprintf(stderr, "Can't make named pipe \"%s\": %s\n", RISC_SERIAL_OUTPUT, strerror(errno));
        }
        serial->output = open(RISC_SERIAL_OUTPUT, O_RDWR | O_NONBLOCK);
        if (serial->output == -1) {
            fprintf(stderr, "Can't open file \"%s\": %s\n", RISC_SERIAL_OUTPUT, strerror(errno));
            exit(1);
        }        
    }
    return serial;
}

void serial_free(struct Serial *serial) {
    if (close(serial->input)==-1) {
        fprintf(stderr, "Can't close file \"%s\": %s\n", RISC_SERIAL_INPUT, strerror(errno));
        exit(1);
    };
    if (close(serial->output)==-1) {
        fprintf(stderr, "Can't close file \"%s\": %s\n", RISC_SERIAL_OUTPUT, strerror(errno));
        exit(1);        
    };
    free(serial);
}

uint32_t serial_stat(struct Serial *serial) {
    if (!serial->rx_available) { // nothing received yet
        char ch;
        int res = read(serial->input, &ch, 1);
        if (res==1) {
            serial->ch = ch;
            serial->rx_available = true;
        }
    }
    return (serial->tx_ready ? 2 : 0) | (serial->rx_available ? 1 : 0);
}

uint32_t serial_receive(struct Serial *serial) {
    uint32_t recv;
    if (serial_stat(serial) & 1) { // char available
        serial->rx_available = false;
        recv = serial->ch;
    } else {
        recv= -1;
    }
    
    return recv;
}


void serial_send(struct Serial *serial, uint32_t value) {
    char ch=value;
    int err=write(serial->output, &ch, 1);
    if (err==-1) {
        fprintf(stderr, "Can't send via serial line: \"%s\"\n", strerror(errno));
    }
    err=fsync(serial->output);
    if (err==-1) {
        fprintf(stderr, "Can't flush serial line: \"%s\"\n", strerror(errno));
    }
}