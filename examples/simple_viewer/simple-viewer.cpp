#include <optional>
#include <iostream>
#include <string>
#include <gtk/gtk.h>
#include <epoxy/gl.h>
#include <epoxy/glx.h>
#include <GL/glu.h>
#include "vrmx.h"
#include "vrmx-io.h"

#define APP_NAME            "simple-viewer"
#define APP_WINDOW_WIDTH    600
#define APP_WINDOW_HEIGHT   400
#define FRAGMENT_SHADER     PROG_DIR "/shaders/shader.frag"
#define VERTEX_SHADER       PROG_DIR "/shaders/shader.vert"
#define CAM_Z               3.0f

#define BUFFER_OFFSET(i) ((char *)NULL + (i))

typedef struct {
  GLuint vb;
} GLBufferState;

typedef struct {
  std::optional<vrmx::VRMContext> vrmCtx;
  std::string filePath;
  guint progId;
  guint vertId;
  guint fragId;
  float eye[3];
  float lookat[3];
  float up[3];
  std::map<int, GLBufferState> gBufferState;
  struct {
    std::map<std::string, GLint> attribs;
    std::map<std::string, GLint> uniforms;
  } gGLProgramState;
} SimpleViewerContext;

bool LoadShader(GLenum shaderType,  // GL_VERTEX_SHADER or GL_FRAGMENT_SHADER(or
                                    // maybe GL_COMPUTE_SHADER) 
                GLuint &shader, const char *shaderSourceFilename) {
  GLint val = 0;

  // free old shader/program
  if (shader != 0) {
    glDeleteShader(shader);
  }

  std::vector<GLchar> srcbuf;
  FILE *fp = fopen(shaderSourceFilename, "rb");
  if (!fp) {
    fprintf(stderr, "failed to load shader: %s\n", shaderSourceFilename);
    return false;
  }
  fseek(fp, 0, SEEK_END);
  size_t len = ftell(fp);
  rewind(fp);
  srcbuf.resize(len + 1);
  len = fread(&srcbuf.at(0), 1, len, fp);
  srcbuf[len] = 0;
  fclose(fp);

  const GLchar *srcs[1];
  srcs[0] = &srcbuf.at(0);

  shader = glCreateShader(shaderType);
  glShaderSource(shader, 1, srcs, NULL);
  glCompileShader(shader);
  glGetShaderiv(shader, GL_COMPILE_STATUS, &val);
  if (val != GL_TRUE) {
    char log[4096];
    GLsizei msglen;
    glGetShaderInfoLog(shader, 4096, &msglen, log);
    printf("%s\n", log);
    // assert(val == GL_TRUE && "failed to compile shader");
    printf("ERR: Failed to load or compile shader [ %s ]\n",
           shaderSourceFilename);
    return false;
  }

  printf("Load shader [ %s ] OK\n", shaderSourceFilename);
  return true;
}

bool LinkShader(GLuint &prog, GLuint &vertShader, GLuint &fragShader) {
  GLint val = 0;

  if (prog != 0) {
    glDeleteProgram(prog);
  }

  prog = glCreateProgram();

  glAttachShader(prog, vertShader);
  glAttachShader(prog, fragShader);
  glLinkProgram(prog);

  glGetProgramiv(prog, GL_LINK_STATUS, &val);
  assert(val == GL_TRUE && "failed to link shader");

  printf("Link shader OK\n");

  return true;
}

static void
init_shaders (SimpleViewerContext *ctx)
{
  if (!LoadShader (GL_VERTEX_SHADER, ctx->vertId, VERTEX_SHADER))
    abort ();
  if (!LoadShader (GL_FRAGMENT_SHADER, ctx->fragId, FRAGMENT_SHADER))
    abort ();

  LinkShader (ctx->progId, ctx->vertId, ctx->fragId);

  // glUseProgram(ctx->progId);

}

