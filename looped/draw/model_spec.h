#pragma once
// What a model must be (J116.6; request 2026-11-01 #3): the new skeleton
// draws every model as clusters with simpler versions (clusters.h), and
// that only works well when a model is made a certain way. These are the
// rules, each with the number it's measured by, why it matters and how to
// fix it. The tools check every model against them (node tools/clusters.mjs
// spec); a model that breaks one is remade, not worked around. Plain C++.
//
// The shape rules (simplify, seams, pieces, open edges) apply to models of
// 2,000 triangles or more: smaller ones are drawn whole.
//
// Measured after joining corners that share a position and a texture place
// (the new frame shades by each triangle's own facing, so split normals
// don't count as seams).
#include <cstdint>
#include <string>
#include <vector>

namespace engine::draw {

struct ModelMeasure {
  uint32_t triangles = 0;
  uint32_t corners = 0;       // after joining
  float seamShare = 0;        // corners that share their position with another corner (texture seams)
  uint32_t parts = 0;         // separate pieces (nothing joins them)
  float trianglesPerPart = 0;
  float openShare = 0;        // edges with a triangle on one side only (holes, open sheets)
  float simplestShare = 0;    // the simplest whole version, as a share of every triangle
  uint32_t materials = 0;
  float size = 0;             // its biggest side (metres)
};

struct ModelRule {
  const char* id;     // "M-SEAMS"
  const char* rule;   // the rule, in plain words
  const char* why;
  const char* fix;
};

struct ModelProblem {
  const ModelRule* rule;
  std::string measured;  // "38% of corners on seams (at most 20%)"
};

// The rules, in order.
const std::vector<ModelRule>& modelRules();
// The rules a measured model breaks.
std::vector<ModelProblem> checkModel(const ModelMeasure& m);

}  // namespace engine::draw

namespace engine {
struct ModelData;
}

namespace engine::draw {
struct ClusterTree;
// Measures a model for the spec: joins corners (position, and texture
// place where its material has a picture), builds each part's clusters
// (`trees`, when given, gets them), and counts seams, pieces, open edges.
ModelMeasure measureModel(const ModelData& m, std::vector<ClusterTree>* trees = nullptr);
}  // namespace engine::draw
