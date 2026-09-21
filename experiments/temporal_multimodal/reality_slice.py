from __future__ import annotations
from dataclasses import dataclass, field
from enum import IntEnum
from math import exp, log, sqrt
from itertools import combinations
from typing import Dict, List, Tuple

class Modality(IntEnum):
    VISUAL=1; AUDIO=2; TOUCH=3; TEXT=4; SENSOR=5

@dataclass(frozen=True)
class Occurrence:
    pattern:int; modality:Modality; t_start:float; t_end:float; source:int=0; provenance:int=0
    @property
    def center(self): return (self.t_start+self.t_end)*0.5

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
    repetitions:int=0; mean_dt:float=0.; m2_dt:float=0.; last_time:float=0.; seen_slices:set[int]=field(default_factory=set)
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
        for i,a in enumerate(items):
            for b in items[i+1:]:
                if a.pattern==b.pattern:continue
                proximity=exp(-self.interval_distance(a,b)/self.default_tau)
                if proximity<self.min_proximity:continue
                key=self._key(a.pattern,b.pattern); link=self.links.setdefault(key,Association(*key))
                self._forget(link,rs.t_end)
                if rs.slice_id in link.seen_slices:continue
                oa=a if a.pattern==key[0] else b; ob=b if b.pattern==key[1] else a; dt=ob.center-oa.center
                link.rho+=self.eta*proximity*(1.-link.rho); w=self.eta*proximity
                if abs(dt)<=self.simultaneous_delta:link.simultaneous+=w
                elif dt>0:link.forward+=w
                else:link.backward+=w
                link.repetitions+=1; delta=dt-link.mean_dt; link.mean_dt+=delta/link.repetitions
                link.m2_dt+=delta*(dt-link.mean_dt); link.last_time=rs.t_end; link.seen_slices.add(rs.slice_id)
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


@dataclass
class ContextAssociation:
    """Sparse presemantic two-pattern context followed by one consequence pattern."""
    antecedents:Tuple[int,int]
    consequence:int
    rho:float=0.
    repetitions:int=0
    mean_delay:float=0.
    m2_delay:float=0.
    last_time:float=0.
    last_decay_time:float=0.
    seen_slices:set[int]=field(default_factory=set)

    @property
    def variance_delay(self):
        return self.m2_delay/(self.repetitions-1) if self.repetitions>1 else 0.


