
#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include "vrmx-settings.h"

namespace vrmx
{
class VRMCameraController {
  public:
    VRMCameraController (VRMCamera& camera);
    void ProcessMouseMotion (double x, double y);
    void SetMouseSensitivity (float sensitivity);

  private:
    float GetCameraRadius () const;

    VRMCamera& camera;
    float phi{0.0f};
    float theta{0.0f};
    float mouseSensitivity{0.01f};
    bool isFirstMotion{true};
    double lastX{0.0};
    double lastY{0.0};
};


};