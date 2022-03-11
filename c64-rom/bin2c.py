#!/usr/bin/env python
import argparse
import os
import re
import textwrap

if __name__ != '__main__':
    raise RuntimeError('not a module')

parser = argparse.ArgumentParser(
    description='Given file.bin, write file.c and file.h')
parser.add_argument('--skip', metavar='N', default=0, type=int,
                    help='start N bytes into the source file')
parser.add_argument('input', metavar='file.bin')
args = parser.parse_args()

size = os.path.getsize(args.input)
basename = os.path.basename(args.input).partition('.')[0]
constname = re.sub(r'[^0-9A-Za-z_]', '', basename.replace('-', '_'))
with open(args.input, 'rb') as inf:
    inf.seek(args.skip)
    with open(f'{basename}.c', 'wt') as outf:
        outf.write(textwrap.dedent(f"""\
            #include <stdint.h>

            const uint8_t {constname}[{size}] = {{
            """))
        while True:
            data = inf.read(8)
            if not data:
                break
            outf.write('\t')
            outf.write(', '.join(f'0x{n:02X}' for n in data))
            outf.write(',\n')
        outf.write('};\n')

with open(f'{basename}.h', 'wt') as outf:
    outf.write(textwrap.dedent(f"""\
        #pragma once

        extern const uint8_t {constname}[{size}];
        """))
