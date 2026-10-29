"""COMM4-02 design fixtures only; never imported by the production application.

Explicit offsets and struct network packing form one oracle. verify.py uses
hex arithmetic, independent layouts and semantic anchors as the second oracle.
Regeneration is deliberate: python generate.py; python verify.py.
"""
import hashlib
import json
from pathlib import Path
import struct

ROOT = Path(__file__).resolve().parent
REV = '00434634-0002-4000-8000-202610290001'
U = lambda n: bytes.fromhex(f'00112233445546778899aabb{n:08x}')
R = bytes.fromhex(REV.replace('-', ''))
REQUESTS = dict(HELLO=1, STATUS=2, CATALOG_GET=3, SELECT_RECIPE=4,
                SET_AUTO=5, STOP=6, TRIGGER=10, ACK=11, RECOVER=12,
                CALIB_BEGIN=20, CALIB_SAMPLE=21, CALIB_SOLVE=22,
                CALIB_VALIDATE=23, CALIB_ACTIVATE=24, CALIB_ABORT=25,
                EPOCH_ROLLOVER=30)
RESPONSES = dict(HELLO_REPLY=101, STATUS_REPLY=102, CATALOG_REPLY=103,
                 ACCEPTED=110, TERMINAL=111, ACKED=112, STATE=113,
                 REJECT=190, PROTOCOL_ERROR=191)
ENUMS = dict(request=REQUESTS, response=RESPONSES,
    reason=dict(enumerate('NONE NOT_AUTO STATION_BUSY CAMERA_BUSY STALE_CONFIG STALE_SESSION CONFLICT QUEUE_FULL RECORDING_ERROR RECOVERY_REQUIRED UNKNOWN_KEY INVALID_VALUE UNSUPPORTED_VERSION BAD_FRAME ACK_MISMATCH ACK_TIMEOUT LEASE_MISMATCH PHYSICAL_GATE_BLOCKED PEER_BUSY RETIRED_EPOCH NOT_READY CAPTURE_TIMEOUT RESULT_INVALID RESOURCE_LIMIT CONTRACT_MISMATCH REMOTE_DISABLED'.split())),
    commandState=dict(enumerate('NONE ACCEPTED RUNNING TERMINAL ACK_COMMITTED COLLECTING CAPTURED CAPTURE_FAILED CANDIDATE VALIDATED ACTIVE ABORTED RECOVERY_REQUIRED'.split())),
    serverState={0:'NOT_CONFIGURED',1:'LISTENING',2:'PEER_CONNECTED',3:'FAULT'},
    stationState={0:'STOPPED',1:'PREPARING',2:'ARMED_MANUAL',3:'ARMED_AUTO',4:'RUNNING',5:'DRAINING',6:'BLOCKED'},
    calibrationState={0:'CREATED',1:'COLLECTING',2:'SOLVING',3:'CANDIDATE',4:'VALIDATING',5:'VALIDATED',6:'ACTIVE',7:'FAILED',8:'ABORTED'})
def put(b, off, fmt, *v):
    struct.pack_into('>'+fmt, b, off, *v)
def ident(b, off, n): b[off:off+16] = U(n)
def frame(kind, station=1, request=9007199254740993):
    b = bytearray(512 if kind < 100 else 1024)
    put(b, 0, '4sIIIIIQ', b'VF04',4,kind,len(b),0,station,request)
    for off,n in [(32,1),(48,2),(64,3),(88,4)]: ident(b,off,n)
    put(b,80,'II',11,12)
    if not station: b[80:104] = bytes(24)
    return b
def pose(q=(1.,0.,0.,0.), xyz=(0.,0.,0.), contract=False, inverse=False):
    b = bytearray(128)
    put(b,0,'7d',*xyz,*q)
    for off,n in [(56,10),(72,11),(88,12),(104,13)]: ident(b,off,n)
    if inverse: b[56:88], b[88:120] = b[88:120], b[56:88]
    put(b,120,'I',2 if contract else 1)
    return b
def terminal_digest(b):
    body = bytearray(b[128:]); body[32:64] = bytes(32)
    return hashlib.sha256(b'vf.socket4.terminal/1\0'+b[20:32]+b[48:104]+body).digest()
def fingerprint(b):
    return hashlib.sha256(b'vf.socket4.request/1\0'+b[8:12]+b[20:32]+b[48:104]+b[128:]).digest()
