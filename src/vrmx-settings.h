#pragma once

#define MAX_LIGHTS  16

namespace vrmx
{
struct VRMCamera {
  glm::vec3 position;
  glm::vec3 front;
  glm::vec3 up;
};

struct VRMSettings {
  struct {
    glm::vec3 positions[MAX_LIGHTS];
    glm::vec3 colors[MAX_LIGHTS];
    size_t count;
  } lighting;
  VRMCamera camera;
  glm::mat4 model;
  glm::mat4 projection;
  bool ignoreNormals;
};
};