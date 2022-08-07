#include "vrmx-io.h"

#define VRMX_DUMP_STRUCT_ENUM_FIELD(st, key, os) \
  (os) << #key << " : " << magic_enum::enum_name (st.key) << std::endl

#define VRMX_DUMP_STRUCT_FIELD(st, key, os) \
  (os) << #key << " : " << st.key << std::endl

std::ostream &
operator << (std::ostream &os, const vrmx::VRMMeta &meta)
{
  VRMX_DUMP_STRUCT_FIELD (meta, title, os);
  VRMX_DUMP_STRUCT_FIELD (meta, version, os);
  VRMX_DUMP_STRUCT_FIELD (meta, author, os);
  VRMX_DUMP_STRUCT_FIELD (meta, contactInformation, os);
  VRMX_DUMP_STRUCT_FIELD (meta, reference, os);
  VRMX_DUMP_STRUCT_FIELD (meta, texture, os);
  VRMX_DUMP_STRUCT_ENUM_FIELD (meta, allowedUserName, os);
  VRMX_DUMP_STRUCT_ENUM_FIELD (meta, violentUssageName, os);
  VRMX_DUMP_STRUCT_ENUM_FIELD (meta, sexualUssageName, os);
  VRMX_DUMP_STRUCT_ENUM_FIELD (meta, commercialUssageName, os);
  VRMX_DUMP_STRUCT_FIELD (meta, otherPermissionUrl, os);
  VRMX_DUMP_STRUCT_FIELD (meta, licenseName, os);
  VRMX_DUMP_STRUCT_FIELD (meta, otherLicenseUrl, os);
  return os;
}
