#!/usr/bin/env python3
"""Extract the 100-program Bass Station plug-in factory bank from a .pchk state.

The public BassStationStereo state used here is 5517 bytes:
  0..3      wrapper version
  4..12     plug-in bank header (starts with b'tffp')
  13..3412  100 compact 34-byte program records
  3413..5512 100 fixed 21-byte program names
  5513      selected program
  5514..5516 trailer

The 34-byte program record is the compact (non-nibble-expanded) form of the
original Bass Station/Rack patch layout already documented by the historical
Novation-compatible editor and our Rack SysEx codec.
"""
from pathlib import Path
import argparse

COUNT=100; REC=34; NAME=21; REC_OFF=13; NAME_OFF=REC_OFF+COUNT*REC

def parse(path: Path):
    b=path.read_bytes()
    if len(b)!=5517 or b[4:8]!=b'tffp':
        raise SystemExit(f'unsupported pchk: length={len(b)}, magic={b[4:8]!r}')
    records=[b[REC_OFF+i*REC:REC_OFF+(i+1)*REC] for i in range(COUNT)]
    names=[]
    for i in range(COUNT):
        raw=b[NAME_OFF+i*NAME:NAME_OFF+(i+1)*NAME]
        names.append(raw.split(b'\0',1)[0].decode('ascii','replace'))
    return records,names,b[5513]

def emit_header(records,names,out: Path):
    lines=['#pragma once','#include <stdint.h>','',
           '/* Recovered Bass Station plug-in factory bank: 100 x 34-byte compact records. */',
           '#define BST_FACTORY_COUNT 100u','#define BST_FACTORY_RECORD_BYTES 34u','',
           'static const uint8_t bst_factory_record[BST_FACTORY_COUNT][BST_FACTORY_RECORD_BYTES] = {']
    for r in records:
        lines.append('    {'+','.join('0x%02x'%x for x in r)+'},')
    lines += ['};','', 'static const char bst_factory_name[BST_FACTORY_COUNT][22] = {']
    for n in names:
        esc=n.replace('\\','\\\\').replace('"','\\"')
        lines.append(f'    "{esc}",')
    lines += ['};','']
    out.write_text('\n'.join(lines))

def main():
    ap=argparse.ArgumentParser(); ap.add_argument('pchk',type=Path); ap.add_argument('--header',type=Path)
    ns=ap.parse_args(); records,names,sel=parse(ns.pchk)
    print(f'{len(records)} programs; selected={sel}; first={names[0]!r}; last={names[-1]!r}')
    if ns.header: emit_header(records,names,ns.header)
if __name__=='__main__':main()
