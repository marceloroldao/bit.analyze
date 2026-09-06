#!/usr/bin/env python3
from pathlib import Path
import argparse, json, math, random, struct, wave

SEED = 0xB17A4A

parser = argparse.ArgumentParser()
parser.add_argument("--output", type=Path, default=Path(__file__).resolve().parent / "generated")
parser.add_argument("--seed", type=lambda x: int(x, 0), default=SEED)
args = parser.parse_args()

ROOT = args.output.resolve()
ROOT.mkdir(parents=True, exist_ok=True)
rng = random.Random(args.seed)


def write_text(path: Path, i: int):
    lines = []
    for n in range(1200):
        lines.append(f"sample={i};row={n};alpha={(n*17+i)%97};beta={(n*n+i)%251}; relation relation memory\n")
    path.write_text("".join(lines), encoding="utf-8")


def write_json(path: Path, i: int):
    data = [{"id": n, "group": n % 17, "value": (n * 31 + i) % 1009,
             "tags": ["memory", "relation", f"g{n%7}"]} for n in range(1800)]
    path.write_text(json.dumps(data, separators=(",", ":")), encoding="utf-8")


def write_csv(path: Path, i: int):
    rows = ["id,group,value,x,y\n"]
    for n in range(5000):
        rows.append(f"{n},{n%19},{(n*43+i)%4093},{math.sin(n/13):.6f},{math.cos(n/17):.6f}\n")
    path.write_text("".join(rows), encoding="utf-8")


def write_wav(path: Path, i: int):
    sr = 8000
    with wave.open(str(path), "wb") as w:
        w.setnchannels(1); w.setsampwidth(2); w.setframerate(sr)
        frames = bytearray()
        f1, f2 = 220 + i*13, 440 + i*7
        for n in range(sr * 2):
            x = 0.55*math.sin(2*math.pi*f1*n/sr) + 0.25*math.sin(2*math.pi*f2*n/sr)
            frames += struct.pack("<h", int(max(-1,min(1,x))*32767))
        w.writeframes(frames)


def write_pgm(path: Path, i: int):
    w = h = 256
    body = bytearray()
    for y in range(h):
        for x in range(w):
            body.append((x*3 + y*5 + ((x//16)^(y//16))*37 + i*11) & 255)
    path.write_bytes(f"P5\n{w} {h}\n255\n".encode() + body)


def write_structured_bin(path: Path, i: int):
    motifs = [b"ABABCDCD0000", b"XYZXYZ123123", bytes(range(32)), b"\x00"*64 + b"\xff"*64]
    out = bytearray()
    local = random.Random(args.seed + 100 + i)
    for n in range(4096):
        m = motifs[(n+i) % len(motifs)]
        out += m
        if n % 31 == 0:
            out += bytes(local.randrange(256) for _ in range(7))
    path.write_bytes(out)


def write_random_bin(path: Path, i: int):
    local = random.Random(args.seed + 1000 + i)
    path.write_bytes(bytes(local.randrange(256) for _ in range(128*1024)))

writers = [
    ("txt", write_text), ("json", write_json), ("csv", write_csv),
    ("wav", write_wav), ("pgm", write_pgm),
    ("structured.bin", write_structured_bin), ("random.bin", write_random_bin),
]

manifest = []
for family, writer in writers:
    for i in range(4):
        suffix = family if not family.endswith(".bin") else family
        name = f"{family.replace('.', '_')}_{i}.{suffix}" if "." not in family else f"{family.replace('.', '_')}_{i}.bin"
        p = ROOT / name
        writer(p, i)
        manifest.append({"family": family, "file": p.name, "bytes": p.stat().st_size})

metadata = {"seed": args.seed, "files": manifest}
(ROOT / "manifest.json").write_text(json.dumps(metadata, indent=2), encoding="utf-8")
print(f"output={ROOT}")
print(f"seed={args.seed}")
print(f"files={len(manifest)} bytes={sum(x['bytes'] for x in manifest)}")
