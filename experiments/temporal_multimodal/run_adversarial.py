import pathlib,random,sys
ROOT=pathlib.Path(__file__).resolve().parent;sys.path.insert(0,str(ROOT))
from reality_slice import Modality,Occurrence,RealitySlice,TemporalAssociator
A=((101,Modality.VISUAL,0.),(202,Modality.AUDIO,.24),(303,Modality.TOUCH,.43),(404,Modality.TEXT,.12))
B=((202,Modality.AUDIO,0.),(505,Modality.SENSOR,.31),(606,Modality.VISUAL,.17))
BACKGROUND=900;CORRELATED=(1201,1202)
def generate(count=100000,seed=2718):
 rng=random.Random(seed);t=1.
 for sid in range(1,count+1):
  items=[]
  if rng.random()<.94:
   s=t+rng.uniform(0,1.25);items.append(Occurrence(BACKGROUND,Modality.SENSOR,s,s+.05,5,sid))
  if rng.random()<.30:
   reverse=rng.random()<.12
   for p,m,o in A:
    if rng.random()<.70:
     off=.55-o if reverse else o;s=t+off+rng.gauss(0,.045);items.append(Occurrence(p,m,s,s+.07,int(m),sid))
  if rng.random()<.25:
   for p,m,o in B:
    if rng.random()<.72:
     s=t+.65+o+rng.gauss(0,.05);items.append(Occurrence(p,m,s,s+.07,int(m),sid))
  if rng.random()<.34:
   base=t+rng.uniform(.05,.45);items.append(Occurrence(1201,Modality.VISUAL,base,base+.06,1,sid))
   s=base+rng.uniform(-.55,.75);items.append(Occurrence(1202,Modality.AUDIO,s,s+.06,2,sid))
  for _ in range(rng.randint(2,7)):
   p=rng.randint(2000,2199);m=rng.choice(list(Modality));s=t+rng.uniform(-.1,1.35);items.append(Occurrence(p,m,s,s+rng.uniform(.02,.1),int(m),sid))
  yield RealitySlice(sid,min(x.t_start for x in items)-.01,max(x.t_end for x in items)+.01,tuple(items),(sid,))
  t+=rng.uniform(1.8,3.)
def pairs(g):
 ids={p for p,_,_ in g};return {(a,b) for a in ids for b in ids if a<b}
def score(x):return x.repetitions/(1.+x.variance_dt)
def main(count=100000):
 e=TemporalAssociator(lambda0=.00001)
 for rs in generate(count):e.ingest(rs)
 expected=pairs(A)|pairs(B);ranked=sorted(e.links.values(),key=score,reverse=True)
 true=[x for x in e.links.values() if (x.a,x.b) in expected];false=[x for x in e.links.values() if (x.a,x.b) not in expected]
 margin=min(map(score,true))/max(map(score,false));top=sum((x.a,x.b) in expected for x in ranked[:len(expected)])
 corr=e.links[CORRELATED]
 print(f"expected={len(expected)} top_expected={top}/{len(expected)} margin={margin:.3f}x")
 print(f"correlated_distractor score={score(corr):.2f} rho={corr.rho:.4f} var={corr.variance_dt:.6f}")
 for x in ranked[:20]:print(f"{x.a}-{x.b} score={score(x):.2f} rho={x.rho:.4f} n={x.repetitions} var={x.variance_dt:.6f}")
 ok=top==len(expected) and margin>=2.;print(f"adversarial_pass={ok}")
 if not ok:raise SystemExit(1)
if __name__=="__main__":main()
