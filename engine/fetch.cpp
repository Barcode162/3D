#include "engine/fetch.h"

#include <emscripten/emscripten.h>

#include <cstdlib>
#include <map>

#include "engine/log.h"

namespace engine {
namespace {

using FetchDone = std::function<void(bool, std::vector<uint8_t>)>;
using FetchProgress = std::function<void(float)>;

struct Download {
  FetchDone done;
  FetchProgress progress;
};
using DecodeDone = std::function<void(bool, Rgba8Image)>;

std::map<int, DecodeDone>& pendingDecodes() {
  static std::map<int, DecodeDone> pending;
  return pending;
}

void onFetchLoad(unsigned, void* arg, void* data, unsigned size) {
  auto* d = static_cast<Download*>(arg);
  const auto* bytes = static_cast<const uint8_t*>(data);
  if (d->progress) d->progress(1.0f);
  d->done(true, std::vector<uint8_t>(bytes, bytes + size));
  delete d;
}

void onFetchError(unsigned, void* arg, int status, const char*) {
  auto* d = static_cast<Download*>(arg);
  LOG_ERROR("Download failed (HTTP %d).", status);
  d->done(false, {});
  delete d;
}

void onFetchProgress(unsigned, void* arg, int loaded, int total) {
  auto* d = static_cast<Download*>(arg);
  if (d->progress && total > 0) d->progress(static_cast<float>(loaded) / static_cast<float>(total));
}

}  // namespace

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
  auto* d = new Download{std::move(done), std::move(progress)};
  emscripten_async_wget2_data(url.c_str(), "GET", "", d, true, onFetchLoad, onFetchError, onFetchProgress);
}

void decodeImage(const std::vector<uint8_t>& bytes, const std::string& mimeType, DecodeDone done) {
  static int nextId = 1;
  const int id = nextId++;
  pendingDecodes()[id] = std::move(done);
  js_decodeImage(id, bytes.data(), static_cast<int>(bytes.size()), mimeType.c_str());
}

}  // namespace engine
