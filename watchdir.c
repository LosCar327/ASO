#define _POSIX_C_SOURCE 200809L
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#define BUF_SIZE 4096

void printUsage() {
    printf("Usage: ./watchdir [-n SECONDS] [-l LOG] [DIR]\n");
    printf("\tSECONDS Refresh rate in [1..60] seconds [default: 1].\n");
    printf("\tLOG\tLog file [default: '/tmp/watchdir.log'].\n");
    printf("\tDIR\tDirectory name [default: '.'].\n");
}

// Prueba commit rama

int main(int argc, char *argv[]) {
    printUsage();
}