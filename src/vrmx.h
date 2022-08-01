#ifndef __VRMX_H__
#define __VRMX_H__

#include <memory>
#include <string>
#include "tiny_gltf.h"

namespace vrmx
{
class VRMContext
{
  std::unique_ptr<tinygltf::Model> model;

  public:
    VRMContext(std::unique_ptr<tinygltf::Model> &model);
    static VRMContext LoadBinaryFromFile (std::string filePath);
};
}

#endif /* __VRMX_H__ */