static void
setup_mesh_state (SimpleViewerContext *ctx)
{
  vrmx::VRMContext &vrmCtx = ctx->vrmCtx.value ();
  tinygltf::Model &model = *vrmCtx.model;
  auto &gBufferState = ctx->gBufferState;

  for (size_t i = 0; i < model.bufferViews.size(); i++) {
    // Map BufferView.
    const tinygltf::BufferView &bufferView = model.bufferViews[i];
    if (bufferView.target == 0) {
      std::cout << "WARN: bufferView.target is zero" << std::endl;
      continue;  // Unsupported bufferView.
    }

    // Map accessors.
    // int sparse_accessor = -1;
    for (size_t a_i = 0; a_i < model.accessors.size(); ++a_i) {
      const auto &accessor = model.accessors[a_i];
      if (accessor.bufferView == i) {
        std::cout << i << " is used by accessor " << a_i << std::endl;
        if (accessor.sparse.isSparse) {
          std::cout << "Sparse accessors not supported yet." << std::endl;
          abort ();
        }
      }
    }

    const tinygltf::Buffer &buffer = model.buffers[bufferView.buffer];
    GLBufferState state;
    glGenBuffers(1, &state.vb);
    glBindBuffer(bufferView.target, state.vb);
    std::cout << "buffer.size= " << buffer.data.size()
              << ", byteOffset = " << bufferView.byteOffset << std::endl;

    glBufferData(bufferView.target, bufferView.byteLength,
                  &buffer.data.at(0) + bufferView.byteOffset,
                  GL_STATIC_DRAW);

    // unbinds: needed?
    glBindBuffer(bufferView.target, 0);

    gBufferState[i] = state;
  }

  glUseProgram(ctx->progId);

  GLint vtloc = glGetAttribLocation(ctx->progId, "in_vertex");
  GLint nrmloc = glGetAttribLocation(ctx->progId, "in_normal");
  GLint uvloc = glGetAttribLocation(ctx->progId, "in_texcoord");

  // GLint diffuseTexLoc = glGetUniformLocation(progId, "diffuseTex");
  GLint isCurvesLoc = glGetUniformLocation(ctx->progId, "uIsCurves");

  ctx->gGLProgramState.attribs["POSITION"] = vtloc;
  ctx->gGLProgramState.attribs["NORMAL"] = nrmloc;
  ctx->gGLProgramState.attribs["TEXCOORD_0"] = uvloc;
  // gGLProgramState.uniforms["diffuseTex"] = diffuseTexLoc;
  ctx->gGLProgramState.uniforms["isCurvesLoc"] = isCurvesLoc;

}

static void QuatToAngleAxis(const std::vector<double> quaternion,
			    double &outAngleDegrees,
			    double *axis) {
  double qx = quaternion[0];
  double qy = quaternion[1];
  double qz = quaternion[2];
  double qw = quaternion[3];
  
  double angleRadians = 2 * acos(qw);
  if (angleRadians == 0.0) {
    outAngleDegrees = 0.0;
    axis[0] = 0.0;
    axis[1] = 0.0;
    axis[2] = 1.0;
    return;
  }

  double denom = sqrt(1-qw*qw);
  outAngleDegrees = angleRadians * 180.0 / M_PI;
  axis[0] = qx / denom;
  axis[1] = qy / denom;
  axis[2] = qz / denom;
}

