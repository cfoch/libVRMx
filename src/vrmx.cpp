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

VRMContext::AttrShaderInfo VRMContext::attrShaderInfoPrimitives[] =
{
  { "POSITION", "in_position", false },
  { "TEXCOORD_0", "in_texCoord", false }, // FIXME: Support TEXCOORD_1, TEXCOORD_2, ..., TEXCOORD_N
  { "NORMAL", "in_normal", false },
  { "", "", false },
};

VRMContext::AttrShaderInfo VRMContext::attrShaderInfoSettings[] =
{
  { "model", "u_model", true},
  { "view", "u_view", true},
  { "projection", "u_projection", true},
  { "cameraPosition", "u_cameraPosition", true},
  { "lightPositions", "u_lightPositions", true},
  { "lightColors", "u_lightColors", true},
  { "lightCount", "u_lightCount", true},
  { "ignoreNormals", "u_ignoreNormals", true},
  { "", "", false },
};

VRMContext::AttrShaderInfo VRMContext::attrShaderInfoPBR[] =
{
  { "baseColorFactor", "u_baseColorFactor", true},
  { "metallicFactor", "u_metallicFactor", true },
  { "roughnessFactor", "u_roughnessFactor", true },
  { "baseColorTexture", "u_baseColorTexture", true },
  { "", "", false },
};

bool
VRMContext::IsValidAttr (VRMContext::AttrShaderInfo (&info)[],
    const std::string &attr, bool checkState = true)
{
  int i;
  bool res = false;

  for (i = 0; !info[i].attr.empty(); i++) {
    if (info[i].attr == attr) {
      res = true;
      break;
    }
  }

  if (res && checkState)
    return res && state.shaders.attributes.find (attr) !=
      state.shaders.attributes.end () && state.shaders.attributes[attr] >=
      0;
  return res;
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

  DrawSettings ();
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
VRMContext::DrawSettings ()
{
  for (const auto &[attr, location] : state.shaders.attributes) {
    for (const auto &info: attrShaderInfoSettings) {
      if (info.attr != attr) // TODO: IsValidAttr?
        continue;

      if (attr == "model") {
        glUniformMatrix4fv (location, 1, GL_FALSE, glm::value_ptr (settings.model));
      } else if (attr == "projection") {
        glUniformMatrix4fv (location, 1, GL_FALSE, glm::value_ptr (settings.projection));
      } else if (attr == "view") {
        glm::mat4 view = glm::lookAt(settings.camera.position,
            settings.camera.position + settings.camera.front,
            settings.camera.up);
        glUniformMatrix4fv (location, 1, GL_FALSE, glm::value_ptr (view));
      } else if (attr == "cameraPosition") {
        glUniform3fv(location, 1, &settings.camera.position[0]);
      } else if (attr == "lightPositions") {
        glUniform3fv(location, settings.lighting.count,
            &settings.lighting.positions[0][0]);
      } else if (attr == "lightColors") {
        glUniform3fv(location, settings.lighting.count,
            &settings.lighting.colors[0][0]);
      } else if (attr == "lightCount") {
        glUniform1ui (location, settings.lighting.count);
      } else if (attr == "ignoreNormals") {
        glUniform1i (location, settings.ignoreNormals);
      }
    }
  }
}

void
VRMContext::DrawTexture (const tinygltf::Primitive &primitive, const tinygltf::TextureInfo &textureInfo)
{
  unsigned int texId, texLoc;
  tinygltf::Texture &texture = model->textures[textureInfo.index];
  tinygltf::Sampler &sampler = model->samplers[texture.sampler];
  tinygltf::Image &image = model->images[texture.source];
  tinygltf::BufferView &bufferView = model->bufferViews[image.bufferView];

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, 1);

  // char name[32];
  // sprintf(name, "TEXCOORD_%d", textureInfo.texCoord);

  // glGenTextures(1, &texId);
  // glActiveTexture(GL_TEXTURE0);
  // glBindTexture(GL_TEXTURE_2D, texId);

  // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, sampler.wrapS);
  // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, sampler.wrapT);
  // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, sampler.minFilter);
  // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, sampler.magFilter);

  // // FIXME: This bufferView data has been already used in SetupMesh().
  // glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image.width, image.height, 0,
  //     GL_RGBA, image.pixel_type, &image.image[0]);
  // glGenerateMipmap(GL_TEXTURE_2D);

  // It seems not needed.
  // See https://learnopengl.com/code_viewer_gh.php?code=src/1.getting_started/4.1.textures/textures.cpp
  texLoc = glGetUniformLocation (state.programId, "u_baseColorTexture");
  assert (texLoc != -1);
  glUniform1i(texLoc, 0);

}

void
VRMContext::DrawMaterial (const tinygltf::Primitive &primitive, const tinygltf::Material &material)
{
  // TODO: Iteration here needed? Maybe just call glGetUniformLocation here?
  for (const auto &[attr, location] : state.shaders.attributes) {
    for (const auto &info: attrShaderInfoPBR) {
      if (!IsValidAttr (attrShaderInfoPBR, info.attr, false) ||
          info.attr != attr)
        continue;
      if (attr == "baseColorFactor") {
        std::vector<GLfloat> baseColorFactor(
            material.pbrMetallicRoughness.baseColorFactor.begin (),
            material.pbrMetallicRoughness.baseColorFactor.end ());
        glUniform4fv (location, 1, &baseColorFactor[0]);
      } else if (attr == "baseColorTexture") {
        DrawTexture (primitive, material.pbrMetallicRoughness.baseColorTexture);
      } else if (attr == "metallicFactor") {
        glUniform1f (location, material.pbrMetallicRoughness.metallicFactor);
      } else if (attr == "roughnessFactor") {
        glUniform1f (location, material.pbrMetallicRoughness.roughnessFactor);
      }
    }
  }
}

