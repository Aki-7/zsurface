#ifndef ZSURFACE_INTERNAL
#define ZSURFACE_INTERNAL

#include <stdlib.h>
#include <wayland-client.h>
#include <wl_zext_client.h>
#include <z11-client-protocol.h>
#include <z11-opengl-client-protocol.h>
#include <zsurface.h>

/* util */

#define UNUSED(x) ((void)x)

inline void* zalloc(size_t size) { return calloc(1, size); }

void zsurface_log(const char* fmt, ...);

/* types */

struct point {
  float x, y, z;
};

struct uv_cood {
  float u, v;
};

struct vertex {
  struct point point;
  struct uv_cood uv;
};

/* zsurface view */

struct zsurface_toplevel;

struct view_triangle {
  struct vertex vetices[3];
};

struct view_rect {
  struct view_triangle triangles[2];
};

struct zsurface_view {
  struct zsurface_toplevel* toplevel;

  float width;
  float height;

  int fd;
  void* shm_data;
  size_t shm_data_len;
  struct wl_shm_pool* pool;

  struct z11_opengl_render_component* render_component;

  struct z11_opengl_vertex_buffer* vertex_buffer;
  struct wl_zext_raw_buffer* vertex_raw_buffer;
  struct view_rect* vertex_data;

  struct z11_opengl_shader_program* shader;

  struct z11_opengl_texture_2d* texture;
  struct wl_zext_raw_buffer* texture_raw_buffer;
  struct zsurface_color_bgra* texture_data;
  uint32_t texture_width;
  uint32_t texture_height;
};

void zsurface_view_resize(
    struct zsurface_view* view, float width, float height);

struct zsurface_view* zsurface_view_create(
    struct zsurface_toplevel* toplevel, float width, float height);

void zsurface_view_destroy(struct zsurface_view* view);

/* top level view */

struct zsurface_toplevel {
  struct zsurface* surface;
  struct zsurface_view* view;

  struct z11_virtual_object* virtual_object;
  struct z11_cuboid_window* cuboid_window;
};

struct zsurface_toplevel* zsurface_toplevel_create(
    struct zsurface* surface, struct zsurface_toplevel_option option);

void zsurface_toplevel_destroy(struct zsurface_toplevel* toplevel);

/* zsurface */

struct zsurface {
  struct zsurface_interface* interface;
  void* data;
  struct zsurface_toplevel* toplevel;  // nullable

  struct wl_display* display;
  struct wl_registry* registry;
  struct z11_compositor* compositor;
  struct wl_shm* shm;
  struct z11_opengl* gl;
  struct z11_opengl_render_component_manager* render_component_manager;
  struct z11_shell* shell;
  struct z11_seat* seat;
  struct z11_ray* ray;  // nullable
};

#endif  //  ZSURFACE_INTERNAL
