#include <iostream>
#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "vrmx.h"
#include "vrmx-exception.h"

namespace vrmx
{
template <typename T>
static void
SetFromObjVal (const tinygltf::Value &val, std::string key, T &out)
{
  tinygltf::Value tmpVal;

  if (!val.Has (key))
    throw UnexpectedType ();

  tmpVal = val.Get (key);
  if constexpr (std::is_enum<T>::value) {
    if (!tmpVal.IsString ())
      throw UnexpectedType ();

    auto name = tmpVal.Get<std::string> ();
    auto mEnum = magic_enum::enum_cast<T> (name);

    if (!mEnum.has_value ())
      throw UnexpectedType ();

    out = mEnum.value ();
  } else if (!(std::is_same<T, std::string>::value && !tmpVal.IsString ()) &&
      !(std::is_same<T, bool>::value && !tmpVal.IsBool ()) &&
      /* !(std::is_same<T, double>::value && !tmpVal.IsNumber ()) && */
      !(std::is_same<T, int>::value && !tmpVal.IsInt ())) {
    out = val.Get (key).Get<T> ();
  }
}

void
VRM::Deserialize (const tinygltf::Value &val)
{
  auto value = val.Get ("meta");
  meta.Deserialize (value);
}

void
VRMMeta::Deserialize (const tinygltf::Value &val)
{
  SetFromObjVal (val, "title", title);
  SetFromObjVal (val, "version", version);
  SetFromObjVal (val, "author", author);
  SetFromObjVal (val, "contactInformation", contactInformation);
  SetFromObjVal (val, "reference", reference);
  SetFromObjVal (val, "allowedUserName", allowedUserName);
  SetFromObjVal (val, "violentUssageName", violentUssageName);
  SetFromObjVal (val, "sexualUssageName", sexualUssageName);
  SetFromObjVal (val, "commercialUssageName", commercialUssageName);
  SetFromObjVal (val, "otherPermissionUrl", otherPermissionUrl);
  SetFromObjVal (val, "licenseName", licenseName);
  SetFromObjVal (val, "otherLicenseUrl", otherLicenseUrl);
}

VRMContext::VRMContext(std::unique_ptr<tinygltf::Model> &model)
{
  vrm.Deserialize (model->extensions["VRM"]);
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
