#include <optional>
#include <iostream>
#include <string>
#include <gtk/gtk.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "vrmx.h"

#define APP_NAME            "simple-viewer"
#define APP_WINDOW_WIDTH    600
#define APP_WINDOW_HEIGHT   400

typedef struct
{
  std::optional<vrmx::VRMContext> vrmCtx;
  std::string filePath;
  guint progId;
} SimpleViewerContext;

static gboolean
render_cb (GtkGLArea *area, GdkGLContext *context, gpointer user_data)
{
  SimpleViewerContext *ctx = (SimpleViewerContext *) user_data;

  glClearColor (0.83f, 0.83f, 0.83f, 1.0f);
  glClear (GL_COLOR_BUFFER_BIT);

  glm::mat4 model = glm::mat4 (1.0);
  model = glm::translate (model, glm::vec3 (1.0f, 1.0f, 0.0f));
  glm::mat4 view = glm::lookAt (
      glm::vec3 (0.0f, 0.0f, 3.0f),
      glm::vec3 (0.0f, 0.0f, 0.0f),
      glm::vec3 (0.0f, 1.0f, 0.0f));
  glm::mat4 projection = glm::ortho (-3.0f, 3.0f, -3.0f, 3.0f, 0.0f, 4.0f);
  glm::mat4 mvp = projection * view * model;

  GLint mvpLoc = glGetUniformLocation (ctx->progId, "mvp");
  glUniformMatrix4fv (mvpLoc, 1, GL_FALSE, glm::value_ptr (mvp));

  if (ctx->vrmCtx != std::nullopt && ctx->vrmCtx->IsSetup ())
    ctx->vrmCtx->Draw ();

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

  if (ctx->vrmCtx != std::nullopt) {
    ctx->progId = glCreateProgram ();
    ctx->vrmCtx->Setup (ctx->progId);
  }
}

static void
activate_cb (GtkApplication *app, gpointer user_data)
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
  }
}

static guint
command_line_cb (GtkApplication *app, GApplicationCommandLine *cl, gpointer
    user_data)
{
  SimpleViewerContext *ctx = (SimpleViewerContext *) user_data;
  GVariantDict *options;
  const gchar **remaining_args;

  options = g_application_command_line_get_options_dict (cl);
  g_variant_dict_lookup (options, G_OPTION_REMAINING, "^a&ay", &remaining_args);

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
  const GOptionEntry params[] =
  {
    {
      G_OPTION_REMAINING, '\0', 0, G_OPTION_ARG_FILENAME_ARRAY, NULL, NULL,
      "[FILE]"
    }, { NULL },
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
