#define _POSIX_C_SOURCE 200809L
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>
#include <limits.h>
#include <signal.h>
#include <sys/time.h>

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

// Función para filtrar entradas de directorio no deseadas, como "." y ".."

static int filter(const struct dirent *entry) {
    return !(entry->d_name[0] == '.' &&
             (entry->d_name[1] == '\0' ||
              (entry->d_name[1] == '.' &&
               entry->d_name[2] == '\0')));
}

static void free_dir_entries(struct dirent **entries, int count) {
    int i;

    for (i = 0; i < count; i++) {
        free(entries[i]);
    }

    free(entries);
}

static int log_initial_entries(FILE *log_file) {
    struct dirent **entries = NULL;
    struct stat entry_info;
    char path[PATH_MAX];
    int count;
    int i;
    int path_length;

    count = scandir(dir_name, &entries, filter, alphasort);

    if (count == -1) {
        fprintf(stderr, "ERROR: scandir()\n");
        return -1;
    }

    for (i = 0; i < count; i++) {
        path_length = snprintf(path, sizeof(path), "%s/%s", dir_name, entries[i]->d_name);      
        // snprintf() devuelve el número de caracteres que habrían sido escritos si el buffer fuera lo suficientemente grande, 
        // excluyendo el carácter nulo final. Si el valor devuelto es negativo, significa que hubo un error. Si es mayor o igual al tamaño del buffer, 
        // significa que el buffer no era lo suficientemente grande y se truncó la salida. Por eso se comprueba si path_length < 0 o path_length >= sizeof(path).

        if (path_length < 0 || (size_t)path_length >= sizeof(path)) {
            fprintf(stderr, "ERROR: stat()\n");
            free_dir_entries(entries, count);
            return -1;
        }

        if (stat(path, &entry_info) == -1) {
            fprintf(stderr, "ERROR: stat()\n");
            free_dir_entries(entries, count);
            return -1;
        }

        if (fprintf(log_file, "Creation: %s\n", entries[i]->d_name) < 0) {
            fprintf(stderr, "ERROR: fprintf()\n");
            free_dir_entries(entries, count);
            return -1;
        }
    }

    free_dir_entries(entries, count);

    if (fflush(log_file) == EOF) {      
        // fflush() devuelve EOF si hay un error al vaciar el buffer del flujo. 
        // Esto puede ocurrir si hay un error de escritura en el archivo, 
        // por ejemplo, si se llena el disco o si se pierde la conexión a un sistema de archivos remoto.
        fprintf(stderr, "ERROR: fflush()\n");
        return -1;
    }

    return 0;
}

// Función para manejar la señal SIGALRM, que se invoca cuando expira el temporizador.

static void handle_alarm(int signal) {
    (void)signal;
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

    FILE *log_file = fopen(log_name, "w");
    // Con la opción "w", fopen() crea el archivo si no existe, o lo trunca a cero si ya existe.

    if (log_file == NULL) {
        fprintf(stderr, "ERROR: fopen()\n");
        return EXIT_FAILURE;
    }

    if (log_initial_entries(log_file) == -1) {
        fclose(log_file);
        return EXIT_FAILURE;
    }

    // Incorporación para manejar la señal SIGALRM y establecer un temporizador que se dispare cada 'seconds' segundos.

    struct sigaction sa;

    sa.sa_handler = handle_alarm;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    if (sigaction(SIGALRM, &sa, NULL) == -1) {
        fprintf(stderr, "ERROR: sigaction()\n");
        fclose(log_file);
        return EXIT_FAILURE;
    }

    // Incorporación de setitimer() para establecer un temporizador que se dispare cada 'seconds' segundos.

    struct itimerval timer;

    timer.it_value.tv_sec = seconds;
    timer.it_value.tv_usec = 0;
    timer.it_interval.tv_sec = seconds;
    timer.it_interval.tv_usec = 0;

    if (setitimer(ITIMER_REAL, &timer, NULL) == -1) {
        fprintf(stderr, "ERROR: setitimer()\n");
        fclose(log_file);
        return EXIT_FAILURE;
    }

    while (1) {
        // Aquí iría la lógica para vigilar el directorio y registrar cambios en el archivo de log.
        pause();
    }


}