def seal(b): b[160:192] = terminal_digest(b); return b
cases = {}
def add(name,b,outcome='valid',**kw):
    cases[name] = dict(data=bytes(b),outcome=outcome,**kw)

for name,kind in REQUESTS.items():
    b=frame(kind)
    if name=='HELLO':
        b=frame(kind,0); b[32:48]=bytes(16); b[128:144]=R
    elif name=='STATUS': b[80:104]=bytes(24)
    elif name=='CATALOG_GET':
        b=frame(kind,0); put(b,128,'II',1,0)
    elif name=='SELECT_RECIPE': put(b,128,'II',21,22)
    elif name=='SET_AUTO': put(b,128,'I',1)
    elif name=='TRIGGER': ident(b,128,20)
    elif name=='ACK': ident(b,128,30); b[144:176]=bytes.fromhex('ab'*32)
    elif name=='RECOVER': ident(b,128,30)
    elif name=='CALIB_BEGIN':
        ident(b,128,40); put(b,144,'II',1,4); ident(b,152,41)
        put(b,168,'IIII',5,6,0,8); b[184:312]=pose(q=(0.,0.,0.,0.),contract=True)
    elif name=='CALIB_SAMPLE':
        ident(b,128,40); ident(b,144,42); put(b,160,'II',4,1); ident(b,168,41)
        put(b,184,'QQII',2**64-1,2**53+1,13,1)
        b[208:336]=pose(q=(.5,.5,.5,.5),xyz=(1.,2.,3.)); put(b,336,'II',9,1)
    elif name=='CALIB_SOLVE':
        ident(b,128,40); b[144:176]=bytes.fromhex('cd'*32); put(b,176,'I',10)
    elif name=='CALIB_VALIDATE':
        for off,n in [(128,40),(144,43),(160,44)]: ident(b,off,n)
        b[176:208]=bytes.fromhex('de'*32); put(b,208,'II',11,12)
    elif name=='CALIB_ACTIVATE':
        for off,n in [(128,40),(144,43),(160,44),(176,45)]: ident(b,off,n)
        b[208:240]=bytes.fromhex('ef'*32)
    elif name=='CALIB_ABORT': ident(b,128,40); put(b,144,'I',3)
    elif name=='EPOCH_ROLLOVER':
        b=frame(kind,0); ident(b,128,50); ident(b,144,51)
    add(name.lower(),b)
add('hello_discovery',bytearray(cases['hello']['data'][:128])+bytearray(384))
for station in (1,2,3):
    b=bytearray(cases['trigger']['data']); put(b,20,'I',station); add(f'trigger_station{station}',b)
for val in (0,2**53+1,2**64-1):
    b=bytearray(cases['trigger']['data']); put(b,24,'Q',val)
    add(f'request_{val}',b,'INVALID_VALUE' if val==0 else 'valid')

for name,kind in RESPONSES.items():
    if name=='CATALOG_REPLY': continue
    b=frame(kind)
    if name=='HELLO_REPLY':
        b=frame(kind,0)
        ident(b,128,60); b[144:160]=R; ident(b,160,51)
        put(b,176,'8I',2,32,64,65536,2000,512,1024,0)
    elif name=='TERMINAL':
        ident(b,128,30); ident(b,144,31)
        put(b,192,'10I',1,1,1,0,0xffff,0xffffffff,0x80000001,0xf,1,3)
        put(b,280,'16i',-2**31,2**31-1,*range(-7,7))
        for i,s in enumerate(['','é'*32,'valid','last']):
            t=s.encode(); put(b,344+68*i,'I',len(t)); b[348+68*i:348+68*i+len(t)]=t
        b[616:744]=pose(q=(.5,.5,.5,.5),xyz=(1.,2.,3.))
        put(b,876,'QIIII',2**64-1,0,0,1,0); put(b,948,'II',4,10); seal(b)
    elif name=='STATUS_REPLY': put(b,884,'III',1,0,0)
    elif name=='ACCEPTED': put(b,228,'I',1); put(b,952,'I',10)
    elif name=='STATE': put(b,204,'I',9); put(b,228,'I',12)
    elif name=='REJECT': put(b,204,'I',1); put(b,952,'I',10)
    elif name=='PROTOCOL_ERROR': put(b,204,'I',13)
    elif name=='ACKED':
        t=cases['terminal']['data']; b[128:144]=t[128:144]; b[160:192]=t[160:192]; put(b,228,'I',4)
    add(name.lower(),b)

