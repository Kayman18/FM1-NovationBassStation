#!/usr/bin/env python3
"""Prepare original Novation Bass Station banks for FM1 TEST11.

Accepted inputs:
  * raw Bass Station Rack 76-byte single-patch SysEx (.syx or embedded in binary)
  * Standard MIDI Files containing those SysEx frames
  * Bass Station 2.x plug-in state banks containing the classic ``tffp`` block
    (including .pchk and common VST2 .fxb/.fxp wrappers)
  * .zip archives containing any of the above

The TEST11 native plug-in-record transport preserves the software instrument's wider
LFO/envelope ranges instead of forcing plug-in patches through Rack export clamps.

Outputs:
  *.syx  concatenated TEST11 import/name frames
  *.mid  the same messages paced for flash writes
  *.txt  slot/name/source manifest
"""
from __future__ import annotations
import argparse, io, zipfile, struct
from pathlib import Path
from typing import Iterable

RACK_PREFIX = bytes((0xF0,0x00,0x20,0x29,0x01,0x03,0x22)); RACK_LEN=76
CTRL_PREFIX = bytes((0xF0,0x7D,ord('B'),ord('S'),0x07))
PLUGIN_PREFIX = bytes((0xF0,0x7D,ord('B'),ord('S'),0x08,0x30))
PCHK_MAGIC=b'tffp'; PCHK_HEAD=9; PCHK_COUNT=100; PCHK_REC=34; PCHK_NAME=21
USER_SLOTS=128
VST_CCNK=b'CcnK'; VST_OPAQUE_PROGRAM=b'FPCh'; VST_OPAQUE_BANK=b'FBCh'; VST_REGULAR_PROGRAM=b'FxCk'; VST_REGULAR_BANK=b'FxBk'
BASSSTATION_IDS={b'Nvb2',b'NvB2'}

def _be32(data:bytes,off:int)->int:
    if off<0 or off+4>len(data): raise ValueError('truncated VST2 integer')
    return int.from_bytes(data[off:off+4],'big',signed=False)

def scan_vst2(data:bytes)->list[tuple[str,bytes,str]]:
    """Return Bass Station VST2 opaque chunks as (kind, chunk, label).

    Supports standard VST2 CcnK/FPCh program chunks and CcnK/FBCh bank
    chunks. Regular FxCk/FxBk parameter-only files are recognized but are not
    converted because they do not contain the plug-in's classic tffp state.
    """
    out=[];pos=0
    while True:
        i=data.find(VST_CCNK,pos)
        if i<0: break
        pos=i+1
        if i+28>len(data): continue
        declared=_be32(data,i+4)
        end=i+8+declared
        if declared<20 or end>len(data): continue
        magic=data[i+8:i+12];fxid=data[i+16:i+20]
        if fxid not in BASSSTATION_IDS: continue
        if magic==VST_OPAQUE_PROGRAM:
            # fxProgramSet: 28-byte common header + 28-byte name + uint32 chunkSize.
            if i+60> end: continue
            name=_printable_name(data[i+28:i+56]) or 'Bass Station Program'
            n=_be32(data,i+56); a=i+60; b=a+n
            if b<=end: out.append(('program',data[a:b],name))
        elif magic==VST_OPAQUE_BANK:
            # fxBankSet: 28-byte common header + 128 reserved bytes + uint32 chunkSize.
            if i+160> end: continue
            n=_be32(data,i+156); a=i+160; b=a+n
            if b<=end: out.append(('bank',data[a:b],'Bass Station Bank'))
        elif magic in (VST_REGULAR_PROGRAM,VST_REGULAR_BANK):
            # Recognized intentionally; no opaque tffp payload to extract here.
            continue
    return out

def valid_rack(f:bytes)->bool:
    return len(f)==RACK_LEN and f.startswith(RACK_PREFIX) and f[-1]==0xF7 and all(b<0x80 for b in f[1:-1])

def scan_raw(data:bytes)->list[bytes]:
    out=[]; pos=0
    while True:
        i=data.find(RACK_PREFIX,pos)
        if i<0: break
        f=data[i:i+RACK_LEN]
        if valid_rack(f): out.append(f); pos=i+RACK_LEN
        else: pos=i+1
    return out

def vlq_read(data:bytes,pos:int)->tuple[int,int]:
    v=0
    for _ in range(4):
        if pos>=len(data): raise ValueError('truncated VLQ')
        b=data[pos];pos+=1;v=(v<<7)|(b&0x7f)
        if not b&0x80:return v,pos
    raise ValueError('invalid VLQ')

def vlq(v:int)->bytes:
    b=[v&0x7f];v>>=7
    while v:b.append(0x80|(v&0x7f));v>>=7
    return bytes(reversed(b))

