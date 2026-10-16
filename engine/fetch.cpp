#include "engine/fetch.h"

#include <emscripten/emscripten.h>

#include <algorithm>
#include <cstdlib>
#include <map>

#include "engine/log.h"

namespace engine {
namespace {

using FetchDone = std::function<void(bool, std::vector<uint8_t>)>;
using FetchProgress = std::function<void(float)>;
using DecodeDone = std::function<void(bool, Rgba8Image)>;

struct Download {
  FetchDone done;
  FetchProgress progress;
};

std::map<int, Download>& pendingDownloads() {
  static std::map<int, Download> pending;
  return pending;
}

std::map<int, DecodeDone>& pendingDecodes() {
  static std::map<int, DecodeDone> pending;
  return pending;
}

}  // namespace

// Called from JavaScript as a download comes in (fraction 0..1).
extern "C" EMSCRIPTEN_KEEPALIVE void engine_fetchProgress(int id, float fraction) {
  auto& pending = pendingDownloads();
  const auto it = pending.find(id);
  if (it != pending.end() && it->second.progress) it->second.progress(std::min(fraction, 1.0f));
}

// Called from JavaScript when a download has finished (size < 0 = failed).
extern "C" EMSCRIPTEN_KEEPALIVE void engine_fetchDone(int id, uint8_t* data, int size) {
  auto& pending = pendingDownloads();
  const auto it = pending.find(id);
  if (it == pending.end()) return;
  Download d = std::move(it->second);
  pending.erase(it);
  if (size < 0) {
    d.done(false, {});
    return;
  }
  std::vector<uint8_t> bytes(data, data + size);
  free(data);
  if (d.progress) d.progress(1.0f);
  d.done(true, std::move(bytes));
}

// fetch() with progress. Files that are gzipped (they start with 1f 8b) are
// unpacked with the browser's DecompressionStream; if the server already
// unpacked them (Content-Encoding: gzip), they arrive ready to use.
EM_JS(void, js_fetch, (int id, const char* urlPtr), {
  var url = UTF8ToString(urlPtr);
  fetch(url).then(function(resp) {
    if (!resp.ok) throw new Error('HTTP ' + resp.status);
    var total = +(resp.headers.get('Content-Length') || 0);
    var reader = resp.body.getReader();
    var chunks = [];
    var got = 0;
    function pump() {
      return reader.read().then(function(r) {
        if (r.done) return;
        chunks.push(r.value);
        got += r.value.length;
        if (total > 0) _engine_fetchProgress(id, got / total);
        return pump();
      });
    }
    return pump().then(function() {
      var bytes = new Uint8Array(got);
      var o = 0;
      for (var i = 0; i < chunks.length; i++) { bytes.set(chunks[i], o); o += chunks[i].length; }
      if (bytes.length > 2 && bytes[0] == 0x1f && bytes[1] == 0x8b) {
        var ds = new window['DecompressionStream']('gzip');
        var stream = new Blob([bytes])['stream']()['pipeThrough'](ds);
        return new Response(stream).arrayBuffer().then(function(b) { return new Uint8Array(b); });
      }
      return bytes;
    });
  }).then(function(bytes) {
    var ptr = _malloc(Math.max(bytes.length, 1));
    HEAPU8.set(bytes, ptr);
    _engine_fetchDone(id, ptr, bytes.length);
  }).catch(function(e) {
    err('Download failed: ' + url + ' (' + e + ')');
    _engine_fetchDone(id, 0, -1);
  });
});

// Called from JavaScript when an image has been decoded (pixels = 0 on failure).
extern "C" EMSCRIPTEN_KEEPALIVE void engine_imageDecoded(int id, uint8_t* pixels, int width, int height) {
  auto& pending = pendingDecodes();
  const auto it = pending.find(id);
  if (it == pending.end()) return;
  DecodeDone done = std::move(it->second);
  pending.erase(it);
  Rgba8Image image;
  if (pixels) {
    image.width = width;
    image.height = height;
    image.pixels.assign(pixels, pixels + static_cast<size_t>(width) * height * 4);
    free(pixels);
  }
  done(pixels != nullptr, std::move(image));
}

// createImageBitmap + a 2D canvas give raw RGBA pixels; they're copied into
// WebAssembly memory and handed back to C++.
EM_JS(void, js_decodeImage, (int id, const uint8_t* data, int size, const char* mime), {
  var bytes = HEAPU8.slice(data, data + size);
  var blob = new Blob([bytes], {type: UTF8ToString(mime)});
  createImageBitmap(blob, {colorSpaceConversion: 'none', premultiplyAlpha: 'none'}).then(function(bitmap) {
    var canvas = typeof OffscreenCanvas !== 'undefined' ? new OffscreenCanvas(bitmap.width, bitmap.height)
                                                        : document.createElement('canvas');
    canvas.width = bitmap.width;
    canvas.height = bitmap.height;
    var ctx = canvas.getContext('2d', {colorSpace: 'srgb', willReadFrequently: true});
    ctx.drawImage(bitmap, 0, 0);
    var pixels = ctx.getImageData(0, 0, bitmap.width, bitmap.height).data;
    var ptr = _malloc(pixels.length);
    HEAPU8.set(pixels, ptr);
    _engine_imageDecoded(id, ptr, bitmap.width, bitmap.height);
  }).catch(function(e) {
    err('Image decode failed: ' + e);
    _engine_imageDecoded(id, 0, 0, 0);
  });
});

void fetchFile(const std::string& url, FetchDone done, FetchProgress progress) {
  static int nextId = 1;
  const int id = nextId++;
  pendingDownloads()[id] = {std::move(done), std::move(progress)};
  js_fetch(id, url.c_str());
}

void decodeImage(const std::vector<uint8_t>& bytes, const std::string& mimeType, DecodeDone done) {
  static int nextId = 1;
  const int id = nextId++;
  pendingDecodes()[id] = std::move(done);
  js_decodeImage(id, bytes.data(), static_cast<int>(bytes.size()), mimeType.c_str());
}

}  // namespace engine
