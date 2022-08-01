#include <iostream>
#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "vrmx.h"

namespace vrmx
{
VRMContext::VRMContext(std::unique_ptr<tinygltf::Model> &model)
{
  this->model = std::move (model);
}

VRMContext
VRMContext::LoadBinaryFromFile (std::string filePath)
{
  std::unique_ptr<tinygltf::Model> model (new tinygltf::Model);
  tinygltf::TinyGLTF loader;
  std::string err, warn;
  bool ret;

  ret = loader.LoadBinaryFromFile (model.get (), &err, &warn, filePath);

  if (!warn.empty ())
    std::cerr << warn << std::endl;

  if (!err.empty () || !ret)
    throw err;

  return VRMContext (model);
}
}
