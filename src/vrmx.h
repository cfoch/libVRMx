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
  public:
    VRM vrm;
    std::unique_ptr<tinygltf::Model> model;

    VRMContext(std::unique_ptr<tinygltf::Model> &model);
    static VRMContext LoadBinaryFromFile (std::string filePath);
    void ToJSONFile (const std::string &path);
};
}

#endif /* __VRMX_H__ */
