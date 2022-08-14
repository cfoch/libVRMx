#include <iostream>
#include <cassert>
#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "vrmx.h"
#include "vrmx-exception.h"
#include "config.h"

#define BUFFER_OFFSET(i) ((char *) NULL + (i))

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
  tinygltf::Value value;

  if (!val.Has ("meta"))
    throw;
  value = val.Get ("meta");
  meta.Deserialize (value);

  if (!val.Has ("humanoid"))
    throw;
  value = val.Get ("humanoid");
  humanoid.Deserialize (value);
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

void
VRMHumanoid::Deserialize (const tinygltf::Value &val)
{
  if (!val.Has ("humanBones"))
    throw;

  tinygltf::Value humanBonesVal = val.Get ("humanBones");
  if (!humanBonesVal.IsArray ())
    throw UnexpectedType ();

  humanBones.reserve (humanBonesVal.Size ());
  for (size_t i; i < humanBones.size (); i++)
    humanBones[i].Deserialize (humanBonesVal.Get (i));
}

void
VRMHumanoidBone::Deserialize (const tinygltf::Value &val)
{
  SetFromObjVal (val, "bone", bone);
  SetFromObjVal (val, "node", node);
}

VRMContext::VRMContext(std::unique_ptr<tinygltf::Model> &model)
{
  vrm.Deserialize (model->extensions["VRM"]);
  this->model = std::move (model);
  this->state.isSetup = false;
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
    spdlog::warn (warn);

  if (!err.empty () || !ret)
    throw err;

  return VRMContext (model);
}

void
VRMContext::ToJSONFile (const std::string &path)
{
  tinygltf::TinyGLTF tinygltf;

  tinygltf.WriteGltfSceneToFile (model.get (), path, false, false, true, false);
}

GLuint
VRMContext::LoadShader (const std::string &src, const GLenum type)
{
  GLuint shader = 0;
  GLint val;

  if (src.empty ())
    return shader;

  shader = glCreateShader (type);
  if (shader == 0) {
    spdlog::error ("Cannot load shader\n{}", src);
    return shader;
  }

  const char *csrc = src.c_str ();
  glShaderSource (shader, 1, &csrc, NULL);
  glCompileShader (shader);

  glGetShaderiv (shader, GL_COMPILE_STATUS, &val);
  if (val == GL_FALSE) {
    char log[4096];
    glGetShaderInfoLog (shader, sizeof (log) / sizeof (log[0]), NULL, log);
    spdlog::error ("Cannot compile shader {}:\n{}\n{}", shader, src, log);
    return false;
  }

  spdlog::debug ("Load shader OK:\n{}", src);
  return shader;
}

bool
VRMContext::LinkShader ()
{
  GLint val = 0;

  glAttachShader (state.programId, state.shaders.vertexShaderId);
  glAttachShader (state.programId, state.shaders.fragmentShaderId);
  glLinkProgram (state.programId);

  glGetProgramiv (state.programId, GL_LINK_STATUS, &val);
  if (val == GL_FALSE) {
    spdlog::error ("Cannot link shaders.");
    return false;
  }

  spdlog::debug ("Link shaders OK");
  return true;
}

bool
VRMContext::InitShaders ()
{
  if (!(state.shaders.vertexShaderId = LoadShader (VERTEX_SHADER_SRC,
      GL_VERTEX_SHADER)))
    return false;
  if (!(state.shaders.fragmentShaderId = LoadShader (FRAGMENT_SHADER_SRC,
      GL_FRAGMENT_SHADER)))
    return false;
  return LinkShader ();
}

void
VRMContext::Draw ()
{
  if (model->scenes.size () == 0) {
    spdlog::warn ("No scenes found. Nothing to draw.");
    return;
  }

  int sceneIdx = 0;
  if (model->defaultScene >= 0)
    sceneIdx = model->defaultScene;

  const tinygltf::Scene &scene = model->scenes[sceneIdx];
  for (size_t i = 0; i < scene.nodes.size (); i++)
    DrawNode (model->nodes[scene.nodes[i]]);
}

void
VRMContext::DrawNode (const tinygltf::Node &node)
{
  if (node.mesh > -1) {
    assert (node.mesh < model->meshes.size ());
    DrawMesh (model->meshes[node.mesh]);
  }

  for (size_t i = 0; i < node.children.size (); i++) {
    assert (node.children[i] < model->nodes.size ());
    DrawNode (model->nodes[node.children[i]]);
  }
}

static int
GetNumComponentsInType (int type)
{
  switch (type) {
    case TINYGLTF_TYPE_SCALAR:
      return 1;
    case TINYGLTF_TYPE_VEC2:
      return 2;
    case TINYGLTF_TYPE_VEC3:
      return 3;
    case TINYGLTF_TYPE_VEC4:
      return 4;
  }
  spdlog::critical ("unsupported type");
  assert (0);
}

