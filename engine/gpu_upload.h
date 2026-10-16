#pragma once
// Smooth loading: GPU uploads that don't freeze the frame.
//
// - A job queue run with a time budget each frame (runJobs), so a model
//   that arrives with 40 textures is set up over several frames instead
//   of one long stall.
// - Images go from the file bytes straight onto the GPU: the browser
//   decodes them off the main thread (createImageBitmap) and copies them
//   into the texture (copyExternalImageToTexture); mipmaps are then made
//   on the GPU. No pixel ever passes through the CPU.
#include <webgpu/webgpu_cpp.h>

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace engine {

// Adds work to run later in a frame (in order).
void queueJob(std::function<void()> job);
// Runs queued jobs until `budgetMs` has passed (always at least one).
void runJobs(double budgetMs);
size_t pendingJobs();

// Decodes PNG/JPEG bytes and creates a mipmapped texture from them, once
// for each format in `formats` (RGBA8Unorm for data like normal maps,
// RGBA8UnormSrgb for colors). Colors are taken exactly as stored. `done`
// runs from the job queue with the textures (empty on failure).
void loadImageTexture(const wgpu::Device& device, const std::vector<uint8_t>& bytes, const std::string& mimeType,
                      std::vector<wgpu::TextureFormat> formats, const char* label,
                      std::function<void(std::vector<wgpu::Texture> textures)> done);

// Fills mip levels 1.. of `texture` from level 0 (each level averages the
// one above, in linear light for sRGB formats). The texture needs
// TextureBinding and RenderAttachment usage.
void generateMipmaps(const wgpu::Device& device, const wgpu::Texture& texture);

}  // namespace engine
