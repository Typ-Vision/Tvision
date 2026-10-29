"""Independent COMM4-02 byte/word oracle (no imports from generate.py).

Checks committed data rather than invoking the generator. --write-words emits
the independently calculated PLC tables; ordinary verification checks them.
No sockets, production codec, runtime admission or hardware are exercised.
"""
import argparse
import csv
from decimal import Decimal, localcontext, ROUND_HALF_EVEN
import hashlib
import io
import json
import math
from pathlib import Path
import re
import struct

ROOT=Path(__file__).resolve().parent
parser=argparse.ArgumentParser(); parser.add_argument('--write-words',action='store_true'); args=parser.parse_args()
m=json.loads((ROOT/'manifest.json').read_text(encoding='utf-8'))
checks=0
def check(cond,msg):
    global checks
    checks+=1
    if not cond: raise AssertionError(msg)
def u(b,o,n=4): return int(b[o:o+n].hex(),16)
def spans(widths,start,end):
    offset=start; result=[]
    for width in widths:
        check(width>0,'positive width'); result.append((offset,offset+width)); offset+=width
    check(offset==end,f'layout end {offset}, expected {end}'); return result
# Independent width arithmetic: these are not generated from the fixture writer.
header=spans([4,4,4,4,4,4,8,16,16,16,4,4,16,24],0,128)
pose=spans([24,32,16,16,16,16,4,4],0,128)
operational=spans([16,16,32]+[4]*10+[16]*3+[64,272,128,4,128,8]+[4]*4+[16]*3+[4,4,68],128,1024)
hello=spans([16,16,16]+[4]*8+[816],128,1024)
catalog=spans([16,4,4,16,16,16,4,64,4,640,4,4,104],128,1024)
request_widths={1:[16,4,364],2:[384],3:[4,4,16,360],4:[4,4,376],5:[4,380],6:[384],10:[16,16,16,336],11:[16,32,336],12:[16,368],20:[16,4,4,16,4,4,4,4,128,200],21:[16,16,4,4,16,8,8,4,4,128,4,4,168],22:[16,32,4,332],23:[16,16,16,32,4,4,296],24:[16,16,16,16,16,32,272],25:[16,4,364],30:[16,16,352]}
request_reserved={k:spans(v,128,512)[-1] for k,v in request_widths.items()}
for cat in range(1,14):
    fields=m['metadata'][str(cat)]; occupied=set(range(72))
    for f in fields:
        size=16 if f['format']=='16s' else 4
        check(f['offset']%4==0,'metadata word alignment')
        area=set(range(f['offset'],f['offset']+size))
        check(not area&occupied,'metadata overlap'); occupied|=area
    check(max(occupied)<256,'metadata capacity')

