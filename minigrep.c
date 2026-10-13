#define _POSIX_C_SOURCE 200809L
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#define BUF_SIZE 4096

void printUsage() {
    fprintf(stderr, "Uso: ./minigrep -r REGEX [-s BUFSIZE] [-v] [-n] [-o] [-h]\n");
    fprintf(stderr, "\t-r REGEX Expresion regular.\n");
    fprintf(stderr, "\t-s BUFSIZE Tamaño de los buffers de lectura y escritura en bytes (por defecto, 1024).\n");
    fprintf(stderr, "\t-v Muestra las líneas que NO sean reconocidas por la expresión regular.\n");
    fprintf(stderr, "\t-n Antepone a cada línea mostrada su número de línea en la entrada.\n");
    fprintf(stderr, "\t-o Muestra solo la parte de cada línea reconocida por la expresión regular.\n");
}


int main(int argc, char *argv[]) {
    int opt;
    char *s = NULL;

    optind = 1;
    while ((opt = getopt(argc, argv, "r:s:vnoh")) != -1) {
        switch (opt) {
            case 'r':
                fprintf(stdout, "Opción -r");
                break;
            case 's':
                fprintf(stdout, "Opción -s");
                break;
            case 'v':
                fprintf(stdout, "Opción -v");
                break;
            case 'n':
                fprintf(stdout, "Opción -n");
                break;
            case 'o':
                fprintf(stdout, "Opción -o");
                break;
            case 'h':
            default:
                printUsage();
                exit(EXIT_FAILURE);
        }
    }

    if (optind < argc) {
        printUsage();
        exit(EXIT_FAILURE);
    }

    return EXIT_SUCCESS;
}