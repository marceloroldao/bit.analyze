import random
from reality_slice import Modality,Occurrence,RealitySlice
HIDDEN=((101,Modality.VISUAL,0.),(202,Modality.AUDIO,.24),(303,Modality.TOUCH,.43),(404,Modality.TEXT,.12))
def generate(count=100000,seed=1701):
    rng=random.Random(seed); t=1.; noise=list(range(1000,1100))
    for sid in range(1,count+1):
        items=[]
        if rng.random()<.36:
            for pattern,modality,offset in HIDDEN:
                if rng.random()<.88:
                    start=t+offset+rng.gauss(0.,.035)
                    items.append(Occurrence(pattern,modality,start,start+.08,int(modality),sid))
        for _ in range(rng.randint(1,5)):
            pattern=rng.choice(noise); modality=rng.choice(list(Modality)); start=t+rng.uniform(0.,1.2)
            items.append(Occurrence(pattern,modality,start,start+rng.uniform(.02,.12),int(modality),sid))
        yield RealitySlice(sid,t-.1,t+1.5,tuple(items),(sid,))
        t+=rng.uniform(1.7,3.2)
