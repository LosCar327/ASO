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

typedef struct {
    int opt;
    int n;
} Entrada;

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
}

void optionR(int s, char *buffer) {
    regex_t regex;
    int a = regcomp(&regex, optarg, REG_NEWLINE | REG_EXTENDED);
    char *exp = optarg;
    regmatch_t b[1];


    if (regcomp(&regex, exp, REG_EXTENDED | REG_NEWLINE) == 0) {
        if (regexec(&regex, buffer, 1, b, 0) == 0) {
            int longitud = b[0].rm_eo - b[0].rm_so;
            char *resultado = malloc((size_t)longitud+1);
            memcpy(resultado, buffer + b[0].rm_so, (size_t)longitud);
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

int comprobando(int opt, int argc, Entrada *ent) {
    for (int i = 0; i < argc-1; i++) {
        if (opt == ent[i].opt) {
            return 1;
        }
    }

    return 0;
}

int main(int argc, char *argv[]) {
    int opt, s = BUF_SIZE;
    char *buffer = malloc(sizeof(int) * s);
    ssize_t n;
    Entrada *ent = malloc(sizeof(Entrada) * (argc-1));
    char **prueba = malloc(sizeof(char *) * argc);

    for (int i = 0; i < argc-1; i++) {
        prueba[i] = malloc(strlen(argv[i + 1]) + 1);
        strcpy(prueba[i], argv[i + 1]);
    }

    int na = 0;

    for (int i = 0; i < argc-1; i++) {
        int res = 0;

        for (int j = 0; j < na; j++) {
            if (atoi(prueba[i]) == ent[j].opt) {
                ent[j].n++;
                res = 1;
                break;
            }
        }

        if (res == 0) {
            ent[na].opt = atoi(prueba[i]);
            ent[na].n = 0;
            na++;
        }
    }

    free(prueba);

    n = read(STDIN_FILENO, buffer, s-1);

    if (n == -1) {
        free(ent);
        free(buffer);
        exit(EXIT_FAILURE);
    }

    buffer[n] = '\0';

    optind = 1;
    while ((opt = getopt(argc, argv, "r:s:vnoh")) != -1) {
        switch (opt) {
            case 'r':
                optionR(s, buffer);
                break;
            case 's':
		        s = atoi(optarg);
                break;
            case 'v':
                break;
            case 'n':
                break;
            case 'o':
                break;
            case 'h':
                printUsage(EXIT_SUCCESS);
                free(ent);
                free(buffer);
                exit(EXIT_SUCCESS);
                break;
            default:
                printUsage(EXIT_FAILURE);
                free(ent);
                free(buffer);
                exit(EXIT_FAILURE);
                break;
        }
    }

    if (optind < argc) {
        printUsage(EXIT_FAILURE);
    }

    free(ent);
    free(buffer);

    return EXIT_SUCCESS;
}
