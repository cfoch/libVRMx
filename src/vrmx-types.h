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

typedef struct : public ISerializable
{
  enum class VRMHumanoidBoneType
  {
    hips,
    leftUpperLeg,
    rightUpperLeg,
    leftLowerLeg,
    rightLowerLeg,
    leftFoot,
    rightFoot,
    spine,
    chest,
    neck,
    head,
    leftShoulder,
    rightShoulder,
    leftUpperArm,
    rightUpperArm,
    leftLowerArm,
    rightLowerArm,
    leftHand,
    rightHand,
    leftToes,
    rightToes,
    leftEye,
    rightEye,
    jaw,
    leftThumbProximal,
    leftThumbIntermediate,
    leftThumbDistal,
    leftIndexProximal,
    leftIndexIntermediate,
    leftIndexDistal,
    leftMiddleProximal,
    leftMiddleIntermediate,
    leftMiddleDistal,
    leftRingProximal,
    leftRingIntermediate,
    leftRingDistal,
    leftLittleProximal,
    leftLittleIntermediate,
    leftLittleDistal,
    rightThumbProximal,
    rightThumbIntermediate,
    rightThumbDistal,
    rightIndexProximal,
    rightIndexIntermediate,
    rightIndexDistal,
    rightMiddleProximal,
    rightMiddleIntermediate,
    rightMiddleDistal,
    rightRingProximal,
    rightRingIntermediate,
    rightRingDistal,
    rightLittleProximal,
    rightLittleIntermediate,
    rightLittleDistal,
    upperChest,
  };

  VRMHumanoidBoneType bone;
  int node;
  // NOTE: The following fields seems to be for Unity.
  // bool useDefaultValues;
  // double min[3];
  // double max[3];
  // double center[3];
  // double axisLength;

  void Deserialize (const tinygltf::Value & val);
} VRMHumanoidBone;

typedef struct : public ISerializable
{
  std::vector<VRMHumanoidBone> humanBones;
  // NOTE: The following fields seems to be for Unity.
  // double armStretch;
  // double legStretch;
  // double upperArmTwist;
  // double lowerArmTwist;
  // double upperLegTwist;
  // double lowerLegTwist;
  // double feetSpacing;
  // bool hasTranslationDoF;

  void Deserialize (const tinygltf::Value & val);
} VRMHumanoid;

struct VRM : public ISerializable
{
  VRMMeta meta;
  VRMHumanoid humanoid;

  void Deserialize (const tinygltf::Value &val);
};
}

#endif /* __VRMX_TYPES_H__ */
