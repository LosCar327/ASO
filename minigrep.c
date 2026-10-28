#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <regex.h>

#define BUF_SIZE 1024

void printUsage(int num) {
    FILE *exitProgram;

    if (num == EXIT_SUCCESS) {
        exitProgram = stdout;
    }

    else {
        exitProgram = stderr;
    }

    fprintf(exitProgram, "Uso: ./minigrep -r REGEX [-s BUFSIZE] [-v] [-n] [-o] [-h]\n");
    fprintf(exitProgram, "\t-r REGEX Expresion regular.\n");
    fprintf(exitProgram, "\t-s BUFSIZE Tamaño de los buffers de lectura y escritura en bytes (por defecto, 1024).\n");
    fprintf(exitProgram, "\t-v Muestra las líneas que NO sean reconocidas por la expresión regular.\n");
    fprintf(exitProgram, "\t-n Antepone a cada línea mostrada su número de línea en la entrada.\n");
    fprintf(exitProgram, "\t-o Muestra solo la parte de cada línea reconocida por la expresión regular.\n");

    exit(num);
}

void printParametrosNoValidos(int argc, char **argv) {
    fprintf(stderr, "Hay parametro nos validos. Uso: %s [-t] [-x NUMERO] [-s STRING] [-y NUMERO]\n", argv[0]);

    for (int i = optind; i < argc; i++) {
        fprintf(stdout, "param%d: \"%s\", ", i - optind, argv[i]);
    }

    fprintf(stdout, "\n");
    exit(EXIT_FAILURE);
}

void leerER(char *r, char *buffer) {
    regex_t regex;
    int a = regcomp(&regex, optarg, REG_NEWLINE | REG_EXTENDED);
    regmatch_t match[1];

    if (regcomp(&regex, r, REG_EXTENDED | REG_NEWLINE) == 0) {
        if (regexec(&regex, buffer, 1, match, 0) == 0) {
            int longitud = match[0].rm_eo - match[0].rm_so;
            char *resultado = malloc((size_t)longitud+1);
            memcpy(resultado, buffer + match[0].rm_so, (size_t)longitud);
            resultado[longitud] = '\0';

            fprintf(stdout, "%s\n", resultado);

            free(resultado);
        }
    }

    else {
        fprintf(stdout, "No coincide");
    }

    regfree(&regex);
}

void cambiarSalto() {
    // La idea sería leer la entrada para cambiar \n por \0 de la entrada recibida
}

int main(int argc, char **argv) {
    int opt, s = BUF_SIZE, flagV = 0, flagN = 0, flagO = 0;
    char *r = NULL;
    
    optind = 1;
    while ((opt = getopt(argc, argv, "r:s:vnoh")) != -1) {
        switch (opt) {
            case 'r':
                r = optarg;
                break;
            case 's':
		        s = atoi(optarg);
                break;
            case 'v':
                flagV = 1;
                break;
            case 'n':
                flagN = 1;
                break;
            case 'o':
                flagO = 1;
                break;
            case 'h':
                printUsage(EXIT_SUCCESS);
                break;
            default:
                printUsage(EXIT_FAILURE);
                break;
        }
    }

    if (optind < argc) {
        printParametrosNoValidos(argc, argv);
    }

    //fprintf(stdout, "r: \"%s\", s: %d, flagV: %d, flagN: %d, flagO: %d\n", r, s, flagV, flagN, flagO);

    char *buffer = malloc(s);
    ssize_t bytes = read(STDIN_FILENO, buffer, sizeof(buffer));

    if (bytes == -1) {
        perror("read");
        free(buffer);
        return EXIT_FAILURE;
    }

    buffer[bytes] = '\0';

    if (r != NULL) {
        leerER(r, buffer);
    }

    free(buffer);

    return EXIT_SUCCESS;
}
