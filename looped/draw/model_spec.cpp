// What a model must be (J116.6): see model_spec.h.
#include "looped/draw/model_spec.h"

#include <cstdio>

namespace engine::draw {

namespace {
constexpr float kMaxSeamShare = 0.20f;
constexpr float kMinTrianglesPerPart = 48;
constexpr float kMaxOpenShare = 0.05f;
constexpr float kMaxSimplestShare = 0.25f;
constexpr uint32_t kMaxMaterials = 8;
constexpr uint32_t kBigModel = 2000;  // smaller models are drawn whole: a 224-triangle bottle costs almost nothing
constexpr float kMaxTrianglesPerSquareMetre = 20000;

std::string pct(float x) {
  char b[32];
  std::snprintf(b, sizeof b, "%.0f%%", x * 100);
  return b;
}
}  // namespace

const std::vector<ModelRule>& modelRules() {
  static const std::vector<ModelRule> kRules = {
      {"M-SIMPLIFY", "A big model (2,000 triangles or more): its simplest version is at most a quarter of its triangles.",
       "Far away a model should cost a few triangles; a model that can't get simpler costs full price at any distance.",
       "Usually one of the rules below is the cause; fix that one."},
      {"M-SEAMS", "At most 20% of its corners sit on a texture seam.",
       "A seam can only slide along itself when simplified, and where seams meet nothing moves; many seams freeze the model.",
       "Unwrap in fewer, bigger islands; put the seams where the model already has a crease."},
      {"M-PIECES", "Its separate pieces average at least 48 triangles each.",
       "Hundreds of tiny loose pieces (screws, labels, bottles as separate bits) can only vanish one by one, so the simpler versions stay heavy.",
       "Join pieces that touch; make rows of small things (bottles, books, keys) one mesh, or a few."},
      {"M-OPEN", "At most 5% of its edges are open (a triangle on one side only).",
       "Open edges are locked when simplifying, like seams.",
       "Close holes and give sheets a thickness, or merge them into what they lie on."},
      {"M-MATERIALS", "At most 8 materials.",
       "Each material is its own draw and its own clusters; many tiny ones split the model up.",
       "Share materials, or put several looks in one texture."},
      {"M-DENSITY", "At most 20,000 triangles per square metre of its biggest side.",
       "Detail nobody can see at arm's length still has to be stored and simplified.",
       "Bake the fine detail into the texture instead of the triangles."},
  };
  return kRules;
}

std::vector<ModelProblem> checkModel(const ModelMeasure& m) {
  std::vector<ModelProblem> out;
  const auto& R = modelRules();
  auto say = [&](int i, std::string measured) { out.push_back({&R[static_cast<size_t>(i)], std::move(measured)}); };
  if (m.triangles >= kBigModel && m.simplestShare > kMaxSimplestShare)
    say(0, pct(m.simplestShare) + " of its triangles at its simplest (at most " + pct(kMaxSimplestShare) + ")");
  if (m.triangles >= kBigModel && m.seamShare > kMaxSeamShare) say(1, pct(m.seamShare) + " of corners on seams (at most " + pct(kMaxSeamShare) + ")");
  if (m.triangles >= kBigModel && m.trianglesPerPart < kMinTrianglesPerPart) {
    char b[96];
    std::snprintf(b, sizeof b, "%u pieces, %.0f triangles each (at least %.0f)", m.parts, m.trianglesPerPart, kMinTrianglesPerPart);
    say(2, b);
  }
  if (m.triangles >= kBigModel && m.openShare > kMaxOpenShare) say(3, pct(m.openShare) + " of edges open (at most " + pct(kMaxOpenShare) + ")");
  if (m.materials > kMaxMaterials) say(4, std::to_string(m.materials) + " materials (at most " + std::to_string(kMaxMaterials) + ")");
  const float area = m.size * m.size;
  if (area > 0 && m.triangles / area > kMaxTrianglesPerSquareMetre) {
    char b[96];
    std::snprintf(b, sizeof b, "%.0f triangles per m² (at most %.0f)", m.triangles / area, kMaxTrianglesPerSquareMetre);
    say(5, b);
  }
  return out;
}

}  // namespace engine::draw