class SparseContextAssociator:
    """Learn only observed tight two-pattern contexts that precede a consequence.

    This does not create synthetic pattern IDs. The antecedent context is retained as
    an unordered tuple of two opaque pattern IDs. Admission can additionally require
    that both lower-order antecedent->consequence relations remain insufficient.
    """

    def __init__(
        self,
        eta=.18,
        lambda0=.015,
        consolidation=1.,
        simultaneous_delta=.12,
        context_span=.15,
        max_consequence_delay=1.5,
        min_pattern_support=1,
    ):
        if context_span < 0 or max_consequence_delay <= 0:
            raise ValueError("invalid sparse context timing")
        if min_pattern_support < 1:
            raise ValueError("min_pattern_support must be >= 1")
        self.eta=eta
        self.lambda0=lambda0
        self.consolidation=consolidation
        self.simultaneous_delta=simultaneous_delta
        self.context_span=context_span
        self.max_consequence_delay=max_consequence_delay
        self.min_pattern_support=int(min_pattern_support)
        self.links:Dict[Tuple[int,int,int],ContextAssociation]={}
        self.context_slices:Dict[Tuple[int,int],int]={}
        self.total_slices=0

    @staticmethod
    def _context_key(a,b):
        if a==b: raise ValueError("context antecedents must be distinct")
        return (a,b) if a<b else (b,a)

    def _forget(self,link,now):
        reference=max(link.last_time,link.last_decay_time)
        if reference<=0 or now<=reference:return
        lam=self.lambda0/(1.+self.consolidation*log(1.+link.repetitions))
        link.rho*=exp(-lam*(now-reference))
        link.last_decay_time=now

    def advance_time(self,now):
        """Decay all known higher-order links without fabricating observations."""
        now=float(now)
        for link in self.links.values():
            self._forget(link,now)

    def ingest(self,rs,pattern_support=None):
        self.advance_time(rs.t_end)
        self.total_slices+=1
        items=sorted(rs.occurrences,key=lambda x:(x.center,x.pattern))
        if self.min_pattern_support>1:
            if pattern_support is None:
                raise ValueError(
                    "pattern_support is required when min_pattern_support > 1"
                )
            items=[
                item for item in items
                if int(pattern_support.get(item.pattern,0))>=self.min_pattern_support
            ]
        seen_contexts=set()
        seen_triples=set()

        for left,right in combinations(items,2):
            if left.pattern==right.pattern:continue
            if abs(right.center-left.center)>self.context_span:continue
            context=self._context_key(left.pattern,right.pattern)
            if context not in seen_contexts:
                self.context_slices[context]=self.context_slices.get(context,0)+1
                seen_contexts.add(context)

            boundary=max(left.center,right.center)
            for consequence in items:
                if consequence.pattern in context:continue
                delay=consequence.center-boundary
                if delay<=self.simultaneous_delta:continue
                if delay>self.max_consequence_delay:continue
                triple=(context[0],context[1],consequence.pattern)
                if triple in seen_triples:continue
                link=self.links.setdefault(
                    triple,
                    ContextAssociation(context,consequence.pattern),
                )
                self._forget(link,rs.t_end)
                if rs.slice_id in link.seen_slices:continue
                proximity=exp(-delay/self.max_consequence_delay)
                link.rho+=self.eta*proximity*(1.-link.rho)
                link.repetitions+=1
                delta=delay-link.mean_delay
                link.mean_delay+=delta/link.repetitions
                link.m2_delay+=delta*(delay-link.mean_delay)
                link.last_time=rs.t_end
                link.last_decay_time=rs.t_end
                link.seen_slices.add(rs.slice_id)
                seen_triples.add(triple)

    def context_coverage(self,link):
        denominator=self.context_slices.get(link.antecedents,0)
        return min(1.,link.repetitions/denominator) if denominator>0 else 0.

    def temporal_stability(self,link,kappa=.20):
        return exp(-sqrt(max(0.,link.variance_delay))/kappa)

    def context_reliability(self,link):
        return self.context_coverage(link)*self.temporal_stability(link)

    @staticmethod
    def _lower_order_reliability(pairwise,antecedent,consequence):
        key=pairwise._key(antecedent,consequence)
        link=pairwise.links.get(key)
        if link is None:return 0.
        fwd,sim,back=link.direction_probabilities()
        if antecedent==key[0]:
            direction_confidence=fwd
        else:
            direction_confidence=back
        return pairwise.directional_coverage(link)*direction_confidence

    def lower_order_reliabilities(self,link,pairwise):
        return tuple(
            self._lower_order_reliability(
                pairwise,
                antecedent,
                link.consequence,
            )
            for antecedent in link.antecedents
        )

    def admitted_contexts(
        self,
        pairwise,
        *,
        min_repetitions=3,
        min_independent_slices=3,
        min_rho=.39,
        min_context_reliability=.75,
        max_lower_order_reliability=.75,
    ):
        """Return sparse contexts supported by repetition and needed beyond pairwise links."""
        admitted=[]
        for key,link in sorted(self.links.items()):
            lower=self.lower_order_reliabilities(link,pairwise)
            if link.repetitions<min_repetitions:continue
            if len(link.seen_slices)<min_independent_slices:continue
            if link.rho<min_rho:continue
            if self.context_reliability(link)<min_context_reliability:continue
            if any(value>=max_lower_order_reliability for value in lower):continue
            admitted.append(link)
        return tuple(admitted)
