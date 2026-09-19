from reality_slice import TemporalAssociator
from synthetic_stream import HIDDEN,generate
def main(count=100000):
    engine=TemporalAssociator(lambda0=.00001)
    for rs in generate(count=count):engine.ingest(rs)
    hidden={p for p,_,_ in HIDDEN}; expected={(a,b) for a in hidden for b in hidden if a<b}
    top=engine.strongest(12); got={(x.a,x.b) for x in top[:6]}
    for x in top:
        f,s,b=x.direction_probabilities()
        print(f"{x.a}-{x.b} rho={x.rho:.4f} n={x.repetitions} dt={x.mean_dt:+.4f}s var={x.variance_dt:.6f} dir=({f:.2f},{s:.2f},{b:.2f})")
    hidden_scores=[x.repetitions/(1.0+x.variance_dt) for x in top if (x.a,x.b) in expected]
    distractor_scores=[x.repetitions/(1.0+x.variance_dt) for x in engine.links.values() if (x.a,x.b) not in expected]
    margin=min(hidden_scores)/max(distractor_scores)
    ok=got==expected and margin>=5.0
    print(f"hidden_pair_recovery={len(got&expected)}/{len(expected)} evidence_margin={margin:.2f}x pass={ok}")
    if not ok:raise SystemExit(1)
if __name__=="__main__":main()