def scan_smf(data:bytes)->list[bytes]:
    if len(data)<14 or data[:4]!=b'MThd':return []
    pos=8+int.from_bytes(data[4:8],'big');out=[]
    while pos+8<=len(data):
        tag=data[pos:pos+4];n=int.from_bytes(data[pos+4:pos+8],'big');pos+=8;chunk=data[pos:pos+n];pos+=n
        if tag!=b'MTrk':continue
        q=0;running=None
        while q<len(chunk):
            try:_,q=vlq_read(chunk,q)
            except ValueError:break
            if q>=len(chunk):break
            st=chunk[q]
            if st<0x80:
                if running is None:break
                st=running
            else:
                q+=1
                if st<0xf0:running=st
                elif st not in (0xf8,0xf9,0xfa,0xfb,0xfc,0xfd,0xfe):running=None
            if st in (0xf0,0xf7):
                try:ln,q=vlq_read(chunk,q)
                except ValueError:break
                payload=chunk[q:q+ln];q+=ln
                if st==0xf0:
                    f=b'\xf0'+payload
                    if valid_rack(f):out.append(f)
                continue
            if st==0xff:
                if q>=len(chunk):break
                q+=1
                try:ln,q=vlq_read(chunk,q)
                except ValueError:break
                q+=ln;continue
            if 0x80<=st<=0xef:q+=1 if (st&0xe0)==0xc0 else 2;continue
            need={0xf1:1,0xf2:2,0xf3:1,0xf6:0}.get(st)
            if need is None:break
            q+=need
    return out

def unique_frames(data:bytes)->list[bytes]:
    found=scan_smf(data)+scan_raw(data);out=[];seen=set()
    for f in found:
        if f not in seen:seen.add(f);out.append(f)
    return out

def _printable_name(raw:bytes)->str|None:
    z=raw.split(b'\0',1)[0]
    if not z:return None
    if any(c<32 or c>126 for c in z):return None
    return z.decode('ascii')

def scan_tffp(data:bytes)->list[tuple[list[bytes],list[str],int]]:
    """Find complete classic Bass Station 100-program tffp banks inside wrappers."""
    out=[];pos=0;need=PCHK_HEAD+PCHK_COUNT*PCHK_REC+PCHK_COUNT*PCHK_NAME+4
    while True:
        m=data.find(PCHK_MAGIC,pos)
        if m<0:break
        if m+need<=len(data):
            rec0=m+PCHK_HEAD;name0=rec0+PCHK_COUNT*PCHK_REC
            names=[];ok=True
            for i in range(PCHK_COUNT):
                n=_printable_name(data[name0+i*PCHK_NAME:name0+(i+1)*PCHK_NAME])
                if n is None:ok=False;break
                names.append(n)
            sel=data[name0+PCHK_COUNT*PCHK_NAME] if ok else 255
            if ok and sel<PCHK_COUNT:
                records=[data[rec0+i*PCHK_REC:rec0+(i+1)*PCHK_REC] for i in range(PCHK_COUNT)]
                out.append((records,names,sel));pos=m+need;continue
        pos=m+1
    return out

def safe_name(s:str,suffix:str='')->str:
    s=''.join(c if 32<=ord(c)<=126 else ' ' for c in s).strip();s=' '.join(s.split())
    if suffix:
        keep=max(1,19-len(suffix));s=s[:keep].rstrip()+suffix
    return (s or 'IMPORTED')[:19]

def ctrl(op:int,slot:int|None=None,name:str|None=None)->bytes:
    b=bytearray(CTRL_PREFIX);b.append(op)
    if slot is not None:
        if not 0<=slot<USER_SLOTS:raise ValueError(f'slot outside 0..{USER_SLOTS-1}')
        b.append(slot)
    if name is not None:b.extend(name.encode('ascii','replace')[:19])
    b.append(0xf7);return bytes(b)

def plugin_record(slot:int,record:bytes)->bytes:
    if len(record)!=34 or not 0<=slot<USER_SLOTS:raise ValueError('bad plug-in record')
    b=bytearray(PLUGIN_PREFIX);b.append(slot)
    for v in record:b.extend(((v>>4)&15,v&15))
    b.append(0xf7);return bytes(b)

def smf(messages:Iterable[bytes],gap_ms:int)->bytes:
    ticks=max(96,int(round(max(100,gap_ms)*.96)));tr=bytearray();first=True
    for msg in messages:
        tr.extend(vlq(0 if first else ticks));first=False;tr.append(0xf0);payload=msg[1:] if msg and msg[0]==0xf0 else msg;tr.extend(vlq(len(payload)));tr.extend(payload)
    tr.extend(b'\x00\xff\x2f\x00')
    return b'MThd'+(6).to_bytes(4,'big')+b'\x00\x00\x00\x01\x01\xe0'+b'MTrk'+len(tr).to_bytes(4,'big')+tr

