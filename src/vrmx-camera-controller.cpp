#include "vrmx-camera-controller.h"

namespace vrmx
{
VRMCameraController::VRMCameraController (VRMCamera& camera) :
  camera (camera)
{
  // Nothing.
}

void
VRMCameraController::SetMouseSensitivity (float sensitivity)
{
  mouseSensitivity = sensitivity;
}

void
VRMCameraController::ProcessMouseMotion (double x, double y)
{
  // FIXME: Control is not accurate enough.
  float radius = GetCameraRadius ();

  if (isFirstMotion) {
    lastX = x;
    lastY = y;
    isFirstMotion = false;
  }

  double x_offset = (lastX - x) * mouseSensitivity;
  double y_offset = (lastY - y) * mouseSensitivity;

  lastX = x;
  lastY = y;

  theta += x_offset;
  phi += y_offset;
  phi = std::clamp (phi, 0.1f, glm::pi<float>() - 0.1f);

  camera.position.x = radius * sin (phi) * cos (theta);
  camera.position.y = radius * cos (phi);
  camera.position.z = radius * sin (phi) * sin (theta);
  camera.front = glm::normalize (-camera.position);
}

float
VRMCameraController::GetCameraRadius () const
{
  glm::vec3 target (0.0f); // Assume target is at origin.
  return glm::length (camera.position - target);
}

}