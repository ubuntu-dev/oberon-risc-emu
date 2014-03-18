#include <stdbool.h>
#include <stdint.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include "risc-serial.h"

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
    serial->input = open("risc-input", O_RDONLY | O_NONBLOCK);
    printf("serial->input = %x\n", serial->input);
    serial->output = open("risc-output", O_RDWR | O_NONBLOCK); // setvbuf(serial->output, NULL, _IONBF, 0);
    printf("serial->output = %x\n", serial->output);
    return serial;
}

void serial_free(struct Serial *serial) {
    close(serial->input);
    close(serial->output);
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
        printf("recv=%d\n", recv); fflush(stdout);
    } else {
        recv= -1;
    }
    
    return recv;
}


void serial_send(struct Serial *serial, uint32_t value) {
    printf("send %d\n", value); fflush(stdout);
    char ch=value;
    int err=write(serial->output, &ch, 1);
    printf("done sending. Result=%d\n", err);
    if (err==-1) {
        printf ("errno = %d\n", errno);
    }
    err=fsync(serial->output);
    if (err!=0) {
        printf ("fsync errno = %d\n", errno);
    }
    
     fflush(stdout);
}