static void DrawMesh(SimpleViewerContext *ctx, const tinygltf::Mesh &mesh) {
  tinygltf::Model &model = *ctx->vrmCtx->model;

  auto &gBufferState = ctx->gBufferState;
  auto &gGLProgramState = ctx->gGLProgramState;

  if (ctx->gGLProgramState.uniforms["isCurvesLoc"] >= 0) {
    glUniform1i(ctx->gGLProgramState.uniforms["isCurvesLoc"], 0);
  }

  for (size_t i = 0; i < mesh.primitives.size(); i++) {
    const tinygltf::Primitive &primitive = mesh.primitives[i];

    if (primitive.indices < 0) return;

    // Assume TEXTURE_2D target for the texture object.
    // glBindTexture(GL_TEXTURE_2D, gMeshState[mesh.name].diffuseTex[i]);

    std::map<std::string, int>::const_iterator it(primitive.attributes.begin());
    std::map<std::string, int>::const_iterator itEnd(
        primitive.attributes.end());

    for (; it != itEnd; it++) {
      assert(it->second >= 0);
      const tinygltf::Accessor &accessor = model.accessors[it->second];
      glBindBuffer(GL_ARRAY_BUFFER, gBufferState[accessor.bufferView].vb);
      // CheckErrors("bind buffer");
      int size = 1;
      if (accessor.type == TINYGLTF_TYPE_SCALAR) {
        size = 1;
      } else if (accessor.type == TINYGLTF_TYPE_VEC2) {
        size = 2;
      } else if (accessor.type == TINYGLTF_TYPE_VEC3) {
        size = 3;
      } else if (accessor.type == TINYGLTF_TYPE_VEC4) {
        size = 4;
      } else {
        assert(0);
      }
      // it->first would be "POSITION", "NORMAL", "TEXCOORD_0", ...
      if ((it->first.compare("POSITION") == 0) ||
          (it->first.compare("NORMAL") == 0) ||
          (it->first.compare("TEXCOORD_0") == 0)) {
        if (gGLProgramState.attribs[it->first] >= 0) {
          // Compute byteStride from Accessor + BufferView combination.
          int byteStride =
              accessor.ByteStride(model.bufferViews[accessor.bufferView]);
          assert(byteStride != -1);
          glVertexAttribPointer(gGLProgramState.attribs[it->first], size,
                                accessor.componentType,
                                accessor.normalized ? GL_TRUE : GL_FALSE,
                                byteStride, BUFFER_OFFSET(accessor.byteOffset));
          // CheckErrors("vertex attrib pointer");
          glEnableVertexAttribArray(gGLProgramState.attribs[it->first]);
          // CheckErrors("enable vertex attrib array");
        }
      }
      // TODO: Else unbind?
    }

    const tinygltf::Accessor &indexAccessor =
        model.accessors[primitive.indices];
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,
                 gBufferState[indexAccessor.bufferView].vb);
    // CheckErrors("bind buffer");
    int mode = -1;
    if (primitive.mode == TINYGLTF_MODE_TRIANGLES) {
      mode = GL_TRIANGLES;
    } else if (primitive.mode == TINYGLTF_MODE_TRIANGLE_STRIP) {
      mode = GL_TRIANGLE_STRIP;
    } else if (primitive.mode == TINYGLTF_MODE_TRIANGLE_FAN) {
      mode = GL_TRIANGLE_FAN;
    } else if (primitive.mode == TINYGLTF_MODE_POINTS) {
      mode = GL_POINTS;
    } else if (primitive.mode == TINYGLTF_MODE_LINE) {
      mode = GL_LINES;
    } else if (primitive.mode == TINYGLTF_MODE_LINE_LOOP) {
      mode = GL_LINE_LOOP;
    } else {
      assert(0);
    }
    glDrawElements(mode, indexAccessor.count, indexAccessor.componentType,
                   BUFFER_OFFSET(indexAccessor.byteOffset));
    // CheckErrors("draw elements");

    {
      std::map<std::string, int>::const_iterator it(
          primitive.attributes.begin());
      std::map<std::string, int>::const_iterator itEnd(
          primitive.attributes.end());

      for (; it != itEnd; it++) {
        if ((it->first.compare("POSITION") == 0) ||
            (it->first.compare("NORMAL") == 0) ||
            (it->first.compare("TEXCOORD_0") == 0)) {
          if (gGLProgramState.attribs[it->first] >= 0) {
            glDisableVertexAttribArray(gGLProgramState.attribs[it->first]);
          }
        }
      }
    }
  }
}

