#define _POSIX_C_SOURCE 200809L
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#define BUF_SIZE 4096
// Parámetros default del ejemplo de ejecución
#define DEFAULT_SECONDS 1
#define MIN_SECONDS 1
#define MAX_SECONDS 60
#define DEFAULT_LOG "/tmp/watchdir.log"
#define DEFAULT_DIR "."

/*
Los argumentos de la línea de órdenes debe procesarse con getopt() y no podrá suponerse un orden
concreto de dichos argumentos. Además podrá utilizarse más de una vez un argumento, en cuyo caso
se tomará la última aparición del mismo como la correcta.
*/

char *dir_name = DEFAULT_DIR;
char *log_name = DEFAULT_LOG;

// Siguiendo el esquema de minigrep.c, se implementa la función printUsage() para mostrar la ayuda del programa.
// stdout si es -h y stderr si es error

void printUsage(int num) {

    FILE *exitProgram;

    if (num == EXIT_SUCCESS) {
        exitProgram = stdout;
    }

    else {
        exitProgram = stderr;
    }

    fprintf(exitProgram, "Usage: ./watchdir [-n SECONDS] [-l LOG] [DIR]\n");
    fprintf(exitProgram, "\tSECONDS Refresh rate in [1..60] seconds [default: 1].\n");
    fprintf(exitProgram, "\tLOG\tLog file [default: '/tmp/watchdir.log'].\n");
    fprintf(exitProgram, "\tDIR\tDirectory name [default: '.'].\n");
}

// Convierte el texto de -n en un entero y comprueba que está en [1..60].
// atoi() devuelve 0 si el texto no es un número, y 0 queda fuera del rango

static int parse_seconds(const char *s) {
    int value = atoi(s);

    if (value < MIN_SECONDS || value > MAX_SECONDS) {
        printUsage(EXIT_FAILURE);
    }
    return value;
}

int main(int argc, char *argv[]) {
    int opt;
    int seconds = DEFAULT_SECONDS;

    optind = 1;
    while ((opt = getopt(argc, argv, "n:l:h")) != -1) {
        switch (opt) {
            case 'n':
                seconds = parse_seconds(optarg);
                break;
            case 'l':
                log_name = optarg;
                break;
            case 'h':
                printUsage(EXIT_SUCCESS);
            default:
                printUsage(EXIT_FAILURE);
        }
    }

    // Como mucho un DIR posicional (el resto de argumentos ya los usó getopt)
    if (optind < argc - 1) {
        printUsage(EXIT_FAILURE);
    }

    if (optind == argc - 1) {
        dir_name = argv[optind];
    }

    return EXIT_SUCCESS;
}
