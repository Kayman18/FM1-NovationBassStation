from pathlib import Path
import importlib.util, tempfile, zipfile, subprocess, sys
root=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('prep',root/'tools/prepare_bassstation_bank.py');m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)

def mk_tffp(selected=7):
    names=[f'P{i:02d}'.encode() for i in range(100)]
    blob=bytearray(b'xxxx'+b'tffp'+b'\0'*5)
    blob.extend(b''.join(bytes([i&255])*34 for i in range(100)))
    for n in names:blob.extend(n+b'\0'*(21-len(n)))
    blob.extend(bytes((selected,1,0x7f,0x7f)))
    return bytes(blob),names

def vst2(kind:bytes,chunk:bytes,name=b'TEST PROGRAM',fxid=b'Nvb2'):
    if kind==b'FPCh':
        body=kind+(1).to_bytes(4,'big')+fxid+(2100).to_bytes(4,'big')+(68).to_bytes(4,'big')+name[:28].ljust(28,b'\0')+len(chunk).to_bytes(4,'big')+chunk
    elif kind==b'FBCh':
        body=kind+(2).to_bytes(4,'big')+fxid+(2100).to_bytes(4,'big')+(100).to_bytes(4,'big')+bytes(128)+len(chunk).to_bytes(4,'big')+chunk
    else: raise ValueError(kind)
    return b'CcnK'+len(body).to_bytes(4,'big')+body

f=bytearray(76);f[:7]=m.RACK_PREFIX;f[-1]=0xF7
assert m.valid_rack(bytes(f));assert m.unique_frames(b'junk'+bytes(f)+b'tail')==[bytes(f)]
msgs=[m.ctrl(0x20,3),bytes(f),m.ctrl(0x25,3,'TEST PATCH'),m.ctrl(0x21)];mid=m.smf(msgs,500);assert m.scan_smf(mid)==[bytes(f)]
assert m.ctrl(0x25,3,'TEST PATCH').startswith(b'\xF0\x7D\x42\x53\x07\x25\x03TEST PATCH')
# TEST15 native 34-byte plug-in record: nibble-expanded and 7-bit clean.
r=bytes(range(34));msg=m.plugin_record(7,r);assert len(msg)==76 and msg[:7]==b'\xF0\x7D\x42\x53\x08\x30\x07' and msg[-1]==0xf7 and all(x<128 for x in msg[1:-1])
assert bytes((msg[7+i*2]<<4)|msg[8+i*2] for i in range(34))==r
# Minimal synthetic tffp bank.
blob,names=mk_tffp(7);banks=m.scan_tffp(blob);assert len(banks)==1 and banks[0][2]==7 and banks[0][1][7]=='P07' and banks[0][0][7]==bytes([7])*34
# Standards-compliant VST2 opaque wrappers and Bass Station ID filtering.
fxp=vst2(b'FPCh',blob,b'P07');fxb=vst2(b'FBCh',blob)
vp=m.scan_vst2(fxp);vb=m.scan_vst2(fxb)
assert len(vp)==1 and vp[0][0]=='program' and vp[0][1]==blob
assert len(vb)==1 and vb[0][0]=='bank' and vb[0][1]==blob
assert m.scan_vst2(vst2(b'FPCh',blob,fxid=b'NOPE'))==[]
print('prepare_bassstation_bank TEST15 parsers: PASS')

# Mixed archive regression: Rack capture must be bracketed per patch so explicit
# native records and Rack records cannot overwrite each other.
with tempfile.TemporaryDirectory() as td:
    td=Path(td); zpath=td/'mixed.zip'
    with zipfile.ZipFile(zpath,'w') as z:
        z.writestr('rack.syx',bytes(f))
        z.writestr('selected.fxp',fxp)
    out=td/'out'
    subprocess.check_call([sys.executable,str(root/'tools/prepare_bassstation_bank.py'),str(zpath),'--start','10','-o',str(out)])
    sx=Path(str(out)+'.syx').read_bytes()
    start=m.ctrl(0x20,10); stop=m.ctrl(0x21); native=m.plugin_record(11,bytes([7])*34)
    assert sx.startswith(start+bytes(f)+stop+m.ctrl(0x25,10,'rack'))
    assert native in sx and m.ctrl(0x25,11,'P07') in sx
print('prepare_bassstation_bank mixed-slot TEST15: PASS')

# A full FXB plus 36 distinct Rack patches exceeds the 128-slot USER capacity. TEST15 must
# Split it into two load sets rather than dropping patches or aborting.
with tempfile.TemporaryDirectory() as td:
    td=Path(td); zpath=td/'large.zip'
    with zipfile.ZipFile(zpath,'w') as z:
        z.writestr('factory.fxb',fxb)
        for i in range(36):
            rf=bytearray(f);rf[7]=i&15;rf[8]=(i>>4)&15;z.writestr(f'user{i}.syx',bytes(rf))
    out=td/'community'
    subprocess.check_call([sys.executable,str(root/'tools/prepare_bassstation_bank.py'),str(zpath),'-o',str(out)])
    a=Path(str(out)+'-01.txt');b=Path(str(out)+'-02.txt')
    assert a.exists() and b.exists()
    assert '# patches: 128' in a.read_text() and '# patches: 8' in b.read_text()
    assert Path(str(out)+'-01.mid').exists() and Path(str(out)+'-02.mid').exists()
print('prepare_bassstation_bank TEST15 multi-batch: PASS')
