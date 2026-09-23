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
def score(engine,x):return engine.evidence_score(x)
def structural_score(engine,link):
 base=score(engine,link)
 neighbors_a={x.b if x.a==link.a else x.a for x in engine.links.values() if link.a in (x.a,x.b) and x is not link}
 neighbors_b={x.b if x.a==link.b else x.a for x in engine.links.values() if link.b in (x.a,x.b) and x is not link}
 shared=neighbors_a & neighbors_b
 if not shared:return base
 supports=[]
 for n in shared:
  ka=engine._key(link.a,n); kb=engine._key(link.b,n)
  if ka in engine.links and kb in engine.links:
   supports.append(min(score(engine,engine.links[ka]),score(engine,engine.links[kb])))
 if not supports:return base
 supports.sort(reverse=True)
 closure=sum(supports[:3])/len(supports[:3])
 return base*closure
def overlapping_community_score(engine,link):
 base=structural_score(engine,link)
 shared=[]
 nodes={p for pair in engine.links for p in pair}
 for n in nodes-{link.a,link.b}:
  ka=engine._key(link.a,n); kb=engine._key(link.b,n)
  if ka not in engine.links or kb not in engine.links:continue
  triad=min(score(engine,link),score(engine,engine.links[ka]),score(engine,engine.links[kb]))
  if triad>0:shared.append(triad)
 if not shared:return base
 shared.sort(reverse=True)
 support=sum(shared[:2])/len(shared[:2])
 return base*(1.+support/(score(engine,link)+support))
def main(count=100000):
 e=TemporalAssociator(lambda0=.00001)
 for rs in generate(count):e.ingest(rs)
 expected=pairs(A)|pairs(B);ranked=sorted(e.links.values(),key=lambda x:overlapping_community_score(e,x),reverse=True)
 true=[x for x in e.links.values() if (x.a,x.b) in expected];false=[x for x in e.links.values() if (x.a,x.b) not in expected]
 margin=min(overlapping_community_score(e,x) for x in true)/max(overlapping_community_score(e,x) for x in false);top=sum((x.a,x.b) in expected for x in ranked[:len(expected)])
 corr=e.links[CORRELATED]
 print(f"expected={len(expected)} top_expected={top}/{len(expected)} margin={margin:.3f}x")
 print(f"correlated_distractor score={score(e,corr):.2f} structural={structural_score(e,corr):.2f} community={overlapping_community_score(e,corr):.2f} rho={corr.rho:.4f} var={corr.variance_dt:.6f} stability={e.temporal_stability(corr):.4f}")
 for x in ranked[:20]:print(f"{x.a}-{x.b} score={score(e,x):.2f} structural={structural_score(e,x):.2f} community={overlapping_community_score(e,x):.2f} rho={x.rho:.4f} n={x.repetitions} var={x.variance_dt:.6f} stability={e.temporal_stability(x):.4f}")
 ok=top==len(expected) and margin>=2.;print(f"adversarial_pass={ok}")
 if not ok:raise SystemExit(1)
if __name__=="__main__":main()