frames={}; words=io.StringIO(newline=''); w=csv.writer(words,lineterminator='\n')
w.writerow(['fixture','byteOffset','uint32Hex','uint32Decimal','uint16High','uint16Low'])
for case in m['cases']:
    name=case['name']; b=(ROOT/'golden'/(name+'.bin')).read_bytes(); frames[name]=b
    check(b.hex()==(ROOT/'golden'/(name+'.hex')).read_text().strip(),name+' hex/bin')
    check(len(b)==case['bytes'] and len(b) in (512,1024),name+' exact length')
    check(hashlib.sha256(b).hexdigest()==case['sha256'],name+' manifest SHA')
    for off in range(0,len(b),4):
        h=b[off:off+4].hex(); value=int(h[:4],16)*65536+int(h[4:],16)
        w.writerow([name,off,h,value,int(h[:4],16),int(h[4:],16)])
    if case['outcome']!='valid': continue
    kind=u(b,8)
    check(b[:4]==bytes([86,70,48,52]) and u(b,4)==4 and u(b,12)==len(b),name+' header')
    check(b[16:20]==bytes(4) and b[104:128]==bytes(24),name+' header reserved')
    check(u(b,24,8)>0,name+' request nonzero')
    pose_off=184 if kind==20 else 208 if kind==21 else 616 if kind==111 and u(b,224) else None
    if pose_off is not None:
        check(not any(b[pose_off+124:pose_off+128]),name+' PoseBlock reserved')
        check(all(any(b[pose_off+i:pose_off+i+16]) for i in (56,72,88,104)),name+' pose frame/revision nonzero')
        nums=struct.unpack('>7d',b[pose_off:pose_off+56])
        check(all(math.isfinite(x) for x in nums),name+' finite PoseBlock')
        if kind==20: check(not any(b[pose_off:pose_off+56]) and u(b,pose_off+120)==2,name+' contract-only pose')
        else: check(abs(math.sqrt(sum(x*x for x in nums[3:]))-1)<=1e-6 and u(b,pose_off+120)==1,name+' quaternion pose convention/norm')
    if kind<100:
        a,z=request_reserved[kind]; check(not any(b[a:z]),name+' request reserved')
        payload=b'vf.socket4.request/1'+bytes(1)+b[8:12]+b[20:24]+b[24:32]+b[48:64]+b[64:80]+b[80:84]+b[84:88]+b[88:104]+b[128:512]
        check(hashlib.sha256(payload).hexdigest()==case['requestFingerprint'],name+' fingerprint')
    elif kind==101: check(not any(b[208:]),name+' hello reserved')
    elif kind==103:
        check(not any(b[920:]),name+' catalog reserved')
        n=u(b,200); check(n<=64 and not any(b[204+n:268]),name+' name padding')
        b[204:204+n].decode('utf-8',errors='strict')
        check(u(b,268)==256 and u(b,272)==1 and u(b,276)==256,name+' metadata fixed schema')
        check(not any(b[528:912]),name+' metadata tail')
        fields=m['metadata'][str(u(b,144))]; used=set(range(72))
        for f in fields:
            n=16 if f['format']=='16s' else 4; off=272+f['offset']; used.update(range(f['offset'],f['offset']+n))
            actual=b[off:off+n].hex() if n==16 else u(b,off)
            check(actual==f['value'],name+' metadata '+f['name'])
        check(all(b[272+i]==0 for i in range(256) if i not in used),name+' metadata reserved')
    else:
        check(not any(b[956:]),name+' operational reserved')
        if kind==111:
            payload=(b'vf.socket4.terminal/1'+bytes(1)+b[20:24]+b[24:32]+b[48:64]+b[64:80]+b[80:84]+b[84:88]+b[88:104]+b[128:160]+bytes(32)+b[192:])
            check(hashlib.sha256(payload).digest()==b[160:192],name+' terminal digest')
            check(case['terminalDigest']==b[160:192].hex(),name+' manifest terminal digest')
        for i in range(4):
            off=344+68*i; n=u(b,off); check(n<=64,name+' text limit')
            b[off+4:off+4+n].decode('utf-8',errors='strict')
            check(not any(b[off+4+n:off+68]),name+' text zero padding')
        check(u(b,208)<65536 and u(b,220)<16,name+' validity reserved bits')
        check(u(b,216)&~u(b,212)==0,name+' invalid flag value')
        for i in range(16):
            if not (u(b,208)>>i)&1: check(not any(b[280+4*i:284+4*i]),name+' missing Number zero')
        if not u(b,224): check(not any(b[616:744]),name+' missing pose zero')

text=words.getvalue()
if args.write_words: (ROOT/'plc-words.csv').write_text(text,encoding='ascii',newline='')
check((ROOT/'plc-words.csv').read_text(encoding='ascii')==text,'independent PLC table')
decoded=json.loads((ROOT/'expected-fields.json').read_text())
for name,offset,width,expected in decoded['fields']:
    b=frames[name]
    actual='hex:'+b[offset:offset+width].hex() if expected.startswith('hex:') else str(u(b,offset,width))
    check(actual==expected,name+' independent decoded field '+str(offset))
