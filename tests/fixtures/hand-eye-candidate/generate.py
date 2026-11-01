import json, math, uuid
from pathlib import Path
root=Path(__file__).resolve().parent
def mul(a,b): return [[sum(a[i][k]*b[k][j] for k in range(4)) for j in range(4)] for i in range(4)]
def inv(a):
 r=[[a[j][i] for j in range(3)] for i in range(3)]
 return [r[i]+[-sum(r[i][k]*a[k][3] for k in range(3))] for i in range(3)]+[[0,0,0,1]]
def pose(axis,angle,t):
 n=math.sqrt(sum(v*v for v in axis));x,y,z=[v/n for v in axis];c=math.cos(angle);s=math.sin(angle);u=1-c
 return [[c+x*x*u,x*y*u-z*s,x*z*u+y*s,t[0]],[y*x*u+z*s,c+y*y*u,y*z*u-x*s,t[1]],[z*x*u-y*s,z*y*u+x*s,c+z*z*u,t[2]],[0,0,0,1]]
def sid(n):return str(uuid.UUID(int=n))
def frame(n):return dict(deviceId='fixture',robotId='',groupId='',frameUuid=sid(n),revision=1)
def spatial(m,source,target):
 # All generated rotations avoid pi; independent matrix to quaternion formula.
 w=math.sqrt(max(0,1+m[0][0]+m[1][1]+m[2][2]))/2
 q=[w,(m[2][1]-m[1][2])/(4*w),(m[0][2]-m[2][0])/(4*w),(m[1][0]-m[0][1])/(4*w)]
 return dict(schemaVersion=1,convention='right-handed-column-vector',source=frame(source),target=frame(target),translationUnit='mm',translation=[m[i][3] for i in range(3)],rotation=dict(format='quaternion-wxyz',value=q))
x=pose([1,2,-1],.47,[57,-83,126]);fixed=pose([-2,1,3],.62,[430,210,690])
for mode in ['eye-in-hand','eye-to-hand']:
 samples=[]
 for i in range(17):
  g=pose([1+(i%3),(-1)**i*(1+i%4),2-i%5],-.7+.11*i,[160+23*i,-120+31*(i%5),300+37*(i%4)])
  c=mul(mul(inv(x),inv(g)),fixed) if mode=='eye-in-hand' else mul(mul(inv(x),g),fixed)
  samples.append(dict(sample=sid(100+i),frame=sid(200+i),holdout=i>=12,baseTFlange=spatial(g,2,1),cameraTPlate=spatial(c,4,3)))
 data=dict(format='comm4-22-golden-v1',generator='stdlib independent 4x4 arithmetic; no OpenCV',mode=mode,groundTruth=spatial(x,3,2 if mode=='eye-in-hand' else 1),fixedTruth=spatial(fixed,4,1 if mode=='eye-in-hand' else 2),samples=samples)
 (root/(mode+'.json')).write_text(json.dumps(data,indent=2)+'\n')