void
VRMContext::DrawMesh (const tinygltf::Mesh &mesh)
{
  for (size_t i = 0; i < mesh.primitives.size (); i++) {
    const tinygltf::Primitive &primitive = mesh.primitives[i];

    if (primitive.indices < 0)
      return;

    // Accoring ChatGPT, I should use one VAO per-primitive. I tested it here, and worked.
    glBindVertexArray (1);

    for (const auto &[attr, accessorIdx]: primitive.attributes) {
      assert (accessorIdx >= 0);
      const tinygltf::Accessor &accessor = model->accessors[accessorIdx];

      int size = GetNumComponentsInType (accessor.type);

      // TODO: Support TEXTCOORD_0, TEXTCOORD_1, ..., TEXTCOORD_N
      if (!IsValidAttr (attrShaderInfoPrimitives, attr))
        continue;

      glBindBuffer (GL_ARRAY_BUFFER, state.buffers.vbos[accessor.bufferView]);

      // Compute byteStride from Accessor + BufferView combination.
      int byteStride = accessor.ByteStride (
          model->bufferViews[accessor.bufferView]);
      assert (byteStride != -1);

      glEnableVertexAttribArray (state.shaders.attributes[attr]);
      glVertexAttribPointer (state.shaders.attributes[attr], size,
          accessor.componentType, accessor.normalized ? GL_TRUE : GL_FALSE,
          byteStride, BUFFER_OFFSET (accessor.byteOffset));
    }
  
    glUseProgram(state.programId);
    if (primitive.material >= 0)
      DrawMaterial (primitive, model->materials[primitive.material]);

    const tinygltf::Accessor &indexAccessor =
        model->accessors[primitive.indices];
    glBindBuffer (GL_ELEMENT_ARRAY_BUFFER,
        state.buffers.vbos[indexAccessor.bufferView]);

    int mode = TinygltfModeToGLEnum (primitive.mode);
    glDrawElements (mode, indexAccessor.count, indexAccessor.componentType,
        BUFFER_OFFSET (indexAccessor.byteOffset));

    for (const auto &[attr, accessorIdx]: primitive.attributes) {
      // TODO: Support TEXTCOORD_0, TEXTCOORD_1, ..., TEXTCOORD_N
      if (!IsValidAttr (attrShaderInfoPrimitives, attr))
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

void
VRMContext::SetSettings (const VRMSettings &settings)
{
  this->settings = settings;
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
    state.buffers.vbos[i] = vbo;
    // Accoring ChatGPT, I should use one VAO per-primitive.
    state.buffers.vaos[i] = vao;

    std::cout << i << ". vao: " << vao << std::endl;
  }

  glUseProgram (state.programId);

  for (const auto &info: attrShaderInfoPrimitives) {
    if (!IsValidAttr (attrShaderInfoPrimitives, info.attr, false))
      continue;

    state.shaders.attributes[info.attr] = info.isUniform ?
        glGetUniformLocation (state.programId, info.var) :
        glGetAttribLocation (state.programId, info.var);

      std::cout << "isUniform: " << info.isUniform << std::endl;
      std::cout << "val: " << info.var << std::endl;
      std::cout << "location: " << state.shaders.attributes[info.attr]  << std::endl;
      std::cout << "-------------" << std::endl;

    assert (state.shaders.attributes[info.attr] >= 0);
  }

  for (const auto &info: attrShaderInfoPBR) {
    if (!IsValidAttr (attrShaderInfoPBR, info.attr, false))
      continue;

    state.shaders.attributes[info.attr] = info.isUniform ?
        glGetUniformLocation (state.programId, info.var) :
        glGetAttribLocation (state.programId, info.var);
    assert (state.shaders.attributes[info.attr] >= 0);
  }

  for (const auto &info: attrShaderInfoSettings) {
    if (!IsValidAttr (attrShaderInfoSettings, info.attr, false))
      continue;

    state.shaders.attributes[info.attr] = info.isUniform ?
        glGetUniformLocation (state.programId, info.var) :
        glGetAttribLocation (state.programId, info.var);
    assert (state.shaders.attributes[info.attr] >= 0);
  }

  for (auto &texture : model->textures) {
    unsigned int texId, texLoc;
    tinygltf::Sampler &sampler = model->samplers[texture.sampler];
    tinygltf::Image &image = model->images[texture.source];
    tinygltf::BufferView &bufferView = model->bufferViews[image.bufferView];

    std::cout << "process texture " << std::endl;
    glGenTextures(1, &texId);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texId);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, sampler.wrapS);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, sampler.wrapT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, sampler.minFilter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, sampler.magFilter);

    // FIXME: This bufferView data has been already used in SetupMesh().
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image.width, image.height, 0,
        GL_RGBA, image.pixel_type, &image.image[0]);
    glGenerateMipmap(GL_TEXTURE_2D);
    

    std::cout << "textId: " << texId << std::endl;
  }



  return true;
}
}
