from __future__ import annotations
from dataclasses import dataclass
from enum import IntEnum
from math import exp, log, sqrt
from typing import Dict, Hashable, List, Tuple

class Modality(IntEnum):
    VISUAL=1; AUDIO=2; TOUCH=3; TEXT=4; SENSOR=5

@dataclass(frozen=True)
class Occurrence:
    """One opaque pattern occurrence in a time interval.

    `modality` is retained as the historical field name, but it is not a
    closed taxonomy. Any hashable stream/channel identity is accepted. The
    association dynamics do not branch on this value.
    """
    pattern:int; modality:Hashable; t_start:float; t_end:float; source:int=0; provenance:int=0
    def __post_init__(self):
        hash(self.modality)
    @property
    def center(self): return (self.t_start+self.t_end)*0.5
    @property
    def stream_id(self): return self.modality

@dataclass(frozen=True)
class RealitySlice:
    slice_id:int; t_start:float; t_end:float; occurrences:Tuple[Occurrence,...]; provenance:Tuple[int,...]=()
    def __post_init__(self):
        if self.t_end < self.t_start: raise ValueError("invalid RealitySlice window")
        if any(x.t_start < self.t_start or x.t_end > self.t_end for x in self.occurrences):
            raise ValueError("occurrence outside RealitySlice")

@dataclass
class Association:
    a:int; b:int; rho:float=0.; forward:float=0.; simultaneous:float=0.; backward:float=0.
    repetitions:int=0; mean_dt:float=0.; m2_dt:float=0.; last_time:float=0.
    @property
    def variance_dt(self): return self.m2_dt/(self.repetitions-1) if self.repetitions>1 else 0.
    def direction_probabilities(self):
        total=self.forward+self.simultaneous+self.backward
        return (0.,0.,0.) if total==0 else (self.forward/total,self.simultaneous/total,self.backward/total)

class TemporalAssociator:
    def __init__(self,eta=.18,lambda0=.015,consolidation=1.,simultaneous_delta=.12,min_proximity=.03,default_tau=1.5):
        self.eta=eta; self.lambda0=lambda0; self.consolidation=consolidation
        self.simultaneous_delta=simultaneous_delta; self.min_proximity=min_proximity; self.default_tau=default_tau
        self.links:Dict[Tuple[int,int],Association]={}
        self.total_slices=0; self.pattern_slices:Dict[int,int]={}
    @staticmethod
    def _key(a,b): return (a,b) if a<b else (b,a)
    @staticmethod
    def interval_distance(a,b):
        if a.t_end>=b.t_start and b.t_end>=a.t_start: return 0.
        return min(abs(b.t_start-a.t_end),abs(a.t_start-b.t_end))
    def _forget(self,link,now):
        if link.last_time<=0 or now<=link.last_time:return
        lam=self.lambda0/(1.+self.consolidation*log(1.+link.repetitions))
        decay=exp(-lam*(now-link.last_time))
        link.rho*=decay; link.forward*=decay; link.simultaneous*=decay; link.backward*=decay
    def ingest(self,rs):
        self.total_slices+=1
        for pattern in {x.pattern for x in rs.occurrences}:
            self.pattern_slices[pattern]=self.pattern_slices.get(pattern,0)+1
        items=sorted(rs.occurrences,key=lambda x:(x.center,x.pattern))
        seen_pairs=set()
        for i,a in enumerate(items):
            for b in items[i+1:]:
                if a.pattern==b.pattern:continue
                proximity=exp(-self.interval_distance(a,b)/self.default_tau)
                if proximity<self.min_proximity:continue
                key=self._key(a.pattern,b.pattern); link=self.links.setdefault(key,Association(*key))
                self._forget(link,rs.t_end)
                if key in seen_pairs:continue
                seen_pairs.add(key)
                oa=a if a.pattern==key[0] else b; ob=b if b.pattern==key[1] else a; dt=ob.center-oa.center
                link.rho+=self.eta*proximity*(1.-link.rho); w=self.eta*proximity
                if abs(dt)<=self.simultaneous_delta:link.simultaneous+=w
                elif dt>0:link.forward+=w
                else:link.backward+=w
                link.repetitions+=1; delta=dt-link.mean_dt; link.mean_dt+=delta/link.repetitions
                link.m2_dt+=delta*(dt-link.mean_dt); link.last_time=rs.t_end
    def selectivity(self,link):
        if self.total_slices<=0:return 0.
        pa=self.pattern_slices.get(link.a,0)/self.total_slices; pb=self.pattern_slices.get(link.b,0)/self.total_slices
        pab=link.repetitions/self.total_slices
        return pab/(pa*pb) if pa>0 and pb>0 else 0.
    def directional_coverage(self,link):
        """Fraction of antecedent-bearing slices that also contain this pair.

        The antecedent is inferred from the dominant observed temporal direction.
        Simultaneous relations use the more frequent pattern as the conservative
        denominator. This is structural coverage, not causal confidence.
        """
        fwd,sim,back=link.direction_probabilities()
        direction=max((("forward",fwd),("simultaneous",sim),("backward",back)),key=lambda x:(x[1],x[0]))[0]
        if direction=="forward": denominator=self.pattern_slices.get(link.a,0)
        elif direction=="backward": denominator=self.pattern_slices.get(link.b,0)
        else: denominator=max(self.pattern_slices.get(link.a,0),self.pattern_slices.get(link.b,0))
        return min(1.,link.repetitions/denominator) if denominator>0 else 0.
    def directional_reliability(self,link):
        fwd,sim,back=link.direction_probabilities()
        return self.directional_coverage(link)*max(fwd,sim,back)
    def temporal_stability(self,link,kappa=.20):
        return exp(-sqrt(max(0.,link.variance_dt))/kappa)
    def evidence_score(self,link):
        return link.repetitions*self.selectivity(link)*self.temporal_stability(link)
    def strongest(self,limit=20):
        return sorted(self.links.values(),key=lambda x:(x.rho,x.repetitions),reverse=True)[:limit]
