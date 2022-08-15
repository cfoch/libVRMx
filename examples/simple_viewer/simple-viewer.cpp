#include <iostream>
#include <string>
#include <gtk/gtk.h>
// #include <GL/gl.h>
#include <epoxy/gl.h>
#include <epoxy/glx.h>

#include "vrmx.h"
#include "vrmx-io.h"

#define APP_NAME            "simple-viewer"
#define APP_WINDOW_WIDTH    600
#define APP_WINDOW_HEIGHT   400
#define FRAGMENT_SHADER     PROG_DIR "/shaders/shader.frag"
#define VERTEX_SHADER       PROG_DIR "/shaders/shader.vert"

typedef struct {
  std::string filePath;
  guint progId;
  guint vertId;
  guint fragId;
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
}

static gboolean
render_cb (GtkGLArea *area, GdkGLContext *context, gpointer user_data)
{
  SimpleViewerContext *ctx = (SimpleViewerContext *) user_data;

  glClearColor (1.0, 0.0, 0.0, 1.0);
  glClear (GL_COLOR_BUFFER_BIT);

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

  init_shaders (ctx);
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
  g_signal_connect (gl_area, "render", G_CALLBACK (render_cb), NULL);
  g_signal_connect (gl_area, "realize", G_CALLBACK (realize_cb), ctx);

  gtk_window_set_child (GTK_WINDOW (window), gl_area);

  gtk_widget_show (gl_area);
  gtk_widget_show (window);
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
