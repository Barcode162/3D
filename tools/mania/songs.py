# The Training pack's songs (J117.8): for each song in tools/mania/song_list.json,
# checks its licence on its own page, downloads it (cached in build/mania-cache/),
# trims it (fading out) to what its map uses, encodes it for the game
# (assets/music/rhythm/pack/<id>.m4a), times it exactly (tools/mania/timing.py:
# the beat grid fitted through the drum hits) and measures what the mapper needs
# (tools/mania/songs/<id>.json): per 1/8 beat, how hard the kick, the snare and
# the hats hit (0-9); per beat, how loud it is and how much it rings on (0-9).
# Usage: python3 tools/mania/songs.py [id ...]   (needs ffmpeg, numpy, curl)
import json, os, subprocess, sys
import numpy as np
sys.path.insert(0, os.path.dirname(__file__))
import timing

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
CACHE = os.path.join(ROOT, 'build', 'mania-cache')
OUT = os.path.join(ROOT, 'assets', 'music', 'rhythm', 'pack')
FEAT = os.path.join(ROOT, 'tools', 'mania', 'songs')
SR = 22050

def sh(*args):
    return subprocess.run(args, capture_output=True, text=True)

def licence_ok(page):
    html = sh('curl', '-sS', '-m', '30', page).stdout
    return 'Creative Commons Attribution 4.0' in html

def fetch(url, path):
    if not os.path.exists(path) or os.path.getsize(path) < 100000:
        sh('curl', '-sS', '-L', '-m', '180', '-o', path, url)
    return os.path.exists(path) and os.path.getsize(path) > 100000

def duration(path):
    r = sh('ffprobe', '-v', 'quiet', '-show_entries', 'format=duration', '-of', 'csv=p=0', path)
    return float(r.stdout.strip() or 0)

def encode(src, dst, seconds):
    d = duration(src)
    args = ['ffmpeg', '-v', 'error', '-y', '-i', src]
    if d > seconds + 10:
        args += ['-t', str(seconds), '-af', f'afade=t=out:st={seconds - 4}:d=4']
    args += ['-ac', '2', '-ar', '44100', '-c:a', 'aac', '-b:a', '112k', '-movflags', '+faststart', dst]
    subprocess.run(args, check=True)

def load(path):
    raw = subprocess.run(['ffmpeg', '-v', 'quiet', '-i', path, '-ac', '1', '-ar', str(SR), '-f', 'f32le', '-'],
                         capture_output=True).stdout
    return np.frombuffer(raw, dtype=np.float32)

def exact_grid(path):
    """The beat grid: tempo snapped to a whole number when it's within 0.01
    BPM of one (songs made in a music program keep an exact tempo), then the
    offset refitted through the onsets at that tempo."""
    r = timing.analyse(path)
    bpm = r['bpm']
    if abs(bpm - round(bpm)) < 0.01:
        bpm = float(round(bpm))
    x = load(path)
    sig, low = timing.onset_curve(x)
    fs = SR / timing.HOP
    thr = np.quantile(sig, 0.85)
    pk = np.where((sig[1:-1] > sig[:-2]) & (sig[1:-1] >= sig[2:]) & (sig[1:-1] > thr))[0] + 1
    a, b, c = sig[pk - 1], sig[pk], sig[pk + 1]
    den = a - 2 * b + c
    off = np.where(den != 0, 0.5 * (a - c) / den, 0)
    times = (pk + off) / fs + (timing.N / 2) / SR - timing.BIAS_MS / 1000
    w = sig[pk]
    q = 60 / bpm / 4
    o = r['offset_ms'] / 1000
    for _ in range(4):
        k = np.round((times - o) / q)
        res = times - (o + k * q)
        m = np.abs(res) < 0.02
        o += float(np.average(res[m], weights=w[m]))
    k = np.round((times - o) / q)
    res = (times - (o + k * q)) * 1000
    m = np.abs(res) < 20
    spread = float(np.sqrt(np.average(res[m] ** 2, weights=w[m])))
    beat = 60 / bpm
    # The beat among the four quarter positions: where the kicks are.
    lw = low[pk]
    kk = k.astype(int)
    j = int(np.argmax([lw[m & (kk % 4 == i)].sum() for i in range(4)]))
    first = (o + j * q) % beat
    return bpm, first * 1000, spread, r

