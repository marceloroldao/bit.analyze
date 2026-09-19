import pathlib,sys
ROOT=pathlib.Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT))
from reality_slice import Modality,Occurrence,RealitySlice,TemporalAssociator
from synthetic_stream import HIDDEN,generate
def test_overlap():
    assert TemporalAssociator.interval_distance(Occurrence(1,Modality.VISUAL,1,2),Occurrence(2,Modality.AUDIO,1.5,2.5))==0
def test_hidden_pairs():
    e=TemporalAssociator(lambda0=.00001)
    for rs in generate(5000):e.ingest(rs)
    h={p for p,_,_ in HIDDEN}; expected={(a,b) for a in h for b in h if a<b}
    assert {(x.a,x.b) for x in e.strongest(6)}==expected
def test_direction():
    e=TemporalAssociator(lambda0=0,simultaneous_delta=.01)
    for sid in range(1,21):
        t=sid*10.;e.ingest(RealitySlice(sid,t,t+2,(Occurrence(10,Modality.VISUAL,t,t+.05),Occurrence(20,Modality.AUDIO,t+.5,t+.55))))
    f,s,b=e.links[(10,20)].direction_probabilities()
    assert f>.99 and s<.01 and b<.01 and .49<e.links[(10,20)].mean_dt<.51
def test_forgetting_consolidation():
    weak=TemporalAssociator(lambda0=.1);strong=TemporalAssociator(lambda0=.1)
    def rs(sid,t):return RealitySlice(sid,t,t+1,(Occurrence(1,Modality.VISUAL,t,t+.1),Occurrence(2,Modality.AUDIO,t+.2,t+.3)))
    weak.ingest(rs(1,1.))
    for i in range(1,30):strong.ingest(rs(i,float(i)))
    weak._forget(weak.links[(1,2)],100.);strong._forget(strong.links[(1,2)],100.)
    assert strong.links[(1,2)].rho>weak.links[(1,2)].rho
