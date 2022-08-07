#ifndef __VRMX_H__
#define __VRMX_H__

#include <memory>
#include <string>
#include "tiny_gltf.h"
#include "vrmx-types.h"
#include "spdlog/spdlog.h"
#include "spdlog/cfg/env.h"

namespace vrmx
{
class VRMContext
{
  std::unique_ptr<tinygltf::Model> model;

  public:
    VRM vrm;

    VRMContext(std::unique_ptr<tinygltf::Model> &model);
    static VRMContext LoadBinaryFromFile (std::string filePath);
};
}

#endif /* __VRMX_H__ */
