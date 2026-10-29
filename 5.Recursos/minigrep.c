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
#define BUFFER_MIN 1
#define BUFFER_MAX 1048576
#define MAX_LINE_SIZE 4096

void printUsage(int num) {
    FILE *exitProgram;

    if (num == EXIT_SUCCESS) {
        exitProgram = stdout;
    }

    else {
        exitProgram = stderr;
    }

    fprintf(exitProgram, "Uso: ./minigrep -r REGEX [-s BUFSIZE] [-v] [-n] [-o] [-h]\n");
    fprintf(exitProgram, "\t-r REGEX Expresión regular.\n");
    fprintf(exitProgram, "\t-s BUFSIZE Tamaño de los buffers de lectura y escritura en bytes (por defecto, 1024).\n");
    fprintf(exitProgram, "\t-v Muestra las líneas que NO sean reconocidas por la expresión regular.\n");
    fprintf(exitProgram, "\t-n Antepone a cada línea mostrada su número de línea en la entrada.\n");
    fprintf(exitProgram, "\t-o Muestra solo la parte de cada línea reconocida por la expresión regular.\n\n");

    exit(num);
}

ssize_t write_all(int fd, char* buf, ssize_t num_read) {
    ssize_t num_written = 0;
    ssize_t num_left = num_read;
    char* buf_left = buf;
    
    if (buf[0] == '\0') {
        return 0;
    }

    while (num_left > 0) {
        num_written = write(fd, buf_left, num_left);

        if(num_written == -1) {
            fprintf(stderr, "ERROR: write()\n");
            exit(EXIT_FAILURE);
        }

        num_left -= num_written;
        buf_left += num_written;
    }

    if (write(fd, "\n", 1) == -1) {
        fprintf(stderr, "ERROR: write()\n");
        exit(EXIT_FAILURE);
    }

    return num_written;
}

size_t leerER(char *r, char *bufferL, char *bufferE, size_t total, int flagV) {
    regex_t regex;
    regmatch_t match[1];
    int flag;

    if (regcomp(&regex, r, REG_EXTENDED | REG_NEWLINE) != 0) {
        fprintf(stderr, "ERROR: REGEX mal construida\n");
        exit(EXIT_FAILURE);
    }

    size_t inicio = 0;
    size_t i = 0;

    while (i < total) {
        //while (i < total && bufferL[i] != '\n' && bufferL[i] != '\0') {
        while (i < total && bufferL[i] != '\n') {
            i++;
        }

        if (i >= total) {
            break;
        }

        size_t longitud = i - inicio;

        if (longitud + 1 > MAX_LINE_SIZE) {
            fprintf(stderr, "ERROR: Línea demasiado larga\n");
            exit(EXIT_FAILURE);
        }

        memcpy(bufferE, bufferL + inicio, longitud);
        bufferE[longitud] = '\0';

        int ret = regexec(&regex, bufferE, 0, NULL, 0);

        if ((flagV == 0 && ret == 0) || (flagV == 1 && ret == REG_NOMATCH)) {
            write_all(STDOUT_FILENO, bufferE, (ssize_t)longitud);
        }

        i++;
        inicio = i;
    }

    regfree(&regex);

    return total - inicio;
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

    if (argc == 1) {
        fprintf(stderr, "ERROR: REGEX vacía\n");
        exit(EXIT_FAILURE);
    }

    if (s < BUFFER_MIN || s > BUFFER_MAX) {
        fprintf(stderr, "ERROR: BUFSIZE debe ser mayor que 0 y menor que o igual a 1 MB\n");
        exit(EXIT_FAILURE);
    }

    char *bufferL = malloc(s);
    char *bufferE = malloc(s);

    if (bufferL == NULL || bufferE == NULL) {
        fprintf(stderr, "ERROR: malloc()\n");
        exit(EXIT_FAILURE);
    }

    ssize_t bytes_leidos = 0;
    size_t bytes_pendientes = 0;

    while ((bytes_leidos = read(STDIN_FILENO, bufferL + bytes_pendientes, s - bytes_leidos)) > 0) {
        size_t total = bytes_pendientes + bytes_leidos;
        bufferL[total] = '\0';

        if (r != NULL) {
            size_t bytes_sobrantes = leerER(r, bufferL, bufferE, total, flagV);

            if (bytes_sobrantes > 0) {
                memmove(bufferL, bufferL + (total - bytes_sobrantes), bytes_sobrantes);
            }

            bytes_pendientes = bytes_sobrantes;
        }

        if (bytes_pendientes >= s) {
            fprintf(stderr, "ERROR: Línea demasiado larga\n");
            free(bufferL);
            free(bufferE);
            exit(EXIT_FAILURE);
        }
    }

    if (bytes_leidos == -1) {
        fprintf(stderr, "ERROR: read()\n");
        free(bufferL);
        free(bufferE);
        exit(EXIT_FAILURE);
    }

    free(bufferL);
    free(bufferE);

    return EXIT_SUCCESS;

    /*
    ssize_t bytes = 0;

    if (!isatty(STDIN_FILENO)) {
        bytes = read(STDIN_FILENO, bufferL, s);
    }

    if (bytes == -1) {
        fprintf(stderr, "ERROR: read()\n");
        free(bufferL);
        exit(EXIT_FAILURE);
    }

    bufferL[bytes] = '\0';

    if (r != NULL) {
        leerER(r, bufferL, bufferE, flagV);
    }

    free(bufferL);
    free(bufferE);

    return EXIT_SUCCESS;*/
}