static void DrawNode(SimpleViewerContext *ctx, const tinygltf::Node &node) {
  tinygltf::Model &model = *ctx->vrmCtx->model;
  std::cout << "draaw node ......." << std::endl;
  // Apply xform
  glPushMatrix();
  if (node.matrix.size() == 16) {
    // Use `matrix' attribute
    glMultMatrixd(node.matrix.data());
  } else {
    // Assume Trans x Rotate x Scale order
    if (node.translation.size() == 3) {
      glTranslated(node.translation[0], node.translation[1],
                   node.translation[2]);
    }    

    if (node.rotation.size() == 4) {
      double angleDegrees;
      double axis[3];

      QuatToAngleAxis(node.rotation, angleDegrees, axis);
      
      glRotated(angleDegrees, axis[0], axis[1], axis[2]);
    }

    if (node.scale.size() == 3) {
      glScaled(node.scale[0], node.scale[1], node.scale[2]);
    }
  }

    std::cout << "model meshes: " << model.meshes.size () << std::endl;
    std::cout << "node children: " << node.children.size () << std::endl;
    std::cout << "node.mes: " << node.mesh << std::endl;


  if (node.mesh > -1) {
    assert(node.mesh < model.meshes.size());
    std::cout << "model mesh..." << std::endl;

    DrawMesh(ctx, model.meshes[node.mesh]);
  }

  // Draw child nodes.
  for (size_t i = 0; i < node.children.size(); i++) {
    assert(node.children[i] < model.nodes.size());
    DrawNode(ctx, model.nodes[node.children[i]]);
  }

  glPopMatrix();
}

static void
DrawModel(SimpleViewerContext *ctx)
{
  // vrmx::VRMContext &vrmCtx = ctx->vrmCtx.value ();
  tinygltf::Model &model = *ctx->vrmCtx->model;

  // If the glTF asset has at least one scene, and doesn't define a default one
  // just show the first one we can find
  assert(model.scenes.size() > 0);
  int scene_to_display = model.defaultScene > -1 ? model.defaultScene : 0;
  const tinygltf::Scene &scene = model.scenes[scene_to_display];
  for (size_t i = 0; i < scene.nodes.size(); i++) {
    DrawNode(ctx, model.nodes[scene.nodes[i]]);
  }
}

static void
render_vrm (SimpleViewerContext * ctx)
{
  vrmx::VRMContext &vrmCtx = ctx->vrmCtx.value ();
  tinygltf::Model &model = *vrmCtx.model;

  std::cout << ctx->vrmCtx->vrm.meta;
  setup_mesh_state (ctx);
  DrawModel (ctx);
}

guint VBO;
guint VAO;

static gboolean
render_cb (GtkGLArea *area, GdkGLContext *context, gpointer user_data)
{
  SimpleViewerContext *ctx = (SimpleViewerContext *) user_data;

  glClearColor(0.1f, 0.2f, 0.3f, 1.0f);
  glClear (GL_COLOR_BUFFER_BIT);

  glUseProgram (ctx->progId);
  glBindVertexArray (VAO);
  glDrawArrays (GL_TRIANGLES, 0, 3);

  // glTF code...
  // glMatrixMode(GL_PROJECTION);
  // glPushMatrix();
  // gluLookAt (ctx->eye[0], ctx->eye[1], ctx->eye[2],
  //     ctx->lookat[0], ctx->lookat[1], ctx->lookat[3],
  //     ctx->up[0], ctx->up[1], ctx->up[3]);

  // if (ctx->vrmCtx != std::nullopt)
  //   render_vrm (ctx);

  // glMatrixMode(GL_PROJECTION);
  // glPopMatrix();

  glFlush ();

  return FALSE;
}