check({u(b,8) for b in frames.values() if u(b,8)<100} >= set(request_widths),'all request kinds')
check({u(b,8) for b in frames.values() if u(b,8)>=100} >= {101,102,103,110,111,112,113,190,191},'all response kinds')
# Literal, hand-calculated numeric and PLC anchors; NOT taken from the writer.
t=frames['terminal']; trig=frames['trigger']; a=frames['ack_exact']
check(trig[24:32].hex()=='0020000000000001','uint64 above 2^53 high/low words')
check(frames['request_18446744073709551615'][24:32]==b'\xff'*8,'uint64 MAX')
check(trig[48:64].hex()=='00112233445546778899aabb00000002','canonical leading-zero UUID')
check([u(trig,o) for o in (48,52,56,60)]==[0x00112233,0x44554677,0x8899aabb,2],'UUID PLC words')
check(t[280:288].hex()=='800000007fffffff','signed32 two complement extremes')
check(int.from_bytes(t[280:284],'big',signed=True)==-2147483648,'INT_MIN decode')
check(u(t,212)==0xffffffff and u(t,216)==0x80000001,'flag0/31')
check(u(t,344)==0 and u(t,412)==64 and t[416:480]==bytes.fromhex('c3a9')*32,'empty/64 UTF8 exact')
check(a[128:144]==t[128:144] and a[144:176]==t[160:192],'exact ACK result/digest')
check(frames['acked'][160:192]==t[160:192] and not any(frames['acked'][192:228]),'ACKED echo, no rehash')
check(u(frames['calib_sample_holdout'],340)==2,'holdout role')
check(u(frames['terminal_sample_holdout'],952)==21 and u(frames['terminal_sample_holdout'],948)==4,'sample origin/camera')
check(u(frames['terminal_stop_control'],892)==2 and u(frames['terminal_abort_control'],892)==2,'normal plus control reservation')
check(u(frames['terminal_select_recipe'],80)==21 and frames['terminal_select_recipe'][88:104]!=trig[88:104],'SELECT new snapshot')
check(frames['status_reply'][80:104]==trig[80:104] and not any(frames['status_reply'][192:228]),'STATUS actual snapshot, no result')

# Negative oracle, deliberately local and bounded to these mutations. This is
# neither a general-purpose decoder nor proof of the future server's behavior.
def negative_reason(b):
    kind=u(b,8)
    if b[:4]!=b'VF04' or u(b,12)!=len(b) or u(b,16) or kind not in set(request_widths)|{101,102,103,110,111,112,113,190,191}: return 'BAD_FRAME'
    if u(b,4)!=4: return 'UNSUPPORTED_VERSION'
    if u(b,24,8)==0: return 'INVALID_VALUE'
    if kind==10:
        if any(b[176:]): return 'BAD_FRAME'
        if b[32:48]!=trig[32:48]: return 'STALE_SESSION'
        if b[88:104]!=trig[88:104]: return 'STALE_CONFIG'
        if b[48:64]!=trig[48:64]: return 'unknown peer, not canonical UUID'
    if kind==11 and (b[144:176]!=t[160:192] or b[128:144]!=t[128:144] or b[20:32]!=t[20:32] or b[48:104]!=t[48:104]): return 'ACK_MISMATCH'
    if kind==103:
        if u(b,272)!=1: return 'INVALID_VALUE'
        if any(b[528:912]): return 'BAD_FRAME'
    if kind==21:
        if b[280:296]!=frames['calib_sample'][280:296]: return 'STALE_CONFIG'
        nums=struct.unpack('>7d',b[208:264]); norm=math.sqrt(sum(x*x for x in nums[3:]))
        if not all(math.isfinite(x) for x in nums) or abs(norm-1)>1e-6 or u(b,328)!=1 or u(b,340) not in (1,2): return 'INVALID_VALUE'
    if kind==111:
        if u(b,208)>65535: return 'BAD_FRAME'
        for off in (344,412,480,548):
            n=u(b,off)
            if n>64: return 'INVALID_VALUE'
            try: b[off+4:off+4+n].decode('utf-8',errors='strict')
            except UnicodeDecodeError: return 'INVALID_VALUE'
            if any(b[off+4+n:off+68]): return 'BAD_FRAME'
        if b[280:284]==bytes.fromhex('00008000'): return 'decoded 32768, not INT_MIN'
    return 'valid'
for case in m['cases']:
    if case['outcome']!='valid': check(negative_reason(frames[case['name']])==case['outcome'],'negative expected '+case['name'])
check(frames['terminal_reconnect_session'][128:]==t[128:],'reconnect exact sealed body/digest')
fps={c['name']:c.get('requestFingerprint') for c in m['cases']}
check(fps['trigger_reconnect_session']==fps['trigger'],'fingerprint session exclusion')

# Exact rational conversion oracle and quaternion policy vectors (design only).
semantic=json.loads((ROOT/'semantic-vectors.json').read_text())
with localcontext() as ctx:
    ctx.prec=90
    for v in semantic['number']:
        scaled=(Decimal(v['input'])*Decimal(v['numerator'])/Decimal(v['denominator'])+Decimal(v['offset']))
        rounded=int(scaled.to_integral_value(rounding=ROUND_HALF_EVEN))
        actual=rounded if -2**31<=rounded<2**31 else 'INVALID_VALUE'
        check(actual==v['expected'],'rational number '+v['name'])
