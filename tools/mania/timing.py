# Exact beat grid for a song: tempo from the beat-frequency comb, then a
# straight-line fit of the grid through the song's onsets (kicks, snares,
# hats), iterated; reports tempo, first-beat offset, how tightly the onsets
# sit on the grid, and drift between the song's first and last thirds.
import sys, subprocess, json, numpy as np
SR = 22050; HOP = 32; N = 512
BIAS_MS = -8.5  # measured on a synthetic kick track: onsets read 8.5 ms early
def load(p):
    raw = subprocess.run(['ffmpeg','-v','quiet','-i',p,'-ac','1','-ar',str(SR),'-f','f32le','-'],capture_output=True).stdout
    return np.frombuffer(raw, dtype=np.float32)
def onset_curve(x):
    frames = (len(x) - N) // HOP
    win = np.hanning(N).astype(np.float32)
    f = np.zeros(frames, np.float32); L = np.zeros(frames, np.float32)
    prev = None
    for s in range(0, frames, 8192):
        e = min(frames, s + 8192)
        idx = np.arange(N)[None, :] + HOP * np.arange(s, e)[:, None]
        S = np.log1p(50 * np.abs(np.fft.rfft(x[idx] * win, axis=1)))
        S2 = np.vstack([(prev if prev is not None else S[0])[None, :], S])
        d = np.maximum(0, np.diff(S2, axis=0))
        lo = d[:, :int(200 / SR * N) + 1].sum(1) * 3
        f[s:e] = d.sum(1) + lo
        L[s:e] = lo
        prev = S[-1]
    k = int(0.25 * SR / HOP)
    c = np.concatenate([[0], np.cumsum(f, dtype=np.float64)]); i = np.arange(len(f))
    lo_ = np.maximum(0, i - k); hi = np.minimum(len(f), i + k + 1)
    out = np.maximum(0, f - 1.2 * (c[hi] - c[lo_]) / (hi - lo_))
    c2 = np.concatenate([[0], np.cumsum(L, dtype=np.float64)])
    low = np.maximum(0, L - 1.2 * (c2[hi] - c2[lo_]) / (hi - lo_))
    return out, low
def comb(sig, bpm, fs):
    t = np.arange(len(sig)); tot = 0
    for h, w in ((1, 1.0), (2, 0.7), (4, 0.5)):
        tot += w * np.abs((sig * np.exp(-2j * np.pi * t * bpm * h / 60 / fs)).sum())
    return tot / sig.sum()
def analyse(p, lo_bpm=95, hi_bpm=235):
    x = load(p); fs = SR / HOP; dur = len(x) / SR
    sig, low = onset_curve(x)
    # frame time = frame start + window centre
    t0 = (N / 2) / SR
    dec = sig[::4]; fsd = fs / 4
    scores = [(comb(dec, b, fsd), b) for b in np.arange(lo_bpm, hi_bpm, 0.25)]
    scores.sort(reverse=True)
    cands = []
    for sc, b in scores[:6]:
        fine = max((comb(dec, bb, fsd), bb) for bb in np.arange(b - 0.3, b + 0.3, 0.01))
        cands.append(fine)
    bpm0 = max(cands)[1]
    # onsets: local maxima above the 85th percentile
    thr = np.quantile(sig, 0.85)
    pk = np.where((sig[1:-1] > sig[:-2]) & (sig[1:-1] >= sig[2:]) & (sig[1:-1] > thr))[0] + 1
    # parabolic refinement
    a, b_, c_ = sig[pk - 1], sig[pk], sig[pk + 1]
    den = a - 2 * b_ + c_; off = np.where(den != 0, 0.5 * (a - c_) / den, 0)
    times = (pk + off) / fs + t0 - BIAS_MS / 1000
    wts = sig[pk]
    # initial phase from the fundamental of the strongest onsets
    period = 60 / bpm0; q = period / 4
    ph = np.angle((wts * np.exp(-2j * np.pi * times / period)).sum())
    offset = (-ph / (2 * np.pi) * period) % period
    for it in range(6):
        k = np.round((times - offset) / q)
        res = times - (offset + k * q)
        tol = 0.030 if it < 2 else 0.020
        m = np.abs(res) < tol
        if m.sum() < 20: break
        A = np.vstack([np.ones(m.sum()), k[m]]).T
        sol, *_ = np.linalg.lstsq(A * wts[m, None], times[m] * wts[m], rcond=None)
        offset, q = sol[0], sol[1]
    k = np.round((times - offset) / q); res = (times - (offset + k * q)) * 1000
    m = np.abs(res) < 20
    on_grid = float((wts[m]).sum() / wts.sum())
    spread = float(np.sqrt(np.average(res[m] ** 2, weights=wts[m])))
    # drift: mean residual in the first and last thirds of the song
    first = m & (times < dur / 3); last = m & (times > 2 * dur / 3)
    drift = float(np.average(res[last], weights=wts[last]) - np.average(res[first], weights=wts[first])) if first.sum() > 5 and last.sum() > 5 else 99
    bpm = 60 / (4 * q)
    beat = 4 * q
    # which of the four quarter positions is the beat: the strongest onsets
    kk = np.round((times - offset) / q).astype(int)
    lw = low[pk]  # the kick drum marks the beat
    strength = [float(lw[m & (kk % 4 == j)].sum()) for j in range(4)]
    j = int(np.argmax(strength))
    first_beat = (offset + j * q) % beat
    # loudness over time (per bar), for the mapper
    return dict(file=p.split('/')[-1], dur=round(dur, 2), bpm=round(bpm, 4), offset_ms=round(first_beat * 1000, 2),
                on_grid=round(on_grid, 3), spread_ms=round(spread, 2), drift_ms=round(drift, 2), bpm_coarse=round(bpm0, 2))
if __name__ == '__main__':
    out = [analyse(p) for p in sys.argv[1:]]
    for r in out: print(json.dumps(r))