def bands(x):
    N, HOP = 1024, 64
    frames = (len(x) - N) // HOP
    win = np.hanning(N).astype(np.float32)
    edges = [int(f / SR * N) for f in (30, 200, 2000, 9000)]
    out = np.zeros((3, frames), np.float32)
    energy = np.zeros(frames, np.float32)
    prev = None
    for s in range(0, frames, 4096):
        e = min(frames, s + 4096)
        idx = np.arange(N)[None, :] + HOP * np.arange(s, e)[:, None]
        mag = np.abs(np.fft.rfft(x[idx] * win, axis=1))
        S = np.log1p(40 * mag)
        S2 = np.vstack([(prev if prev is not None else S[0])[None, :], S])
        d = np.maximum(0, np.diff(S2, axis=0))
        for i in range(3):
            out[i, s:e] = d[:, edges[i]:edges[i + 1]].sum(1)
        energy[s:e] = (mag[:, edges[1]:edges[3]] ** 2).sum(1)
        prev = S[-1]
    return out, energy, HOP, N

def quantise(values, floor_q=0.6):
    v = np.asarray(values, dtype=np.float64)
    if not len(v):
        return ''
    qs = np.quantile(v, [floor_q, 0.7, 0.78, 0.85, 0.9, 0.94, 0.965, 0.98, 0.992])
    return ''.join(str(int(np.searchsorted(qs, a, side='right'))) for a in v)

def features(path, bpm, first_ms):
    x = load(path)
    on, energy, HOP, N = bands(x)
    fs = SR / HOP
    t0 = (N / 2) / SR + timing.BIAS_MS / 1000
    dur = len(x) / SR
    slot = 60 / bpm / 8
    first = first_ms / 1000
    nslots = int((dur - first) / slot)
    rad = max(1, int(0.015 * fs))
    feats = []
    for band in range(3):
        vals = []
        for s in range(nslots):
            f = int(round((first + s * slot - t0) * fs))
            lo, hi = max(0, f - rad), min(on.shape[1], f + rad + 1)
            vals.append(float(on[band, lo:hi].max()) if hi > lo else 0.0)
        feats.append(quantise(vals))
    # Per beat: loudness (dB from the loudest beat) and ringing (energy kept to the beat's end).
    c = np.concatenate([[0], np.cumsum(x.astype(np.float64) ** 2)])
    beat = 60 / bpm
    nbeats = nslots // 8
    rms, ring = [], []
    for b in range(nbeats):
        i0 = int((first + b * beat) * SR)
        i1 = min(len(x), int((first + (b + 1) * beat) * SR))
        rms.append(10 * np.log10((c[i1] - c[i0]) / max(1, i1 - i0) + 1e-12))
        f0 = int((first + b * beat - t0) * fs)
        f1 = int((first + (b + 0.75) * beat - t0) * fs)
        e0 = energy[max(0, f0):max(1, f0 + 3)].mean() if f0 < len(energy) else 0
        e1 = energy[max(0, f1):max(1, f1 + 3)].mean() if f1 < len(energy) else 0
        ring.append(e1 / (e0 + 1e-9))
    top = max(rms) if rms else 0
    rms_d = ''.join(str(int(np.clip(9 + (r - top) / 3.0, 0, 9))) for r in rms)
    ring_d = ''.join(str(int(np.clip(r * 10, 0, 9))) for r in ring)
    return feats, rms_d, ring_d, dur

def main():
    lst = json.load(open(os.path.join(ROOT, 'tools', 'mania', 'song_list.json')))
    only = set(sys.argv[1:])
    os.makedirs(CACHE, exist_ok=True)
    os.makedirs(OUT, exist_ok=True)
    os.makedirs(FEAT, exist_ok=True)
    for s in lst['songs']:
        if only and s['id'] not in only:
            continue
        if not licence_ok(s['page']):
            print(f"FAIL {s['id']}: its page doesn't say Creative Commons Attribution 4.0 — not used")
            continue
        src = os.path.join(CACHE, s['download'].split('file=')[-1])
        if not fetch(s['download'], src):
            print(f"FAIL {s['id']}: download failed")
            continue
        dst = os.path.join(OUT, s['id'] + '.m4a')
        encode(src, dst, s['seconds'])
        bpm, first_ms, spread, r = exact_grid(dst)
        (low, mid, high), rms, ring, dur = features(dst, bpm, first_ms)
        json.dump({'id': s['id'], 'title': s['title'], 'artist': s['artist'], 'licence': s['licence'], 'page': s['page'],
                   'file': f"assets/music/rhythm/pack/{s['id']}.m4a", 'bpm': bpm, 'offset_ms': round(first_ms, 2),
                   'length': round(dur, 2), 'spread_ms': round(spread, 2), 'on_grid': r['on_grid'], 'drift_ms': r['drift_ms'],
                   'low': low, 'mid': mid, 'high': high, 'rms': rms, 'sus': ring},
                  open(os.path.join(FEAT, s['id'] + '.json'), 'w'))
        print(f"ok   {s['id']:28s} {bpm:8.3f} BPM  first beat {first_ms:6.1f} ms  onsets ±{spread:.1f} ms  "
              f"drift {r['drift_ms']:.1f} ms  {dur:.0f} s  {os.path.getsize(dst) // 1024} KB")

if __name__ == '__main__':
    main()
