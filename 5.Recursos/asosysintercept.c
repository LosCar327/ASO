#define _GNU_SOURCE

#include <dlfcn.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <malloc.h>
#include <regex.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
// #include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#define MAIN_LOGGER "asosysint.main.log"
#define RDWR_LOGGER "asosysint.rdwr.log"
#define HEAP_LOGGER "asosysint.heap.log"

#define MIN(a, b) (((a) < (b)) ? (a) : (b))

#define WBUF_SIZE 512

/*
 * References:
 *  http://klamp.works/2015/12/18/hooking-libc-functions.html
 *  http://klamp.works/2015/12/25/hooking-libc-functions-2.html
 *  https://stackoverflow.com/questions/262439/create-a-wrapper-function-for-malloc-and-free-in-c
 *  https://www.jimnewsome.net/posts/interposing-internal-libc-calls
 *  https://www.gnu.org/savannah-checkouts/gnu/libc/manual/html_node/Hooks-for-Malloc.html
 * */

// Log files

static int main_fd = -1;
static int rdwr_fd = -1;
static int heap_fd = -1;
static char wbuf[WBUF_SIZE];

// Environment variables:

static unsigned _RDWR_LOGGER_ENABLED = 0;
static unsigned _HEAP_LOGGER_ENABLED = 0;

static ssize_t _WRITE__MAX_BLOCK_SIZE = (ssize_t)-1;
static ssize_t _READ__MAX_BLOCK_SIZE = (ssize_t)-1;

static unsigned _MALLOC_ENABLED = 1;
static unsigned _READ_ENABLED = 1;
static unsigned _WRITE_ENABLED = 1;

static unsigned _OPENDIR_ENABLED = 0;
static unsigned _READDIR_ENABLED = 0;
static unsigned _CLOSEDIR_ENABLED = 0;

static unsigned _SCANDIR_ENABLED = 1;

static unsigned _STAT_ENABLED = 1;

static unsigned _SIGACTION_ENABLED = 1;
static unsigned _SETITIMER_ENABLED = 1;

