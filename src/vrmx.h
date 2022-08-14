#ifndef __VRMX_H__
#define __VRMX_H__

#include <memory>
#include <string>
#include <epoxy/gl.h>
#include <epoxy/glx.h>
#include <GL/glu.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "tiny_gltf.h"
#include "vrmx-types.h"
#include "spdlog/spdlog.h"
#include "spdlog/cfg/env.h"


#define MAX_LIGHTS  16

namespace vrmx
{
struct VRMSettings {
  struct {
    glm::vec3 positions[MAX_LIGHTS];
    glm::vec3 colors[MAX_LIGHTS];
    size_t count;
  } lighting;
  struct {
    glm::vec3 position;
    glm::vec3 front;
    glm::vec3 up;
  } camera;
  glm::mat4 model;
  glm::mat4 projection;
  bool ignoreNormals;
};

class VRMContext
{
  public:
    VRM vrm;
    std::unique_ptr<tinygltf::Model> model;

    VRMContext(std::unique_ptr<tinygltf::Model> &model);

    static VRMContext LoadBinaryFromFile (std::string filePath);
    void ToJSONFile (const std::string &path);

    bool Setup (GLuint programId);
    void Draw (void);
    bool IsSetup (void) const;
    void SetSettings (const VRMSettings &settings);

  private:
    struct BufferState
    {
      std::map<int, int> vaos;
      std::map<int, int> vbos;
    };
    struct ShadersState
    {
      unsigned int vertexShaderId;
      unsigned int fragmentShaderId;
      std::map<std::string, GLuint> attributes;
    };

    struct State
    {
      GLuint programId;
      struct BufferState buffers;
      struct ShadersState shaders;
      bool isSetup;
    };
    struct AttrShaderInfo
    {
      std::string attr;
      const GLchar *var;
      bool isUniform;
      // TODO: Indicate how to map to a glUniformXXX function?
    };

    struct State state;
    struct VRMSettings settings;

    static struct AttrShaderInfo attrShaderInfoSettings[];
    static struct AttrShaderInfo attrShaderInfoPrimitives[];
    static struct AttrShaderInfo attrShaderInfoPBR[];

    bool InitShaders ();
    GLuint LoadShader (const std::string &src, const GLenum type);
    bool LinkShader ();
    bool SetupMesh (void);
    void DrawNode (const tinygltf::Node &node);
    void DrawMesh (const tinygltf::Mesh &mesh);
    void DrawMaterial (const tinygltf::Material &material);
    void DrawSettings (void);
    bool IsValidAttr (VRMContext::AttrShaderInfo (&info)[],
        const std::string &attr, bool checkState);
};
}

#endif /* __VRMX_H__ */