def sources(path:Path):
    data=path.read_bytes()
    if zipfile.is_zipfile(io.BytesIO(data)):
        with zipfile.ZipFile(io.BytesIO(data)) as z:
            for n in z.namelist():
                if n.endswith('/'):continue
                try:d=z.read(n)
                except Exception:continue
                yield f'{path}!{n}',Path(n),d
    else:yield str(path),path,data

def main()->int:
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('inputs',nargs='+',type=Path);ap.add_argument('-o','--output',type=Path,default=Path('FM1-BassStation-USER-Bank'));ap.add_argument('--start',type=int,default=0);ap.add_argument('--gap-ms',type=int,default=500);ns=ap.parse_args()
    if not 0<=ns.start<USER_SLOTS:ap.error(f'--start must be 0..{USER_SLOTS-1}')
    records=[]
    for root in ns.inputs:
        for src,member,data in sources(root):
            frames=unique_frames(data)
            for i,f in enumerate(frames):
                suffix=f' {i+1:02d}' if len(frames)>1 else ''
                records.append(('rack',f,safe_name(member.stem,suffix),src))
            # Prefer standards-compliant VST2 opaque chunks when a wrapper is present.
            vst=scan_vst2(data)
            bank_inputs=[]
            if vst:
                for vk,chunk,vname in vst:
                    bank_inputs.append((chunk, vk=='program', vname))
            else:
                bank_inputs.append((data, member.suffix.lower() in ('.fxp','.pchk'), member.stem))
            for payload,single,vname in bank_inputs:
                banks=scan_tffp(payload)
                for recs,names,sel in banks:
                    if single:
                        records.append(('plugin',recs[sel],safe_name(names[sel] or vname),src))
                    else:
                        for r,n in zip(recs,names):records.append(('plugin',r,safe_name(n),src))
    # De-duplicate exact patch payloads while preserving the first useful name/source.
    uniq=[];seen=set()
    for kind,payload,name,src in records:
        key=(kind,payload)
        if key not in seen:seen.add(key);uniq.append((kind,payload,name,src))
    records=uniq
    if not records:raise SystemExit('No compatible Bass Station Rack frames, VST2 chunks, or tffp plug-in state records found.')

    def write_batch(batch,base,start,batch_no,total_batches):
        messages=[]
        manifest=['# FM1 Bass Station TEST11 USER/COMMUNITY bank',f'# batch: {batch_no}/{total_batches}',f'# start slot: {start}',f'# patches: {len(batch)}']
        slot=start
        for kind,payload,name,src in batch:
            if kind=='rack':
                # Start/stop capture around each Rack patch so mixed Rack + native
                # plug-in inputs always land in the exact manifest slot.
                messages.append(ctrl(0x20,slot));messages.append(payload);messages.append(ctrl(0x21))
            else:
                messages.append(plugin_record(slot,payload))
            messages.append(ctrl(0x25,slot,name));manifest.append(f'{slot+1:03d}\t{name}\t{kind}\t{src}');slot+=1
        base.parent.mkdir(parents=True,exist_ok=True)
        Path(str(base)+'.syx').write_bytes(b''.join(messages));Path(str(base)+'.mid').write_bytes(smf(messages,ns.gap_ms));Path(str(base)+'.txt').write_text('\n'.join(manifest)+'\n',encoding='utf-8')
        print(f'Batch {batch_no}/{total_batches}: {len(batch)} patch(es), USER slots {start+1}..{start+len(batch)} -> {base}')

    # The first batch honours --start. Additional batches begin at USER slot 1
    # and are separate load sets, allowing archives larger than the FM1 USER-bank capacity to
    # be preserved without silently dropping material.
    room=USER_SLOTS-ns.start; batches=[]
    first=records[:room];batches.append((first,ns.start));left=records[room:]
    while left:
        batches.append((left[:USER_SLOTS],0));left=left[USER_SLOTS:]
    multi=len(batches)>1
    for i,(batch,start) in enumerate(batches,1):
        base=ns.output if not multi else Path(f'{ns.output}-{i:02d}')
        write_batch(batch,base,start,i,len(batches))
    if multi:print(f'Prepared {len(records)} patches as {len(batches)} FM1 USER-bank load sets ({USER_SLOTS} slots max each).')
    else:print(f'Prepared {len(records)} patch(es).')
    return 0
if __name__=='__main__':raise SystemExit(main())