void __attribute__((constructor)) asosysintercept_init(void)
{
    main_fd = open(MAIN_LOGGER, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);

    char *env_var_str;

    // TODO: Build a macro or use a hash table

    env_var_str = getenv("ASOSYSINT_RDWR_LOGGER_ENABLED");
    if (env_var_str)
    {
        _RDWR_LOGGER_ENABLED = atoi(env_var_str);
        if (_RDWR_LOGGER_ENABLED)
            rdwr_fd = open(RDWR_LOGGER, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    }
    if (main_fd != -1)
    {
        _RDWR_LOGGER_ENABLED = 0;
        sprintf(wbuf, "ASOSYSINT_RDWR_LOGGER_ENABLED=%d\n", _RDWR_LOGGER_ENABLED);
        write(main_fd, wbuf, strlen(wbuf));
        _RDWR_LOGGER_ENABLED = 1;
    }

    env_var_str = getenv("ASOSYSINT_HEAP_LOGGER_ENABLED");
    if (env_var_str)
    {
        _HEAP_LOGGER_ENABLED = atoi(env_var_str);
        if (_HEAP_LOGGER_ENABLED)
            heap_fd = open(HEAP_LOGGER, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    }
    if (main_fd != -1)
    {
        _RDWR_LOGGER_ENABLED = 0;
        sprintf(wbuf, "ASOSYSINT_HEAP_LOGGER_ENABLED=%d\n", _HEAP_LOGGER_ENABLED);
        write(main_fd, wbuf, strlen(wbuf));
        _RDWR_LOGGER_ENABLED = 1;
    }

    env_var_str = getenv("ASOSYSINT_WRITE__MAX_BLOCK_SIZE");
    if (env_var_str)
    {
        _WRITE__MAX_BLOCK_SIZE = atoll(env_var_str);
    }
    if (main_fd != -1)
    {
        _RDWR_LOGGER_ENABLED = 0;
        sprintf(wbuf, "ASOSYSINT_WRITE__MAXBLOCK_SIZE=%ld\n", _WRITE__MAX_BLOCK_SIZE);
        write(main_fd, wbuf, strlen(wbuf));
        _RDWR_LOGGER_ENABLED = 1;
    }

    env_var_str = getenv("ASOSYSINT_READ__MAX_BLOCK_SIZE");
    if (env_var_str)
    {
        _READ__MAX_BLOCK_SIZE = atoll(env_var_str);
    }
    if (main_fd != -1)
    {
        _RDWR_LOGGER_ENABLED = 0;
        sprintf(wbuf, "ASOSYSINT_READ__MAX_BLOCK_SIZE=%ld\n", _READ__MAX_BLOCK_SIZE);
        write(main_fd, wbuf, strlen(wbuf));
        _RDWR_LOGGER_ENABLED = 1;
    }

    env_var_str = getenv("ASOSYSINT_MALLOC_ENABLED");
    if (env_var_str)
    {
        _MALLOC_ENABLED = atoi(env_var_str);
    }
    if (main_fd != -1)
    {
        _RDWR_LOGGER_ENABLED = 0;
        sprintf(wbuf, "ASOSYSINT_MALLOC_ENABLED=%d\n", _MALLOC_ENABLED);
        write(main_fd, wbuf, strlen(wbuf));
        _RDWR_LOGGER_ENABLED = 1;
    }

    env_var_str = getenv("ASOSYSINT_READ_ENABLED");
    if (env_var_str)
    {
        _READ_ENABLED = atoi(env_var_str);
    }
    if (main_fd != -1)
    {
        _RDWR_LOGGER_ENABLED = 0;
        sprintf(wbuf, "ASOSYSINT_READ_ENABLED=%d\n", _READ_ENABLED);
        write(main_fd, wbuf, strlen(wbuf));
        _RDWR_LOGGER_ENABLED = 1;
    }

    env_var_str = getenv("ASOSYSINT_WRITE_ENABLED");
    if (env_var_str)
    {
        _WRITE_ENABLED = atoi(env_var_str);
    }
    if (main_fd != -1)
    {
        _RDWR_LOGGER_ENABLED = 0;
        sprintf(wbuf, "ASOSYSINT_WRITE_ENABLED=%d\n", _WRITE_ENABLED);
        write(main_fd, wbuf, strlen(wbuf));
        _RDWR_LOGGER_ENABLED = 1;
    }

    env_var_str = getenv("ASOSYSINT_SCANDIR_ENABLED");
    if (env_var_str)
    {
        _SCANDIR_ENABLED = atoi(env_var_str);
    }
    if (main_fd != -1)
    {
        _RDWR_LOGGER_ENABLED = 0;
        sprintf(wbuf, "ASOSYSINT_SCANDIR_ENABLED=%d\n", _SCANDIR_ENABLED);
        write(main_fd, wbuf, strlen(wbuf));
        _RDWR_LOGGER_ENABLED = 1;
    }

    env_var_str = getenv("ASOSYSINT_STAT_ENABLED");
    if (env_var_str)
    {
        _STAT_ENABLED = atoi(env_var_str);
    }
    if (main_fd != -1)
    {
        _RDWR_LOGGER_ENABLED = 0;
        sprintf(wbuf, "ASOSYSINT_STAT_ENABLED=%d\n", _STAT_ENABLED);
        write(main_fd, wbuf, strlen(wbuf));
        _RDWR_LOGGER_ENABLED = 1;
    }

    env_var_str = getenv("ASOSYSINT_SIGACTION_ENABLED");
    if (env_var_str)
    {
        _SIGACTION_ENABLED = atoi(env_var_str);
    }
    if (main_fd != -1)
    {
        _RDWR_LOGGER_ENABLED = 0;
        sprintf(wbuf, "ASOSYSINT_SIGACTION_ENABLED=%d\n", _SIGACTION_ENABLED);
        write(main_fd, wbuf, strlen(wbuf));
        _RDWR_LOGGER_ENABLED = 1;
    }

    env_var_str = getenv("ASOSYSINT_SETITIMER_ENABLED");
    if (env_var_str)
    {
        _SETITIMER_ENABLED = atoi(env_var_str);
    }
    if (main_fd != -1)
    {
        _RDWR_LOGGER_ENABLED = 0;
        sprintf(wbuf, "ASOSYSINT_SETITIMER_ENABLED=%d\n", _SETITIMER_ENABLED);
        write(main_fd, wbuf, strlen(wbuf));
        _RDWR_LOGGER_ENABLED = 1;
    }
}

void __attribute__((destructor)) asosysintercept_fini(void)
{
    if (main_fd != -1)
        close(main_fd);
    if (rdwr_fd != -1)
        close(rdwr_fd);
    if (heap_fd != -1)
        close(heap_fd);
}

/*************************************************/
/* NOOP - Functions that must not be used at all */
/*************************************************/

int fgetc(FILE *stream) { return EOF; }
char *fgets(char *s, int size, FILE *stream)  { return NULL; }
int getc(FILE *stream) { return EOF; }
int getchar(void) { return EOF; }
int ungetc(int c, FILE *stream)  { return EOF; }

int fputc(int c, FILE *stream) { return EOF; }
// int fputs(const char *s, FILE *stream) { return EOF; }  // Invoked by fprintf()

int bcmp(const void *s1, const void *s2, size_t n) { return -1; }
void bcopy(const void *src, void *dest, size_t n) { return; }
void bzero(void *s, size_t n) { return; }
void explicit_bzero(void *s, size_t n) { return; }
void swab(const void *from, void *to, ssize_t n) { return; }

void *memcpy(void *dest, const void *src, size_t n) { return NULL; }
void *memccpy(void *dest, const void *src, int c, size_t n) { return NULL; }

void *(memchr)(const void *s, int c, size_t n) { return NULL; }
void *memrchr(const void *s, int c, size_t n) { return NULL; }
void *rawmemchr(const void *s, int c) { return NULL; }

int memcmp(const void *s1, const void *s2, size_t n) { return 0; }
void *memfrob(void *s, size_t n) { return NULL; }
void *memmem(const void *haystack, size_t haystacklen, const void *needle, size_t needlelen) { return NULL; }

void *memmove(void *dest, const void *src, size_t n) { return NULL; }
wchar_t *wmemmove(wchar_t *dest, const wchar_t *src, size_t n) { return NULL; }
// void *memset(void *s, int c, size_t n) { return NULL; }

int putc(int c, FILE *stream) { return EOF; }
int putchar(int c) { return EOF; }
// int puts(const char *s) { return EOF; }  // Invoked by printf()

unsigned int sleep(unsigned int seconds) { return 0; }

char *strcat(char *dest, const char *src) { return NULL; }
char *strncat(char *dest, const char *src, size_t n) { return NULL; }

char *strcpy(char *dest, const char *src) { return NULL; }
// char *strncpy(char *dest, const char *src, size_t n)  { return NULL; }

int strcmp(const char *s1, const char *s2) { return 0; }
// int strncmp(const char *s1, const char *s2, size_t n)  { return 0; }

// char *strchr(const char *s, int c) { return NULL; }
char *(strrchr)(const char *s, int c) { return NULL; }
char *strchrnul(const char *s, int c) { return NULL; }

char *(strstr)(const char *haystack, const char *needle) { return NULL; }
char *strcasestr(const char *haystack, const char *needle) { return NULL; }

sighandler_t signal(int signum, sighandler_t handler) { return SIG_ERR; }

/************************************************************/
/* INTERCEPT - Functions that can intercepted and/or logged */
/************************************************************/

void *malloc(size_t size)
{
    if (_MALLOC_ENABLED)
    {
        void *(*old_malloc)(size_t);

        old_malloc = dlsym(RTLD_NEXT, "malloc");
        void *rc = old_malloc(size);

        // Make sure all strings are null-terminated
        memset(rc, 1, size);

        if (_HEAP_LOGGER_ENABLED && heap_fd != -1)
        {
            _RDWR_LOGGER_ENABLED = 0;
            sprintf(wbuf, "M,%p,%ld\n", rc, size);
            write(heap_fd, wbuf, strlen(wbuf));
            _RDWR_LOGGER_ENABLED = 1;
        }

        return rc;
    }

    errno = ENOMEM;
    return NULL;
}

void *calloc(size_t nmemb, size_t size)
{
    if (_MALLOC_ENABLED)
    {
        void *(*old_calloc)(size_t, size_t);
        old_calloc = dlsym(RTLD_NEXT, "calloc");
        void *rc = old_calloc(nmemb, size);

        if (_HEAP_LOGGER_ENABLED && heap_fd != -1)
        {
            _RDWR_LOGGER_ENABLED = 0;
            sprintf(wbuf, "C,%p,%ld\n", rc, size);
            write(heap_fd, wbuf, strlen(wbuf));
            _RDWR_LOGGER_ENABLED = 1;
        }

        return rc;
    }

    errno = ENOMEM;
    return NULL;
}

void *realloc(void *ptr, size_t size)
{
    if (_MALLOC_ENABLED)
    {
        void *(*old_realloc)(void *, size_t);

        old_realloc = dlsym(RTLD_NEXT, "realloc");
        void *rc = old_realloc(ptr, size);

        if (_HEAP_LOGGER_ENABLED && heap_fd != -1)
        {
            _RDWR_LOGGER_ENABLED = 0;
            sprintf(wbuf, "R,%p,%p,%ld\n", ptr, rc, size);
            write(heap_fd, wbuf, strlen(wbuf));
            _RDWR_LOGGER_ENABLED = 1;
        }

        return rc;
    }

    errno = ENOMEM;
    return NULL;
}

void *reallocarray(void *ptr, size_t nmemb, size_t size)
{
    if (_MALLOC_ENABLED)
    {
        void *(*old_reallocarray)(void *, size_t, size_t);

        old_reallocarray = dlsym(RTLD_NEXT, "reallocarray");
        void *rc = old_reallocarray(ptr, nmemb, size);

        if (_HEAP_LOGGER_ENABLED && heap_fd != -1)
        {
            _RDWR_LOGGER_ENABLED = 0;
            sprintf(wbuf, "R,%p,%p,%ld\n", ptr, rc, size);
            write(heap_fd, wbuf, strlen(wbuf));
            _RDWR_LOGGER_ENABLED = 1;
        }

        return rc;
    }

    errno = ENOMEM;
    return NULL;
}

void free(void *ptr)
{
    void *(*old_free)(void *);

    old_free = dlsym(RTLD_NEXT, "free");

    if (_HEAP_LOGGER_ENABLED && heap_fd != -1)
    {
        _RDWR_LOGGER_ENABLED = 0;
        sprintf(wbuf, "F,%p\n", ptr);
        write(heap_fd, wbuf, strlen(wbuf));
        _RDWR_LOGGER_ENABLED = 1;
    }

    old_free(ptr);
}

ssize_t read(int fd, void *buf, size_t count)
{
    if (_READ_ENABLED)
    {
        ssize_t (*old_read)(int, void *, size_t);

        old_read = dlsym(RTLD_NEXT, "read");

        ssize_t nread = old_read(fd, buf, MIN(count, _READ__MAX_BLOCK_SIZE));

        if (_RDWR_LOGGER_ENABLED && rdwr_fd != -1)
        {
            _RDWR_LOGGER_ENABLED = 0;
            sprintf(wbuf, "R,%p,%ld\n", buf, count);
            write(rdwr_fd, wbuf, strlen(wbuf));
            _RDWR_LOGGER_ENABLED = 1;
        }

        return nread;
    }

    errno = EBADF;
    return -1;
}

ssize_t write(int fd, const void *buf, size_t count)
{
    if (_WRITE_ENABLED)
    {
        ssize_t (*old_write)(int, const void *, size_t);

        old_write = dlsym(RTLD_NEXT, "write");

        ssize_t nwrite = old_write(fd, buf, MIN(count, _WRITE__MAX_BLOCK_SIZE));

        if (_RDWR_LOGGER_ENABLED && rdwr_fd != -1)
        {
            _RDWR_LOGGER_ENABLED = 0;
            sprintf(wbuf, "W,%p,%ld\n", buf, count);
            write(rdwr_fd, wbuf, strlen(wbuf));
            _RDWR_LOGGER_ENABLED = 1;
        }
        return nwrite;
    }

    errno = ENOSPC;
    return -1;
}

int regcomp(regex_t *preg, const char *regex, int cflags)
{
    int (*old_regcomp)(regex_t *, const char *, int);

    _HEAP_LOGGER_ENABLED = 0;
    unsigned _malloc_enabled = _MALLOC_ENABLED;
    _MALLOC_ENABLED = 1;
    old_regcomp = dlsym(RTLD_NEXT, "regcomp");
    int rc = old_regcomp(preg, regex, cflags);
    _MALLOC_ENABLED = _malloc_enabled;
    _HEAP_LOGGER_ENABLED = 1;

    return rc;
}

int regexec(const regex_t *preg, const char *string, size_t nmatch,
            regmatch_t pmatch[restrict nmatch], int eflags)
{
    int (*old_regexec)(const regex_t *, const char *, size_t, regmatch_t[], int);

    _HEAP_LOGGER_ENABLED = 0;
    old_regexec = dlsym(RTLD_NEXT, "regexec");
    int rc = old_regexec(preg, string, nmatch, pmatch, eflags);
    _HEAP_LOGGER_ENABLED = 1;

    return rc;
}

size_t regerror(int errcode, const regex_t *preg, char *errbuf,
                size_t errbuf_size)
{
    size_t (*old_regerror)(int, const regex_t *, char *, size_t);

    _HEAP_LOGGER_ENABLED = 0;
    old_regerror = dlsym(RTLD_NEXT, "regerror");
    size_t rc = old_regerror(errcode, preg, errbuf, errbuf_size);
    _HEAP_LOGGER_ENABLED = 1;

    return rc;
}

void regfree(regex_t *preg)
{
    size_t (*old_regfree)(regex_t *);

    _HEAP_LOGGER_ENABLED = 0;
    old_regfree = dlsym(RTLD_NEXT, "regfree");
    old_regfree(preg);
    _HEAP_LOGGER_ENABLED = 1;
}

DIR *opendir(const char *name)
{
    DIR *(*old_opendir)(const char *);

    if (_OPENDIR_ENABLED)
    {
        old_opendir = dlsym(RTLD_NEXT, "opendir");
        return old_opendir(name);
    }

    errno = EBADF;
    return NULL;
}

struct dirent *readdir(DIR *dirp)
{
    struct dirent *(*old_readdir)(DIR *);

    if (_READDIR_ENABLED)
    {
        old_readdir = dlsym(RTLD_NEXT, "readdir");
        return old_readdir(dirp);
    }

    errno = EBADF;
    return NULL;
}

int closedir(DIR *dirp)
{
    int (*old_closedir)(DIR *);

    if (_CLOSEDIR_ENABLED)
    {
        old_closedir = dlsym(RTLD_NEXT, "closedir");
        return old_closedir(dirp);
    }

    errno = EBADF;
    return -1;
}

int scandir(const char *dirp, struct dirent ***namelist,
            int (*filter)(const struct dirent *),
            int (*compar)(const struct dirent **, const struct dirent **))
{
    int (*old_scandir)(const char *, struct dirent ***,
                       int (*)(const struct dirent *),
                       int (*)(const struct dirent **, const struct dirent **));

    if (_SCANDIR_ENABLED)
    {
        old_scandir = dlsym(RTLD_NEXT, "scandir");
        return old_scandir(dirp, namelist, filter, compar);
    }

    errno = ENOMEM;
    return -1;
}

int stat(const char *pathname, struct stat *statbuf)
{
    int (*old_stat)(const char *, struct stat *);

    if (_STAT_ENABLED)
    {
        old_stat = dlsym(RTLD_NEXT, "stat");
        return old_stat(pathname, statbuf);
    }

    errno = EBADF;
    return -1;
}

int sigaction(int signum, const struct sigaction *act, struct sigaction *oldact)
{
    int (*old_sigaction)(int, const struct sigaction *, struct sigaction *);

    if (_SIGACTION_ENABLED)
    {
        old_sigaction = dlsym(RTLD_NEXT, "sigaction");
        return old_sigaction(signum, act, oldact);
    }

    errno = EFAULT;
    return -1;
}

/* setitimer is defined in #include <sys/time.h> */
struct itimerval
{
    struct timeval it_interval; /* Interval for periodic timer */
    struct timeval it_value;    /* Time until next expiration */
};

// struct timeval {
//     time_t      tv_sec;         /* seconds */
//     suseconds_t tv_usec;        /* microseconds */
// };

int setitimer(int which, const struct itimerval *new_value, struct itimerval *old_value)
{
    int (*old_setitimer)(int, const struct itimerval *, struct itimerval *);

    if (_SETITIMER_ENABLED)
    {
        old_setitimer = dlsym(RTLD_NEXT, "setitimer");
        return old_setitimer(which, new_value, old_value);
    }

    errno = EFAULT;
    return -1;
}