# Fixed 256-byte metadata payload: common 72 bytes + category-specific fields.
# Each tuple is a typed field at a metadata-relative offset; remaining bytes zero.
METADATA = {
1:[('stationState',72,'I',0),('activeRecipeKey',76,'I',11),('activeTemplateKey',80,'I',12),('cameraCount',84,'I',1),('activation',88,'16s',U(4)),('cameraSetRevision',104,'16s',U(20)),('remoteEnabled',120,'I',1)],
2:[('applicationType',72,'I',1),('requiredCameraCount',76,'I',1),('publicationTemplateKey',80,'I',12)],
3:[('numberCount',72,'I',16),('flagCount',76,'I',32),('textCount',80,'I',4),('poseCount',84,'I',1),('textMaxBytes',88,'I',64),('numberEncoding',92,'I',1),('rounding',96,'I',1),('poseConvention',100,'I',1)],
4:[('backend',72,'I',0),('acquisitionProfileKey',76,'I',9),('intrinsicKey',80,'I',5),('frameKey',84,'I',7),('capabilityBits',88,'I',0)],
5:[('cameraKey',72,'I',4),('width',76,'I',2448),('height',80,'I',2048),('projection',84,'I',1),('distortion',88,'I',1),('calibrationEvidence',92,'I',0)],
6:[('geometry',72,'I',1),('pointCount',76,'I',49),('unit',80,'I',1),('physicalEvidence',84,'I',0)],
7:[('parentFrameKey',72,'I',0),('convention',76,'I',1),('role',80,'I',1)],
8:[('representation',72,'I',1),('actualPoseKind',76,'I',1),('convention',80,'I',1),('flangeTcpFrameKey',84,'I',0),('controllerVerified',88,'I',0)],
9:[('cameraKey',72,'I',4),('pixelFormat',76,'I',1),('triggerMode',80,'I',1),('nativeProductionAllowed',84,'I',0)],
10:[('algorithm',72,'I',0),('modeMask',76,'I',3),('softwareEvidence',80,'I',0)],
11:[('referenceType',72,'I',1),('unit',76,'I',1),('independentEvidence',80,'I',0)],
12:[('translationUnit',72,'I',1),('rotationUnit',76,'I',2),('physicalApproval',80,'I',0)],
13:[('unit',72,'I',3),('monotonic',76,'I',1),('clockEpoch',80,'16s',U(70)),('synchronisedEvidence',96,'I',0)]}
for category,fields in METADATA.items():
    b=frame(103,0); ident(b,128,51); put(b,144,'II',category,category)
    ident(b,152,100+category); ident(b,168,200+category)
    name=f'category{category}'.encode(); put(b,200,'I',len(name)); b[204:204+len(name)]=name
    put(b,268,'I',256); put(b,272,'II',1,256)
    b[280:312]=hashlib.sha256(f'local-content-{category}'.encode()).digest()
    b[312:344]=hashlib.sha256(f'local-dependencies-{category}'.encode()).digest()
    for label,off,fmt,value in fields: put(b,272+off,fmt,value)
    add(f'catalog_category{category:02}',b)

t=cases['terminal']['data']
ack=bytearray(cases['ack']['data']); ack[128:144]=t[128:144]; ack[144:176]=t[160:192]; add('ack_exact',ack)
for name,req in [('select_recipe',4),('set_auto',5),('stop_control',6),('abort_control',25),('sample_holdout',21)]:
    b=frame(111,request=1000+req); ident(b,128,300+req)
    put(b,192,'III',1,0,1); put(b,228,'I',6 if req==21 else 11 if req==25 else 3)
    put(b,892,'I',2 if req in (6,25) else 1); put(b,952,'I',req)
    if req==4: put(b,80,'II',21,22); ident(b,88,304)
    if req==5: put(b,884,'I',1)
    if req==21:
        for off,n in [(232,40),(248,42),(264,46)]: ident(b,off,n)
        put(b,876,'Q',2**53+1); put(b,948,'I',4)
    add(f'terminal_{name}',seal(b))
b=bytearray(cases['calib_sample']['data']); put(b,340,'I',2); add('calib_sample_holdout',b)
for name,q,xyz,inv in [('identity',(1.,0.,0.,0.),(0.,0.,0.),False),('inverse',(.5,-.5,-.5,-.5),(-2.,-3.,-1.),True)]:
    b=bytearray(t); b[616:744]=pose(q=q,xyz=xyz,inverse=inv); add('pose_'+name,seal(b))