static int
TinygltfModeToGLEnum (int mode)
{
  switch (mode) {
    case TINYGLTF_MODE_TRIANGLES:
      return GL_TRIANGLES;
    case TINYGLTF_MODE_TRIANGLE_STRIP:
      return GL_TRIANGLE_STRIP;
    case TINYGLTF_MODE_TRIANGLE_FAN:
      return GL_TRIANGLE_FAN;
    case TINYGLTF_MODE_POINTS:
      return GL_POINTS;
    case TINYGLTF_MODE_LINE:
      return GL_LINES;
    case TINYGLTF_MODE_LINE_LOOP:
      return GL_LINE_LOOP;
  }
  spdlog::critical ("unsupported type");
  assert (0);
}

void
VRMContext::DrawMesh (const tinygltf::Mesh &mesh)
{
  for (size_t i = 0; i < mesh.primitives.size (); i++) {
    const tinygltf::Primitive &primitive = mesh.primitives[i];

    if (primitive.indices < 0)
      return;

    for (const auto &[attr, accessorIdx]: primitive.attributes) {
      assert (accessorIdx >= 0);
      const tinygltf::Accessor &accessor = model->accessors[accessorIdx];

      int size = GetNumComponentsInType (accessor.type);

      if (attr != "POSITION" || state.shaders.attributes[attr] < 0)
        continue;

      glBindBuffer (GL_ARRAY_BUFFER, state.buffers.vbos[accessor.bufferView]);
      glBindVertexArray (state.buffers.vaos[accessor.bufferView]);

      // Compute byteStride from Accessor + BufferView combination.
      int byteStride = accessor.ByteStride (
          model->bufferViews[accessor.bufferView]);
      assert (byteStride != -1);
      glEnableVertexAttribArray (state.shaders.attributes[attr]);

      glVertexAttribPointer (state.shaders.attributes[attr], size,
          accessor.componentType, accessor.normalized ? GL_TRUE : GL_FALSE,
          byteStride, BUFFER_OFFSET (accessor.byteOffset));
    }

    const tinygltf::Accessor &indexAccessor =
        model->accessors[primitive.indices];
    glBindBuffer (GL_ELEMENT_ARRAY_BUFFER,
        state.buffers.vbos[indexAccessor.bufferView]);

    int mode = TinygltfModeToGLEnum (primitive.mode);
    glDrawElements (mode, indexAccessor.count, indexAccessor.componentType,
        BUFFER_OFFSET (indexAccessor.byteOffset));

    for (const auto &[attr, accessorIdx]: primitive.attributes) {
      if (attr != "POSITION" || state.shaders.attributes[attr] < 0)
        continue;
      glDisableVertexAttribArray (state.shaders.attributes[attr]);
    }
  }
}

bool
VRMContext::Setup (GLuint programId)
{
  state.programId = programId;
  if (InitShaders () && SetupMesh ()) {
    state.isSetup = true;
    return true;
  }
  return false;
}

bool
VRMContext::IsSetup () const
{
  return state.isSetup;
}

bool
VRMContext::SetupMesh ()
{
  for (size_t i = 0; i < model->bufferViews.size (); i++) {
    GLuint vao, vbo;
    const tinygltf::BufferView &bufferView = model->bufferViews[i];

    if (bufferView.target == 0) {
      spdlog::warn ("bufferView {}: target is zero", i);
      continue;
    }

    for (size_t a_i = 0; a_i < model->accessors.size (); ++a_i) {
      const auto &accessor = model->accessors[a_i];
      if (accessor.bufferView == i && accessor.sparse.isSparse) {
        spdlog::critical ("bufferView {}, accessor {}: {}",
            "Sparse accessors not supported yet.", i, a_i);
        return false;
      }
    }

    glGenVertexArrays (1, &vao);
    glGenBuffers (1, &vbo);
    glBindBuffer (bufferView.target, vbo);

    const tinygltf::Buffer &buffer = model->buffers[bufferView.buffer];
    spdlog::debug ("bufferView {}: ",
        "buffer.data.size={}, bufferView.byteOffset={}", i,
        buffer.data.size (), bufferView.byteOffset);
    glBufferData (bufferView.target, bufferView.byteLength, &buffer.data[0] +
        bufferView.byteOffset, GL_STATIC_DRAW);

    glBindBuffer (bufferView.target, 0);
    state.buffers.vbos[i] = vao;
    state.buffers.vaos[i] = vbo;
  }

  glUseProgram (state.programId);
  GLint vtloc = glGetAttribLocation (state.programId, "in_vertex");
  assert (vtloc >= 0);
  state.shaders.attributes["POSITION"] = vtloc;

  return true;
}
}
