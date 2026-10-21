#!/usr/bin/env python3
"""Static package integrity only. Does not run inference or FPGA tools."""
from pathlib import Path
import hashlib,json,struct,array,sys,re,ast
ROOT=Path(__file__).resolve().parents[1]
def digest(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def require(condition,message):
    if not condition:raise RuntimeError(message)
def read_exact(f,n):
    v=f.read(n);require(len(v)==n,'Truncated binary file');return v
def u16(data):
    a=array.array('H');a.frombytes(data)
    if sys.byteorder!='little':a.byteswap()
    return a
checksums=ROOT/'provenance/SHA256SUMS'
require(checksums.exists(),'SHA256SUMS missing')
count=0
for line in checksums.read_text().splitlines():
    expected,name=line.split('  ',1);p=ROOT/name
    require(p.is_file() and digest(p)==expected,'Hash mismatch: '+name);count+=1
external={'ap_int.h','ap_fixed.h','hls_math.h','hls_stream.h'}
includes=0
for p in list((ROOT/'designs').rglob('*.cpp'))+list((ROOT/'designs').rglob('*.h'))+list((ROOT/'testbench').glob('*.h')):
    for inc in re.findall(r'^\s*#\s*include\s*"([^"\n]+)"',p.read_text(),re.M):
        require((p.parent/inc).is_file() or inc in external,'Missing local include '+str(p.relative_to(ROOT))+': '+inc);includes+=1
parameter_counts=json.loads((ROOT/'weights/parameter_counts.json').read_text());total=0
for name,n in parameter_counts['files'].items():
    values=(ROOT/'weights'/name).read_text().replace(',',' ').split();require(len(values)==n,'Weight count mismatch: '+name)
    for value in values:float(value)
    total+=len(values)
require(total==528,'Parameter count mismatch')
summary=[]
for family in ['Graph_M','Graph_L']:
    sample=ROOT/'samples'/family;config=json.loads((ROOT/'designs'/family/'config.json').read_text());meta=json.loads((sample/'metadata.json').read_text())
    masks=(sample/'valid_mask.bin').read_bytes();expected=(sample/'expected_raw.bin').read_bytes();n=config['MAX_NODE'];e=config['TOTAL_EDGE_PER_GRAPH'];graphs=meta['batch_graphs']
    require(len(masks)==graphs*e and len(expected)==2*graphs*e,'Mask/expected size mismatch')
    require(all(x in (0,1) for x in masks),'Invalid mask bytes')
    scores=array.array('h');scores.frombytes(expected)
    if sys.byteorder!='little':scores.byteswap()
    require(all(0<=x<=256 for x in scores),'Expected score outside [0,1]')
    with (sample/'input.bin').open('rb') as f:
        require(struct.unpack('<III',read_exact(f,12))==(graphs,n,e),'Input header mismatch')
        for g in range(graphs):
            f.seek(4*(n*3+e*4),1)
            pairs=u16(read_exact(f,e*4));valid=0;previous=-1;seen_padding=False
            for i in range(e):
                s,d=pairs[2*i],pairs[2*i+1];is_valid=masks[g*e+i]
                require(s<n and d<n and d>=previous,'Endpoint/order violation');previous=d
                if is_valid:
                    require(not seen_padding and s!=n-1 and d!=n-1,'Valid edge uses dummy ID or follows dummy tail');valid+=1
                else:require(s==n-1 and d==n-1,'Invalid padding endpoint');seen_padding=True
            require(seen_padding,'No terminal dummy tail')
            source_count=struct.unpack('<I',read_exact(f,4))[0];require(source_count<=config['SRC_NODE'],'Source metadata overflow')
            ids=u16(read_exact(f,source_count*2));require(all(x<n-1 for x in ids),'Invalid source metadata')
            require(valid==meta['graphs'][g]['valid_edges'],'Valid count mismatch')
            require(source_count==meta['graphs'][g]['source_nodes'],'Source count mismatch')
        require(f.read(1)==b'','Unexpected input bytes')
    summary.append({'design':family,'graphs':graphs,'edge_slots_per_graph':e,'valid_edges':sum(masks)})
# Syntax/cleanup checks do not import or run packaged model/build scripts.
for p in (ROOT/'scripts').glob('*.py'):ast.parse(p.read_text(),filename=p.name)
for p in ROOT.rglob('*'):
    if not p.is_file() or '.build' in p.parts:continue
    require(p.suffix.lower() not in {'.pdf','.dcp','.lic','.log','.jou','.pyc'},'Excluded file type: '+str(p))
    if p.suffix in {'.md','.cpp','.h','.tcl','.json','.xdc','.py'} or p.name in {'README','LICENSE','NOTICE'}:
        t=p.read_text();require(not re.search(r'/(?:home|cad)/[^\s"\'<>]+',t),'Private absolute path: '+str(p.relative_to(ROOT)))
provenance=json.loads((ROOT/'provenance/final_design_provenance.json').read_text())
for family,info in provenance['designs'].items():
    for item in info['core_source_files']:require(digest(ROOT/item['file'])==item['sha256'],'Canonical source mismatch')
    for snippet in json.loads((ROOT/'docs/snippet_inventory.json').read_text()):
        p=ROOT/snippet['file'];chunk=b''.join(p.read_bytes().splitlines(keepends=True)[snippet['start_line']-1:snippet['end_line']]);require(hashlib.sha256(chunk).hexdigest()==snippet['sha256'],'Snippet line/hash mismatch')
print(json.dumps({'status':'PASS_STATIC_PACKAGING_ONLY','files_hash_checked':count,'quoted_includes_checked':includes,'parameter_count':total,'samples':summary,'inference_or_HLS_or_Vivado_run':False,'new_portable_TB_validation':'PASS_VITIS_HLS_2023_2_CSIM_SEE_VALIDATION','license':'PENDING','sample_redistribution':'PENDING'},indent=2))
