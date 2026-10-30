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


ssize_t write_all(int fd, char* buf, ssize_t num_read) {
    ssize_t num_written = 0;
    ssize_t num_left = num_read;
    char* buf_left = buf;
    
    /*if (buf[0] == '\0') {
        fprintf(stderr, "ERROR: write()\n");
        exit(EXIT_FAILURE);
    }*/

    while (num_left > 0) {
        num_written = write(fd, buf_left, num_left);

        if(num_written == -1) {
            return -1;
        }

        num_left -= num_written;
        buf_left += num_written;
    }

    if (write(fd, "\n", 1) == -1) {
        return -1;
    }

    return num_written;
}

int treatRE(regex_t regex, char *lineBuffer, char *writeBuffer, size_t total, int flagV) { 
    size_t k;
    for (k = 0; k < total; k++) {
        writeBuffer[k] = lineBuffer[k];
    }

    writeBuffer[total] = '\0';

    int ret = regexec(&regex, writeBuffer, 0, NULL, 0);

    if ((flagV == 0 && ret == 0) || (flagV == 1 && ret == REG_NOMATCH)) {
        return (int) write_all(STDOUT_FILENO, writeBuffer, (ssize_t)total);
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
    char *auxBuffer = malloc(s);

    if (readBuffer == NULL || writeBuffer == NULL || auxBuffer == NULL) {
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
            free(auxBuffer);
            regfree(&regex);
            exit(EXIT_FAILURE);
        }

        if (received_bytes == 0) {
            /*if (read_bytes == 0) {
                break;
            }
            readBuffer[0] = '\n';
            received_bytes = 1;*/
            exitBucle = true;
        }

        size_t line_bytes = 0;
        size_t pending_bytes = received_bytes;

        while (read_bytes < received_bytes) {

            char byte = *readBuffer;
            pending_bytes++;
            read_bytes++;

            if (byte != '\n') {
                if (line_bytes >= MAX_LINE_SIZE) {
                    fprintf(stderr, "ERROR: Línea demasiado larga\n");
                    free(readBuffer);
                    free(writeBuffer);
                    free(auxBuffer);
                    regfree(&regex);
                    exit(EXIT_FAILURE);
                }

                auxBuffer[line_bytes] = byte;
                line_bytes++;
                continue;
            }

            size_t total_line_bytes = line_bytes;
            line_bytes = 0;
            treated_bytes++;

            if (treatRE(regex, auxBuffer, writeBuffer, line_bytes, flagV) == -1) {
                fprintf(stderr, "ERROR: write()\n");
                free(readBuffer);
                free(writeBuffer);
                free(auxBuffer);
                regfree(&regex);
                exit(EXIT_FAILURE);
            }

            else {
                fprintf(stderr, "ERROR: regexec()\n");
                free(readBuffer);
                free(writeBuffer);
                free(auxBuffer);
                regfree(&regex);
                exit(EXIT_FAILURE);
            }
        }
    }

    free(readBuffer);
    free(writeBuffer);
    free(auxBuffer);
    regfree(&regex);

    return EXIT_SUCCESS;
}
