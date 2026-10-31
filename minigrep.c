#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
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

ssize_t read_all(char *recept_buf, ssize_t recept_buf_size) {
    ssize_t total = 0;
    ssize_t num_readed = 0;

    while (total < recept_buf_size) {
        num_readed = read(STDIN_FILENO, recept_buf + total, recept_buf_size - total);

        if (num_readed == 0) {
            break;
        }

        if (num_readed == -1) {
            return -1;
        }

        total = total + num_readed;
    }

    if (recept_buf_size > 0) {
        recept_buf[total] = '\0';
    }

    return total;
}


ssize_t write_all(int fd, char* lineBuffer, size_t total, char* writeBuffer, size_t s) {
    size_t bytes_sent = 0;

    while (bytes_sent < total) {
        size_t chunk = (total - bytes_sent < (size_t)s) ? (total - bytes_sent) : (size_t)s;

        for (size_t i = 0; i < chunk; i++) {
            writeBuffer[i] = lineBuffer[bytes_sent + i];
        }

        size_t written_in_chunk = 0;

        while (written_in_chunk < chunk) {
            ssize_t w = write(STDOUT_FILENO, writeBuffer + written_in_chunk, chunk - written_in_chunk);

            if (w == -1) {
                return -1;
            }
            written_in_chunk += w;
        }

            bytes_sent += chunk;
        }

        ssize_t nl_written = 0;

        while (nl_written < 1) {
            ssize_t w = write(STDOUT_FILENO, "\n", 1 - nl_written);

            if (w == -1) {
                return -1;
            }
            nl_written += w;
        }
    return (ssize_t) total;
}

int treatRE(regex_t regex, char *lineBuffer, char *writeBuffer, size_t total, int flagV, int s) { 
    int ret = regexec(&regex, lineBuffer, 0, NULL, 0);

    if ((flagV == 0 && ret == 0) || (flagV == 1 && ret == REG_NOMATCH)) {
        if (write_all(STDOUT_FILENO, lineBuffer, total, writeBuffer, (size_t)s) == -1) {
            return -1;
        }
    }

    return 0;
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

    char *readBuffer = malloc(s);
    char *writeBuffer = malloc(s);
    char *lineBuffer = malloc(MAX_LINE_SIZE);
    char *auxBuffer = NULL;
    
    if (readBuffer == NULL || writeBuffer == NULL || lineBuffer == NULL) {
        fprintf(stderr, "ERROR: malloc()\n");
        exit(EXIT_FAILURE);
    }

    regex_t regex;

    if (regcomp(&regex, r, REG_EXTENDED | REG_NEWLINE) != 0) {
        fprintf(stderr, "ERROR: REGEX mal construida\n");
        exit(EXIT_FAILURE);
    }

    size_t read_bytes = 0;
    size_t treated_bytes = 0;
    size_t line_bytes = 0;
    bool exitBucle = false;

    while (exitBucle == false) {
        if (isatty(STDIN_FILENO)) {
            break;
        }

        ssize_t received_bytes = read_all(readBuffer, s);

        if (received_bytes < 0) {
            fprintf(stderr, "ERROR: read()\n");
            free(readBuffer);
            free(writeBuffer);
            free(lineBuffer);
            regfree(&regex);
            exit(EXIT_FAILURE);
        }

        if (received_bytes == 0) {
            if (line_bytes == 0) {
                break;
            }
            readBuffer[0] = '\n';
            received_bytes = 1;
            exitBucle = true;
        }

        size_t pending_bytes = received_bytes;
        auxBuffer = readBuffer;
        read_bytes = 0;

        while (read_bytes < received_bytes) {
            char byte = *auxBuffer;
            pending_bytes++;
            read_bytes++;
            auxBuffer++;

            if (byte != '\n') {
                if (line_bytes >= MAX_LINE_SIZE) {
                    fprintf(stderr, "ERROR: Línea demasiado larga\n");
                    free(readBuffer);
                    free(writeBuffer);
                    free(lineBuffer);
                    regfree(&regex);
                    exit(EXIT_FAILURE);
                }

                lineBuffer[line_bytes] = byte;
                line_bytes++;
                continue;
            }

            if (line_bytes >= MAX_LINE_SIZE) {
                fprintf(stderr, "ERROR: Línea demasiado larga\n");
                free(readBuffer);
                free(writeBuffer);
                free(lineBuffer);
                regfree(&regex);
                exit(EXIT_FAILURE);
            }

            lineBuffer[line_bytes] = '\0';

            if (treatRE(regex, lineBuffer, writeBuffer, line_bytes, flagV, s) == -1) {
                fprintf(stderr, "ERROR: write()\n");
                free(readBuffer);
                free(writeBuffer);
                free(lineBuffer);
                regfree(&regex);
                exit(EXIT_FAILURE);
            }

            line_bytes = 0;
        }
    }

    free(readBuffer);
    free(writeBuffer);
    free(lineBuffer);
    regfree(&regex);

    return EXIT_SUCCESS;
}
