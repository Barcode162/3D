// How different two screenshots are: the % of pixels where any channel
// differs by more than `threshold` (of 255). Different sizes = 100.
import { decodePng } from './png.mjs';

export function diffPercentBuffers(a, b, threshold = 16) {
  const A = decodePng(a), B = decodePng(b);
  if (A.width !== B.width || A.height !== B.height) return 100;
  let diff = 0;
  for (let i = 0; i < A.width * A.height; i++) {
    const k = i * 4;
    if (Math.abs(A.data[k] - B.data[k]) > threshold || Math.abs(A.data[k + 1] - B.data[k + 1]) > threshold ||
        Math.abs(A.data[k + 2] - B.data[k + 2]) > threshold) diff++;
  }
  return (100 * diff) / (A.width * A.height);
}

// Average brightness (0..255) and how much of the picture is near black
// (< 8) or blown out (> 250): quick numbers for "too dark / too bright".
export function exposureStats(buf) {
  const A = decodePng(buf);
  let sum = 0, black = 0, white = 0;
  const n = A.width * A.height;
  for (let i = 0; i < n; i++) {
    const k = i * 4;
    const y = 0.2126 * A.data[k] + 0.7152 * A.data[k + 1] + 0.0722 * A.data[k + 2];
    sum += y;
    if (y < 8) black++;
    if (y > 250) white++;
  }
  return { mean: +(sum / n).toFixed(1), black: +(100 * black / n).toFixed(1), white: +(100 * white / n).toFixed(1) };
}
