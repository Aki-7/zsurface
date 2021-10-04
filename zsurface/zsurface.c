#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <wayland-client.h>
#include <z11-client-protocol.h>
#include <z11-input-client-protocol.h>
#include <z11-opengl-client-protocol.h>
#include <zsurface.h>

#include "internal.h"

static enum zsurface_error zsurface_error = ZSURFACE_ERR_NONE;

enum zsurface_error zsurface_get_error() { return zsurface_error; }

union fixed_flt {
  wl_fixed_t fixed;
  float flt;
};

static void shm_format(
    void* data, struct wl_shm* wl_shm, enum wl_shm_format format)
{
  struct zsurface* surface = data;
  UNUSED(surface);
  UNUSED(wl_shm);
  UNUSED(format);
}

static const struct wl_shm_listener shm_listener = {
    .format = shm_format,
};

static void ray_enter(void* data, struct z11_ray* ray, uint32_t serial,
    struct z11_cuboid_window* cuboid_window, wl_fixed_t ray_origin_x,
    wl_fixed_t ray_origin_y, wl_fixed_t ray_origin_z,
    wl_fixed_t ray_direction_x, wl_fixed_t ray_direction_y,
    wl_fixed_t ray_direction_z)
{
  union fixed_flt origin_x, origin_y, origin_z, direction_x, direction_y,
      direction_z;

  origin_x.fixed = ray_origin_x;
  origin_y.fixed = ray_origin_y;
  origin_z.fixed = ray_origin_z;
  direction_x.fixed = ray_direction_x;
  direction_y.fixed = ray_direction_y;
  direction_z.fixed = ray_direction_z;

  struct zsurface* surface = data;
  UNUSED(surface);
  UNUSED(ray);
  UNUSED(serial);
  UNUSED(cuboid_window);
  fprintf(stderr, "[enter] (%f, %f, %f) - (%f, %f, %f)\n", origin_x.flt,
      origin_y.flt, origin_z.flt, direction_x.flt, direction_y.flt,
      direction_z.flt);
}

static void ray_motion(void* data, struct z11_ray* ray, uint32_t time,
    wl_fixed_t ray_origin_x, wl_fixed_t ray_origin_y, wl_fixed_t ray_origin_z,
    wl_fixed_t ray_direction_x, wl_fixed_t ray_direction_y,
    wl_fixed_t ray_direction_z)
{
  union fixed_flt origin_x, origin_y, origin_z, direction_x, direction_y,
      direction_z;

  origin_x.fixed = ray_origin_x;
  origin_y.fixed = ray_origin_y;
  origin_z.fixed = ray_origin_z;
  direction_x.fixed = ray_direction_x;
  direction_y.fixed = ray_direction_y;
  direction_z.fixed = ray_direction_z;

  struct zsurface* surface = data;
  UNUSED(surface);
  UNUSED(ray);
  UNUSED(time);
  fprintf(stderr, "[motion] (%f, %f, %f) - (%f, %f, %f)\n", origin_x.flt,
      origin_y.flt, origin_z.flt, direction_x.flt, direction_y.flt,
      direction_z.flt);
}

static void ray_leave(void* data, struct z11_ray* ray, uint32_t serial,
    struct z11_cuboid_window* cuboid_window)
{
  struct zsurface* surface = data;
  UNUSED(surface);
  UNUSED(ray);
  UNUSED(serial);
  UNUSED(cuboid_window);
}

static void ray_button(void* data, struct z11_ray* ray, uint32_t serial,
    uint32_t time, uint32_t button, uint32_t state)
{
  struct zsurface* surface = data;
  UNUSED(surface);
  UNUSED(ray);
  UNUSED(serial);
  UNUSED(time);
  UNUSED(button);
  UNUSED(state);
}

static const struct z11_ray_listener ray_listener = {
    .enter = ray_enter,
    .motion = ray_motion,
    .leave = ray_leave,
    .button = ray_button,
};

static void seat_capability(
    void* data, struct z11_seat* seat, uint32_t capabilities)
{
  UNUSED(seat);
  UNUSED(ray_listener);
  struct zsurface* surface = data;

  // if (capabilities & Z11_SEAT_CAPABILITY_RAY) {
  //   surface->ray = z11_seat_get_ray(seat);
  //   z11_ray_add_listener(surface->ray, &ray_listener, surface);
  // } else {
  //   if (surface->ray) z11_ray_destroy(surface->ray);
  //   surface->ray = NULL;
  // }

  // TODO: Handle keyboard

  if (surface->interface->seat_capability)
    surface->interface->seat_capability(surface, surface->data, capabilities);
}

