#include <stdio.h>
#include <stdlib.h>
#include <zsurface.h>

typedef struct {
  struct zsurface* surface;
  struct zsurface_toplevel* toplevel;
  struct zsurface_view* enter_view;
  float view_width, view_height;
  uint32_t texture_width, texture_height;
  uint32_t pointer_x, pointer_y;
  int has_pointer;
} App;

static void seat_capability(
    void* data, struct zsurface* surface, uint32_t capability)
{
  App* app = data;
  (void)app;
  (void)surface;
  (void)capability;
}

static void pointer_leave(void* data)
{
  App* app = data;
  app->has_pointer = 0;
  app->enter_view = NULL;
}

static void pointer_enter(
    void* data, struct zsurface_view* view, float view_x, float view_y)
{
  App* app = data;
  app->has_pointer = 1;
  app->pointer_x = view_x * 10;
  app->pointer_y = view_y * 10;
  app->enter_view = view;
}

static void pointer_motion(void* data, float view_x, float view_y)
{
  App* app = data;
  app->pointer_x = view_x * 10;
  app->pointer_y = view_y * 10;
}

static struct zsurface_interface interface = {
    .seat_capability = seat_capability,
    .pointer_leave = pointer_leave,
    .pointer_enter = pointer_enter,
    .pointer_motion = pointer_motion,
};

static App* app_create(uint32_t width, uint32_t height)
{
  App* app;
  struct zsurface_toplevel_option option;

  app = calloc(1, sizeof *app);
  if (app == NULL) goto out;
  app->texture_width = width;
  app->texture_height = height;
  app->view_width = (float)width / 10.0;
  app->view_height = (float)height / 10.0;
  app->has_pointer = 0;

  option.width = app->view_width;
  option.height = app->view_height;
  app->surface = zsurface_create("z11-0", app, &interface);
  if (app->surface == NULL) goto out_app;

  if (zsurface_check_globals(app->surface) != 0) goto out_surface;

  app->toplevel = zsurface_create_toplevel_view(app->surface, option);
  if (app->toplevel == NULL) goto out_surface;

  zsurface_view_resize_texture(zsurface_toplevel_get_view(app->toplevel),
      app->texture_width, app->texture_height);

  app->enter_view = NULL;

  return app;

out_surface:
  zsurface_destroy(app->surface);

out_app:
  free(app);

out:
  return NULL;
}

void app_paint(App* app)
{
  uint32_t cursor = 0;
  struct zsurface_view* view = zsurface_toplevel_get_view(app->toplevel);
  struct zsurface_color_bgra* data = zsurface_view_get_texture_data(view);

  for (uint32_t y = 0; y < app->texture_height; y++) {
    for (uint32_t x = 0; x < app->texture_width; x++) {
      uint32_t r = (x - app->pointer_x) * (x - app->pointer_x) +
                   (y - app->pointer_y) * (y - app->pointer_y);
      if (app->has_pointer && r < 256 && app->enter_view == view) {
        data[cursor].a = UINT8_MAX;
        data[cursor].b = 3;
        data[cursor].g = 3;
        data[cursor].r = 3;
      } else {
        data[cursor].a = UINT8_MAX;
        data[cursor].b =
            data[cursor].b < UINT8_MAX ? data[cursor].b + 4 : UINT8_MAX;
        data[cursor].g =
            data[cursor].g < UINT8_MAX ? data[cursor].g + 4 : UINT8_MAX;
        data[cursor].r =
            data[cursor].r < UINT8_MAX ? data[cursor].r + 4 : UINT8_MAX;
      }
      cursor++;
    }
  }
}

void app_next_frame(void* data, uint32_t callback_time)
{
  (void)callback_time;
  App* app = data;
  struct zsurface_view* view = zsurface_toplevel_get_view(app->toplevel);
  app_paint(app);
  zsurface_view_add_frame_callback(view, app_next_frame, app);
  zsurface_view_commit(view);
}

int main()
{
  App* app = app_create(512, 256);
  if (app == NULL) return EXIT_FAILURE;
  struct zsurface_view* view = zsurface_toplevel_get_view(app->toplevel);
  app_paint(app);
  zsurface_view_add_frame_callback(view, app_next_frame, app);
  zsurface_view_commit(view);
  zsurface_run(app->surface);
  return 0;
}