static void
realize_cb (GtkWidget *gl_area, gpointer user_data)
{
  SimpleViewerContext *ctx = (SimpleViewerContext *) user_data;

  gtk_gl_area_make_current (GTK_GL_AREA (gl_area));
  if (gtk_gl_area_get_error (GTK_GL_AREA (gl_area)) != NULL)
    abort ();

  ctx->eye[0] = 0.0f;
  ctx->eye[1] = 0.0f;
  ctx->eye[2] = CAM_Z;
  ctx->lookat[0] = 0.0f;
  ctx->lookat[1] = 0.0f;
  ctx->lookat[2] = 0.0f;
  ctx->up[0] = 0.0f;
  ctx->up[1] = 1.0f;
  ctx->up[2] = 0.0f;

  init_shaders (ctx);

  // BEGIN STUPID TRIANGLE
  float vertices[] = {
    -0.5f, -0.5f, 0.0f, // 1.0f, 0.0f, 0.0f,
    0.5f, -0.5f, 0.0f, // 1.0f, 0.0f, 0.0f,
    0.0f, 0.5f, 0.0f, // 1.0f, 0.0f, 0.0f,
  };

  glGenBuffers (1, &VBO);
  glBindBuffer (GL_ARRAY_BUFFER, VBO);
  glBufferData (GL_ARRAY_BUFFER, sizeof (vertices), vertices, GL_STATIC_DRAW);

  glGenVertexArrays (1, &VAO);
  glBindVertexArray (VAO);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void *) 0);

  glEnableVertexAttribArray (0);
  glBindBuffer (GL_ARRAY_BUFFER, 0);
  glBindVertexArray (0);
}

static void
activate_cb (GtkApplication * app, gpointer user_data)
{
  GtkWidget *window, *gl_area;
  SimpleViewerContext *ctx = (SimpleViewerContext *) user_data;

  window = gtk_application_window_new (app);
  gtk_window_set_title (GTK_WINDOW (window), APP_NAME);
  gtk_window_set_default_size (GTK_WINDOW (window), APP_WINDOW_WIDTH,
      APP_WINDOW_HEIGHT);

  gl_area = gtk_gl_area_new ();
  g_signal_connect (gl_area, "render", G_CALLBACK (render_cb), ctx);
  g_signal_connect (gl_area, "realize", G_CALLBACK (realize_cb), ctx);

  gtk_window_set_child (GTK_WINDOW (window), gl_area);

  gtk_widget_show (gl_area);
  gtk_widget_show (window);
}

static void
load_model (SimpleViewerContext *ctx)
{
  std::optional<vrmx::VRMContext> vrmCtx = std::nullopt;

  try {
    ctx->vrmCtx = std::optional<vrmx::VRMContext> (
        vrmx::VRMContext::LoadBinaryFromFile (ctx->filePath));
  } catch (...) {
    ctx->vrmCtx = std::nullopt;
  }
}

static guint
command_line_cb (GtkApplication *app, GApplicationCommandLine *cl,
    gpointer user_data)
{
  SimpleViewerContext *ctx = (SimpleViewerContext *) user_data;
	GVariantDict *options;
	const gchar **remaining_args;

	options = g_application_command_line_get_options_dict (cl);
  g_variant_dict_lookup (options, G_OPTION_REMAINING, "^a&ay",
      &remaining_args);

  if (remaining_args[0] == NULL)
    return 1;
  ctx->filePath = std::string (remaining_args[0]);

  load_model (ctx);
  g_application_activate (G_APPLICATION (app));
  return 0;
}

static void
setup_app_options (GtkApplication *app)
{
  const GOptionEntry params[] = {
    {
      G_OPTION_REMAINING, '\0', 0, G_OPTION_ARG_FILENAME_ARRAY, NULL, NULL,
      "[FILE]"
    },
    {NULL},
  };
  g_application_add_main_option_entries (G_APPLICATION (app), params);
}

int
main (int argc, char **argv)
{
  GtkApplication *app;
  int st;
  SimpleViewerContext ctx = {};

  app = gtk_application_new ("io.github.cfoch", G_APPLICATION_FLAGS_NONE);
  g_application_set_flags (G_APPLICATION (app),
      G_APPLICATION_HANDLES_COMMAND_LINE);

  g_signal_connect (app, "activate", G_CALLBACK (activate_cb), &ctx);
  g_signal_connect (app, "command-line", G_CALLBACK (command_line_cb), &ctx);

  setup_app_options (app);

  st = g_application_run (G_APPLICATION (app), argc, argv);
  g_object_unref (app);

  return st;
}