static const struct z11_seat_listener seat_listener = {
    .capability = seat_capability,
};

static void zsurface_global_registry_handler(void* data,
    struct wl_registry* registry, uint32_t id, const char* interface,
    uint32_t version)
{
  struct zsurface* surface = data;

  if (strcmp(interface, "z11_compositor") == 0) {
    surface->compositor =
        wl_registry_bind(registry, id, &z11_compositor_interface, version);
  } else if (strcmp(interface, "wl_shm") == 0) {
    surface->shm = wl_registry_bind(registry, id, &wl_shm_interface, version);
    wl_shm_add_listener(surface->shm, &shm_listener, surface);
  } else if (strcmp(interface, "z11_opengl") == 0) {
    surface->gl =
        wl_registry_bind(registry, id, &z11_opengl_interface, version);
  } else if (strcmp(interface, "z11_opengl_render_component_manager") == 0) {
    surface->render_component_manager = wl_registry_bind(
        registry, id, &z11_opengl_render_component_manager_interface, version);
  } else if (strcmp(interface, "z11_shell") == 0) {
    surface->shell =
        wl_registry_bind(registry, id, &z11_shell_interface, version);
  } else if (strcmp(interface, "z11_seat") == 0) {
    surface->seat =
        wl_registry_bind(registry, id, &z11_seat_interface, version);
    z11_seat_add_listener(surface->seat, &seat_listener, surface);
  }
}

static void zsurface_global_registry_remover(
    void* data, struct wl_registry* registry, uint32_t id)
{
  UNUSED(data);
  UNUSED(registry);
  UNUSED(id);
}

static const struct wl_registry_listener registry_listener = {
    .global = zsurface_global_registry_handler,
    .global_remove = zsurface_global_registry_remover,
};

struct zsurface* zsurface_create(
    const char* socket, void* data, struct zsurface_interface* interface)
{
  struct zsurface* surface;

  surface = zalloc(sizeof *surface);
  if (surface == NULL) goto out;

  surface->interface = interface;
  surface->data = data;
  surface->toplevel = NULL;

  surface->display = wl_display_connect(socket);
  if (surface->display == NULL) goto out_surface;

  surface->registry = wl_display_get_registry(surface->display);
  if (surface->registry == NULL) goto out_display;

  wl_registry_add_listener(surface->registry, &registry_listener, surface);

  return surface;

out_display:
  wl_display_disconnect(surface->display);

out_surface:
  free(surface);

out:
  return NULL;
}

void zsurface_destroy(struct zsurface* surface)
{
  if (surface->toplevel) zsurface_toplevel_destroy(surface->toplevel);
  wl_display_disconnect(surface->display);  // all wayland object will be freed
  free(surface);
}

int zsurface_check_globals(struct zsurface* surface)
{
  wl_display_roundtrip(surface->display);

  if (surface->compositor && surface->gl && surface->shm &&
      surface->render_component_manager && surface->shell && surface->seat) {
    return 0;
  }
  return -1;
}

struct zsurface_view* zsurface_create_toplevel_view(
    struct zsurface* surface, struct zsurface_toplevel_option option)
{
  if (surface->toplevel != NULL) {
    zsurface_toplevel_destroy(surface->toplevel);
  }
  surface->toplevel = zsurface_toplevel_create(surface, option);
  if (surface->toplevel == NULL) return NULL;

  return surface->toplevel->view;
}

void zsurface_remove_toplevel_view(struct zsurface* surface)
{
  if (surface->toplevel != NULL) {
    zsurface_toplevel_destroy(surface->toplevel);
    surface->toplevel = NULL;
  }
}

struct zsurface_view* zsurface_get_toplevel_view(struct zsurface* surface)
{
  if (surface->toplevel) return surface->toplevel->view;
  return NULL;
}

void zsurface_run(struct zsurface* surface)
{
  while (1) {
    while (wl_display_prepare_read(surface->display) == -1) {
      if (errno != EAGAIN) return;
      if (wl_display_dispatch_pending(surface->display) == -1) return;
    }

    while (wl_display_flush(surface->display) == -1) {
      if (errno != EAGAIN) {
        wl_display_cancel_read(surface->display);
        return;
      };
    }

    if (wl_display_read_events(surface->display) == -1) return;

    if (wl_display_dispatch_pending(surface->display) == -1) return;
  }
}
