#ifndef __VRMX_TYPES_H__
#define __VRMX_TYPES_H__

#include <vector>
#include "magic_enum.hpp"

namespace tinygltf
{
class Value;
}

namespace vrmx
{
typedef struct ISerializable
{
  virtual void Deserialize (const tinygltf::Value &val) = 0;
  virtual ~ISerializable() = default;
} ISerializable;

typedef struct : public ISerializable
{
  enum class VRMMetaAllowedUserName
  {
    OnlyAuthor = 2,
    ExplicitlyLicensedPerson = 4,
    Everyone = 8
  };

  enum class VRMMetaAllow
  {
    Allow,
    Disallow
  };

  std::string title;
  std::string version;
  std::string author;
  std::string contactInformation;
  std::string reference;
  int texture;
  VRMMetaAllowedUserName allowedUserName;
  VRMMetaAllow violentUssageName;
  VRMMetaAllow sexualUssageName;
  VRMMetaAllow commercialUssageName;
  std::string otherPermissionUrl;
  std::string licenseName;
  std::string otherLicenseUrl;

  void Deserialize (const tinygltf::Value & val);
} VRMMeta;

struct VRM : public ISerializable
{
  VRMMeta meta;

  void Deserialize (const tinygltf::Value &val);
};
}

#endif /* __VRMX_TYPES_H__ */
