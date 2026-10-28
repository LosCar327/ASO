#! /usr/bin/env python3

import argparse
import binascii
import os
import random
import sys

parser = argparse.ArgumentParser()
parser.add_argument("-s", "--semilla", help="semilla para el generador de números aleatorios", type=int, default=42)
parser.add_argument("-b", "--numbytes", help="número de bytes aleatorios generados", type=int, required=True)
parser.add_argument("-l", "--tamlineas", help="número de bytes por línea", type=int, default=int(4096))
parser.add_argument("-n", "--nonewline", help="no incluir '\n' en última línea", action='store_true')
args = parser.parse_args()

random.seed(args.semilla)

for i in range(args.numbytes//args.tamlineas):
    random_line_lst = list(binascii.b2a_hex(random.randbytes( (args.tamlineas//2) + 1 ) ).decode('utf8'))
    random_line_lst[args.tamlineas:] = []
    if not args.nonewline or args.numbytes % args.tamlineas:
        random_line_lst[-1] = '\n'
    random_line_bytes = ''.join(random_line_lst).encode()
    os.write(sys.stdout.fileno(), random_line_bytes)

random_line_lst = list(binascii.b2a_hex(random.randbytes( ( (args.numbytes % args.tamlineas)//2 ) + 1 ) ).decode('utf8'))
random_line_lst[args.numbytes % args.tamlineas:] = []
if random_line_lst:
    if not args.nonewline: random_line_lst[-1] = '\n'
    random_line_bytes = ''.join(random_line_lst).encode()
    os.write(sys.stdout.fileno(), random_line_bytes)