for v in semantic['quaternion']:
    q=v['input']; norm=math.sqrt(sum(x*x for x in q))
    valid=all(math.isfinite(x) for x in q) and norm>0 and abs(norm-1)<=1e-6
    check(valid==v['valid'],'quaternion acceptance '+v['name'])
    if valid:
        q=[x/norm for x in q]; first=next(x for x in q if x!=0)
        q=[0. if x==0 else (-x if first<0 else x) for x in q]
        check(all(abs(x-y)<=1e-15 for x,y in zip(q,v['canonical'])),'quaternion canonical '+v['name'])
# Nontrivial 120deg cyclic rotation: R maps (x,y,z)->(z,x,y).
check(struct.unpack('>7d',t[616:672])==(1.,2.,3.,.5,.5,.5,.5),'nontrivial pose')
inv=frames['pose_inverse']; check(struct.unpack('>7d',inv[616:672])==(-2.,-3.,-1.,.5,-.5,-.5,-.5),'inverse numeric ground truth')
check(inv[672:704]==t[704:736] and inv[704:736]==t[672:704],'inverse frame/revision swap')
check(frames['pose_identity'][640:648].hex()=='3ff0000000000000','Float64 identity high/low words')
# Stream fixtures describe expectations for future codec; do not claim framer test.
for v in semantic['stream']:
    stream=b''.join(frames[n] for n in v['frames'])
    check(len(stream)==v['bytes'],'stream byte count')
    if v['name']=='all_split_positions':
        check(all(stream[:i]+stream[i:]==trig for i in range(1,512)),'511 split reconstructions')
    if v['name']=='four_coalesced': check(len(stream)==2048,'four requests=read max')
# Check authoritative document table arithmetic independently of width arrays.
wire=(ROOT.parents[2]/'docs/architecture/communication-v4/wire.md').read_text(encoding='utf-8')
request_names={'HELLO':1,'STATUS':2,'CATALOG_GET':3,'SELECT_RECIPE':4,'SET_AUTO':5,'STOP':6,'TRIGGER':10,'ACK':11,'RECOVER':12,'CALIB_BEGIN':20,'CALIB_SAMPLE':21,'CALIB_SOLVE':22,'CALIB_VALIDATE':23,'CALIB_ACTIVATE':24,'CALIB_ABORT':25,'EPOCH_ROLLOVER':30}
w3=wire.split('## W3 ')[1].split('\n## ')[0]
for name,kind in request_names.items():
    row=re.search(r'^\| '+name+r' \| (.+) \|$',w3,re.M).group(1)
    if kind in (2,6): check('Body384 reserved0' in row,name+' body zero'); continue
    offsets=[int(x) for x in re.findall(r'(?:^|; )(\d+)(?:–\d+)? ',row)]
    expected=[]; pos=128
    for width in request_widths[kind]: expected.append(pos); pos+=width
    check(offsets==expected,name+' W3 authoritative offsets')
w6a=wire.split('## W6a ')[1].split('\n## ')[0]
for category,fields in m['metadata'].items():
    row=re.search(r'^\|'+category+r' [^|]+\|(.+)\|$',w6a,re.M).group(1).strip()
    offsets=[int(x) for x in re.findall(r'(?:^|;)(\d+)(?:–\d+)? ',row)]
    last=fields[-1]; tail=last['offset']+(16 if last['format']=='16s' else 4)
    check(offsets==[f['offset'] for f in fields]+[tail],f'category{category} doc offsets')
check(set(m['enums']['reason'])=={str(i) for i in range(26)},'reason code closed range')
check(m['enums']['reason']['25']=='REMOTE_DISABLED','remote policy enum')
for section,start,end in [('W1',0,128),('W4',0,128),('W5',128,1024)]:
    block=wire.split('## '+section+' ')[1].split('\n## ')[0]
    rows=re.findall(r'^\| (\d+)–(\d+) \| (\d+) \|',block,re.M)
    pos=start
    for lo,hi,n in rows:
        lo,hi,n=map(int,(lo,hi,n)); check(lo==pos and hi-lo+1==n,section+' doc gap/overlap/width'); pos=hi+1
    check(pos==end,section+' doc exact span')
print(json.dumps(dict(status='PASS',checks=checks,frames=len(frames),requestKinds=16,responseKinds=9,metadataCategories=13,skipped=0,profile='D'),indent=2))
