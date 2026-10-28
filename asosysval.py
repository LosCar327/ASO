#! /usr/bin/env python3
# -*- coding: utf-8; -*-

version="v0.23.1"

"""
    Validation `asosysval` v0.23.1.

    Ampliación de Sistemas Operativos (Curso 2026/2027)
    Departamento de Ingeniería y Tecnología de Computadores
    Facultad de Informática de la Universidad de Murcia
"""


################################################################################

import os
import subprocess
import sys
import tempfile
import zipfile

srcfns = ["minigrep.c", "watchdir.c"]
autors = "autores.txt"
allfns = srcfns + [autors]

POSIX_HEADER_LINE = "#define _POSIX_C_SOURCE 200809L\n"


################################################################################


def info(*args):
    print("{}:".format(os.path.basename(sys.argv[0])), *args)


def panic(bin, zfn=None):
    if zfn: info(f"La entrega '{zfn}' NO es válida")
    info(f"usage: {bin} file.zip")
    sys.exit(1)


################################################################################


def main(args):

    """ Main driver. """

    info(f"Version: {version}")

    if len(args) != 2:
        panic(args[0])

    zfn = args[1]

    if not zfn.endswith(".zip"):
        info(f"El archivo '{zfn}' debe tener extensión .zip")
        panic(args[0], zfn)

    if not os.access(zfn, os.R_OK):
        info(f"El archivo '{zfn}' no existe o no tiene permiso de lectura")
        panic(args[0], zfn)

    try:
        zf = zipfile.ZipFile(zfn)
    except zipfile.BadZipFile:
        info(f"El archivo '{zfn}' no es un archivo ZIP válido")
        panic(args[0], zfn)

    if set(zf.namelist()) != set(allfns):
        info(f"El archivo '{zfn}' debe contener los archivos {allfns}")
        panic(args[0], zfn)

    with tempfile.TemporaryDirectory() as td:
        try:
            zf.extractall(td)
        except:
            info(f"No se puede extraer el contenido de '{zfn}'")
            panic(args[0], zfn)

        for fn in zf.namelist():
            if fn == autors:
                continue
            with open(os.path.join(td, fn), 'r') as f:
                header_line = f.readline()
            if header_line != POSIX_HEADER_LINE:
                info(f"La primera línea del archivo '{fn}' debe ser '{POSIX_HEADER_LINE.strip()}' pero es '{header_line.strip()}'")
                panic(args[0], zfn)

        try:
            for fn in zf.namelist():
                if fn == autors:
                    continue
                subprocess.run(
                    ["gcc", "-o", f"{td}/bin", "-ggdb3", "-Wall", "-Werror", "-Wno-unused", "-std=c11", "-fPIC", "-pie", os.path.join(td, fn)],
                    stdout=subprocess.PIPE,
                    stderr=subprocess.PIPE,
                    check=True
                )
        except:
            info(f"El archivo '{fn}' no compila")
            panic(args[0], zfn)

    info(f"La entrega '{zfn}' es válida")

    return 0


################################################################################


if __name__ == "__main__":
    sys.exit(main(sys.argv))
