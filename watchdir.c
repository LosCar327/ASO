#define _POSIX_C_SOURCE 200809L
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>

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
    fprintf(exitProgram, "\tLOG     Log file [default: '/tmp/watchdir.log'].\n");
    fprintf(exitProgram, "\tDIR     Directory name [default: '.'].\n\n");

    exit(num);
}

// Convierte el texto de -n en un entero y comprueba que está en [1..60].
// atoi() devuelve 0 si el texto no es un número, y 0 queda fuera del rango

static int parse_seconds(const char *s) {
    int value = atoi(s);

    if (value < MIN_SECONDS || value > MAX_SECONDS) {
        fprintf(stderr, "ERROR: SECONDS must be a value in [1..60].\n");
        printUsage(EXIT_FAILURE);
    }
    return value;
}

int main(int argc, char *argv[]) {
    int opt;
    int seconds = DEFAULT_SECONDS;
    const char *second_arg = NULL;

    optind = 1;
    while ((opt = getopt(argc, argv, "n:l:h")) != -1) {
        switch (opt) {
            case 'n':
                second_arg = optarg;        // Guarda el argumento de -n (test 3)
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

    // Anotacion para entenderlo: (se puede borrar después)
    /*
    getopt va recorriendo argv y procesa las opciones (-n, -l, -h). 
    Cuando termina, deja la variable global optind apuntando al primer argumento que no es una opción. 
    Esos argumentos que quedan son los posicionales, y aquí solo puede haber uno: el DIR. 
    Un ejemplo con ./watchdir -n 2 -l wd.log wd/
    argv[0] --> ./watchdir
    argv[1] --> -n
    argv[2] --> 2
    argv[3] --> -l
    argv[4] --> wd.log
    argv[5] --> wd/
    optind = 5, que es el índice del primer argumento posicional (DIR).
    Aquí argc vale 6. Al acabar getopt, optind vale 5, que es la posición de wd/. 
    Un argumento posicional es válido si es el último.

    optind == argc - 1: queda exactamente un argumento sin procesar (5 == 6 - 1), 
    así que es el DIR. Lo guardas con dir_name = argv[optind];.

    optind < argc - 1: quedan dos o más argumentos sueltos (por ejemplo a b), y eso es un error de uso. 
    Muestras la ayuda y sales con 1.

    optind == argc: no queda ninguno, así que dir_name conserva el valor por defecto, que es ".".

    ./watchdir --> argc=1, optind=1, no hay argumentos posicionales, dir_name="."
    ./watchdir wd --> argc=2, optind=1, queda un argumento posicional, dir_name="wd"
    ./watchdir a b --> argc=3, optind=1, quedan dos argumentos posicionales, error de uso.
    */
    if (optind < argc - 1) {
        fprintf(stderr, "ERROR: '%s' does not support more than one directory.\n", argv[0]);    // argv[0] es el nombre del programa, que se muestra en el mensaje de error.
        printUsage(EXIT_FAILURE);
    }

    if (optind == argc - 1) {
        dir_name = argv[optind];
    }

    if (second_arg != NULL) {
        seconds = parse_seconds(second_arg);    // Para el test 3, se vuelve a parsear el argumento de -n, aunque ya se hizo antes. 
                                                // Esto es para cumplir con la especificación de que si un argumento se repite, se toma la última aparición.
    }

    struct stat dir_stat;
    if (stat(dir_name, &dir_stat) == -1) {      // Comprueba si el directorio existe y obtiene información sobre él. Si falla, muestra un error y sale.
        fprintf(stderr, "ERROR: stat()\n");
        printUsage(EXIT_FAILURE);
    }

    if (!S_ISDIR(dir_stat.st_mode)) {           // Comprueba si el archivo es un directorio.
        fprintf(stderr, "ERROR: '%s' is not a directory.\n", dir_name);
        printUsage(EXIT_FAILURE);
    }

    int log_fd = open(log_name, O_WRONLY | O_CREAT | O_APPEND, 0644); // Abrir el archivo de log en modo escritura, crear si no existe, y añadir al final. Permisos 0644.

    if (log_fd == -1) {
        perror("open");
        printUsage(EXIT_FAILURE);
    }

    while (1) {
        // Aquí iría la lógica para vigilar el directorio y registrar cambios en el archivo de log.
        pause();
    }


}
