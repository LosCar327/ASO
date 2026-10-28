#!/usr/bin/make -f

MAJOR := 0
MINOR := 1
ASOSYSINT := asosysintercept
VERSION := $(MAJOR).$(MINOR)

# Registros que escribe la biblioteca de intercepción en el directorio de
# trabajo
LOGS := asosysint.main.log asosysint.rdwr.log asosysint.heap.log

CFLAGS=-ggdb3 -Wall -Werror -Wno-unused -std=c11 -fPIC -pie
LDFLAGS=--shared -fPIC
LDLIBS=-Wl,--no-as-needed -ldl

all: lib tarea minigrep watchdir

lib: lib$(ASOSYSINT).so.$(VERSION)

lib$(ASOSYSINT).so.$(VERSION): $(ASOSYSINT).o
	$(CC) $(LDFLAGS) $(LDLIBS) $^ -o $@

tarea: tarea.c
	$(CC) $(CFLAGS) tarea.c -o $@

minigrep: minigrep.c
	$(CC) $(CFLAGS) minigrep.c -o $@

watchdir: watchdir.c
	$(CC) $(CFLAGS) watchdir.c -o $@

clean:
	rm -rf *~ lib$(ASOSYSINT).so.$(VERSION) $(ASOSYSINT).o tarea minigrep watchdir core $(LOGS)

.PHONY: clean