b=bytearray(t); put(b,224,'I',0); b[616:744]=bytes(128); add('pose_missing',seal(b))

def mutation(name,base,off,data,outcome):
    b=bytearray(cases[base]['data']); b[off:off+len(data)]=data
    if len(b)==1024 and int.from_bytes(b[8:12],'big')==111: seal(b)
    add(name,b,outcome)
mutation('bad_flags','trigger',19,b'\1','BAD_FRAME')
mutation('bad_kind','trigger',8,struct.pack('>I',999),'BAD_FRAME')
mutation('bad_reserved','trigger',511,b'\1','BAD_FRAME')
mutation('bad_magic','trigger',0,b'40FV','BAD_FRAME')
mutation('bad_size','trigger',12,struct.pack('>I',2**32-1),'BAD_FRAME')
mutation('bad_version','trigger',4,struct.pack('>I',5),'UNSUPPORTED_VERSION')
mutation('stale_session','trigger',32,U(99),'STALE_SESSION')
mutation('stale_activation','trigger',88,U(99),'STALE_CONFIG')
mutation('ack_wrong_hash','ack_exact',144,bytes(32),'ACK_MISMATCH')
mutation('ack_wrong_epoch','ack_exact',64,U(99),'ACK_MISMATCH')
mutation('ack_wrong_station','ack_exact',20,struct.pack('>I',2),'ACK_MISMATCH')
mutation('ack_wrong_result','ack_exact',128,U(99),'ACK_MISMATCH')
mutation('ack_wrong_activation','ack_exact',88,U(99),'ACK_MISMATCH')
mutation('catalog_unknown_schema','catalog_category01',272,struct.pack('>I',2),'INVALID_VALUE')
mutation('catalog_reserved','catalog_category01',911,b'\1','BAD_FRAME')
mutation('bad_text65','terminal',412,struct.pack('>I',65),'INVALID_VALUE')
mutation('bad_utf8','terminal',484,b'\xff','INVALID_VALUE')
mutation('bad_text_padding','terminal',489,b'\1','BAD_FRAME')
mutation('bad_number_mask','terminal',208,struct.pack('>I',65536),'BAD_FRAME')
mutation('bad_pose_revision','calib_sample',280,U(99),'STALE_CONFIG')
mutation('bad_pose_norm','calib_sample',232,struct.pack('>d',2.),'INVALID_VALUE')
mutation('bad_pose_nan','calib_sample',208,struct.pack('>d',float('nan')),'INVALID_VALUE')
mutation('bad_pose_convention','calib_sample',328,struct.pack('>I',2),'INVALID_VALUE')
mutation('bad_sample_role','calib_sample',340,struct.pack('>I',3),'INVALID_VALUE')
mutation('bad_signed_word_order','terminal',280,bytes.fromhex('00008000'),'decoded 32768, not INT_MIN')
mutation('bad_uuid_guid_order','trigger',48,bytes.fromhex('33221100554477468899aabb00000002'),'unknown peer, not canonical UUID')
mutation('terminal_reconnect_session','terminal',32,U(99),'valid')
mutation('trigger_reconnect_session','trigger',32,U(99),'valid')

out=ROOT/'golden'; out.mkdir(exist_ok=True)
manifest={'contractRevision':REV,'requestBytes':512,'responseBytes':1024,'metadata':{},'enums':ENUMS,'cases':[]}
for category,fields in METADATA.items():
    manifest['metadata'][str(category)]=[dict(name=n,offset=o,format=f,value=v.hex() if isinstance(v,bytes) else v) for n,o,f,v in fields]
for name,case in cases.items():
    data=case.pop('data'); (out/(name+'.bin')).write_bytes(data)
    (out/(name+'.hex')).write_text(data.hex()+'\n',encoding='ascii')
    entry=dict(name=name,bytes=len(data),sha256=hashlib.sha256(data).hexdigest(),**case)
    kind=int.from_bytes(data[8:12],'big')
    if kind==111: entry['terminalDigest']=data[160:192].hex()
    if kind<100: entry['requestFingerprint']=fingerprint(data).hex()
    manifest['cases'].append(entry)
(ROOT/'manifest.json').write_text(json.dumps(manifest,indent=2,ensure_ascii=False)+'\n',encoding='utf-8')
print(f'Generated {len(cases)} golden frames; revision {REV}')
