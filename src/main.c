#define _POSIX_C_SOURCE 200809L

#include <GLES2/gl2.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <wayland-server-core.h>
#include <wlr/backend.h>
#include <wlr/render/allocator.h>
#include <wlr/render/gles2.h>
#include <wlr/render/wlr_renderer.h>
#include <wlr/render/wlr_texture.h>
#include <wlr/types/wlr_compositor.h>
#include <wlr/types/wlr_data_device.h>
#include <wlr/types/wlr_output.h>
#include <wlr/types/wlr_subcompositor.h>
#include <wlr/types/wlr_xdg_shell.h>
#include <wlr/util/log.h>

struct ctwl_server {
    struct wl_display *display;
    struct wlr_backend *backend;
    struct wlr_renderer *renderer;
    struct wlr_allocator *allocator;

    struct wlr_xdg_shell *xdg_shell;
    struct wl_listener new_xdg_toplevel;
    struct wl_listener new_xdg_popup;
    struct wl_list toplevels;

    struct wl_list outputs;
    struct wl_listener new_output;
};

struct ctwl_output {
    struct wl_list link;
    struct ctwl_server *server;
    struct wlr_output *wlr_output;
    struct wl_listener frame;
    struct wl_listener request_state;
    struct wl_listener destroy;
};

struct ctwl_toplevel {
    struct wl_list link;
    struct ctwl_server *server;
    struct wlr_xdg_toplevel *xdg_toplevel;

    struct wl_listener map;
    struct wl_listener unmap;
    struct wl_listener commit;
    struct wl_listener destroy;
};

// static void output_frame_notify(struct wl_listener *listener, void *data)
// {
//     struct ctwl_output *ctwl_output = wl_container_of(listener, ctwl_output,
//     frame); struct ctwl_server *server = ctwl_output->server; struct
//     wlr_output *wlr_output = ctwl_output->output;

//     struct timespec now;
//     clock_gettime(CLOCK_MONOTONIC, &now);
//     // wlr_log(WLR_INFO, "paint %d", now.tv_sec);

//     // DO PAINT
//     struct wlr_output_state state;
//     wlr_output_state_init(&state);
//     struct wlr_render_pass *pass = wlr_output_begin_render_pass(wlr_output,
//     &state, NULL, NULL); wlr_render_pass_add_rect(pass, &(struct
//     wlr_render_rect_options){
//                                        .box = {.width = wlr_output->width,
//                                        .height = wlr_output->height}, .color
//                                        = {
//                                            .r = ctwl_output->color[0],
//                                            .g = ctwl_output->color[1],
//                                            .b = ctwl_output->color[2],
//                                            .a = ctwl_output->color[3]}});
//     wlr_render_pass_submit(pass);
//     wlr_output_commit_state(wlr_output, &state);
//     wlr_output_state_finish(&state);
//     server->last_frame = now;
// }

// static void output_destroy_notify(struct wl_listener *listener, void *data)
// {
//     struct ctwl_output *ctwl_output = wl_container_of(listener, ctwl_output,
//     destroy); wlr_log(WLR_ERROR, "output removed");
//     wl_list_remove(&ctwl_output->frame.link);
//     wl_list_remove(&ctwl_output->destroy.link);
//     wl_list_remove(&ctwl_output->link);
//     free(ctwl_output);
// }

static void output_frame(struct wl_listener *listener, void *data) {
    struct ctwl_output *output = wl_container_of(listener, output, frame);
    struct ctwl_server *server = output->server;
    struct wlr_output *wlr_output = output->wlr_output;

    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);

    int width = wlr_output->width;
    int height = wlr_output->height;
    // wlr_log(WLR_DEBUG, "output frame %dx%d", width, height);

    struct wlr_output_state state;
    wlr_output_state_init(&state);
    struct wlr_render_pass *pass =
        wlr_output_begin_render_pass(wlr_output, &state, NULL, NULL);

    glClearColor(.05, .05, .05, 1.);
    glClear(GL_COLOR_BUFFER_BIT);

    struct ctwl_toplevel *t;
    wl_list_for_each(t, &server->toplevels, link) {
        struct wlr_surface *surface = t->xdg_toplevel->base->surface;
        struct wlr_texture *texture = wlr_surface_get_texture(surface);
        wlr_log(WLR_DEBUG, "isgles: %d", wlr_texture_is_gles2(texture));
        if (texture == NULL) {
            wlr_log(WLR_DEBUG, "NO TEX");
            continue;
        }
        wlr_render_pass_add_texture(
            pass,
            &(struct wlr_render_texture_options){
                .texture = t->xdg_toplevel->base->surface->buffer->texture});
        wlr_surface_send_frame_done(surface, &now);
    };

    wlr_render_pass_submit(pass);
    wlr_output_commit_state(wlr_output, &state);
    wlr_output_state_finish(&state);
}

static void server_new_output(struct wl_listener *listener, void *data) {
    struct wlr_output *wlr_output = data;
    struct ctwl_server *server = wl_container_of(listener, server, new_output);

    wlr_output_init_render(wlr_output, server->allocator, server->renderer);

    struct wlr_output_state state;
    wlr_output_state_init(&state);
    wlr_output_state_set_enabled(&state, true);

    struct wlr_output_mode *mode = wlr_output_preferred_mode(wlr_output);
    if (mode != NULL) {
        wlr_output_state_set_mode(&state, mode);
    }
    wlr_output_commit_state(wlr_output, &state);
    wlr_output_state_finish(&state);

    struct ctwl_output *output = calloc(1, sizeof(*output));
    output->wlr_output = wlr_output;
    output->server = server;

    output->frame.notify = output_frame;
    wl_signal_add(&wlr_output->events.frame, &output->frame);

    wl_list_insert(&server->outputs, &output->link);
}

static void xdg_toplevel_map(struct wl_listener *listener, void *data) {
    struct ctwl_toplevel *toplevel = wl_container_of(listener, toplevel, map);

    wl_list_insert(&toplevel->server->toplevels, &toplevel->link);
    wlr_log(WLR_INFO, "toplevel mapped!");
}

static void xdg_toplevel_unmap(struct wl_listener *listener, void *data) {
    struct ctwl_toplevel *toplevel = wl_container_of(listener, toplevel, unmap);
    wl_list_remove(&toplevel->link);
    wlr_log(WLR_INFO, "toplevel unmapped!");
}

static void xdg_toplevel_commit(struct wl_listener *listener, void *data) {
    struct ctwl_toplevel *toplevel =
        wl_container_of(listener, toplevel, commit);
    if (toplevel->xdg_toplevel->base->initial_commit) {
        wlr_log(WLR_INFO, "toplevel first commit");
        wlr_xdg_toplevel_set_size(toplevel->xdg_toplevel, 0, 0);
    }
}

static void xdg_toplevel_destroy(struct wl_listener *listener, void *data) {
    struct ctwl_toplevel *toplevel =
        wl_container_of(listener, toplevel, destroy);
    wlr_log(WLR_INFO, "toplevel destroyed");

    wl_list_remove(&toplevel->map.link);
    wl_list_remove(&toplevel->unmap.link);
    wl_list_remove(&toplevel->commit.link);
    wl_list_remove(&toplevel->destroy.link);
    free(toplevel);
}

static void server_new_xdg_toplevel(struct wl_listener *listener, void *data) {
    struct ctwl_server *server =
        wl_container_of(listener, server, new_xdg_toplevel);
    struct wlr_xdg_toplevel *xdg_toplevel = data;
    wlr_log(WLR_INFO, "new xdg toplevel");

    struct ctwl_toplevel *toplevel = calloc(1, sizeof(*toplevel));
    toplevel->server = server;
    toplevel->xdg_toplevel = xdg_toplevel;

    // Bind to xdg toplevel events
    toplevel->map.notify = xdg_toplevel_map;
    wl_signal_add(&xdg_toplevel->base->surface->events.map, &toplevel->map);
    toplevel->unmap.notify = xdg_toplevel_unmap;
    wl_signal_add(&xdg_toplevel->base->surface->events.unmap, &toplevel->unmap);
    toplevel->commit.notify = xdg_toplevel_commit;
    wl_signal_add(&xdg_toplevel->base->surface->events.commit,
                  &toplevel->commit);
    toplevel->destroy.notify = xdg_toplevel_destroy;
    wl_signal_add(&xdg_toplevel->events.destroy, &toplevel->destroy);
}

int main(int argc, char **argv) {
    wlr_log_init(WLR_DEBUG, NULL);
    wlr_log(WLR_ERROR, "HELLO MR PERSON");

    struct ctwl_server server = {0};

    // Create a Wayland display.
    server.display = wl_display_create();

    // Create a wlroots backend
    server.backend =
        wlr_backend_autocreate(wl_display_get_event_loop(server.display), NULL);
    if (!server.backend) {
        wlr_log(WLR_ERROR, "failed to create wlr_backend");
        return 1;
    }

    // Create a renderer
    server.renderer = wlr_renderer_autocreate(server.backend);
    if (!server.renderer) {
        wlr_log(WLR_ERROR, "failed to create wlr_renderer");
        return 1;
    }

    wlr_renderer_init_wl_display(server.renderer, server.display);

    // Create an allocator (handles buffer creation)
    server.allocator =
        wlr_allocator_autocreate(server.backend, server.renderer);
    if (!server.allocator) {
        wlr_log(WLR_ERROR, "failed to create wlr_allocator");
        return 1;
    }

    struct wlr_compositor *compositor =
        wlr_compositor_create(server.display, 5, server.renderer);
    wlr_subcompositor_create(server.display);
    wlr_data_device_manager_create(server.display);
    // compositor.

    // Bind to output event
    wl_list_init(&server.outputs);
    server.new_output.notify = server_new_output;
    wl_signal_add(&server.backend->events.new_output, &server.new_output);

    // Setup xdg-shell v3.
    wl_list_init(&server.toplevels);
    server.xdg_shell = wlr_xdg_shell_create(server.display, 3);
    server.new_xdg_toplevel.notify = server_new_xdg_toplevel;
    wl_signal_add(&server.xdg_shell->events.new_toplevel,
                  &server.new_xdg_toplevel);
    // server.new_xdg_popup.notify = server_new_xdg_popup;
    // wl_signal_add(&server.xdg_shell->events.new_popup,
    // &server.new_xdg_popup);

    // Add socket for WL server
    const char *socket = wl_display_add_socket_auto(server.display);
    if (!socket) {
        wlr_backend_destroy(server.backend);
        return 1;
    }

    // Start wayland backend.
    if (!wlr_backend_start(server.backend)) {
        wlr_log(WLR_ERROR, "failed to start backend");
        wlr_backend_destroy(server.backend);
        wl_display_destroy(server.display);
        return 1;
    }

    wlr_log(WLR_INFO, "Running Wayland compositor on WAYLAND_DISPLAY=%s",
            socket);

    // Start the WL display
    wl_display_run(server.display);

    // Cleanup
    wl_display_destroy_clients(server.display);
    wl_display_destroy(server.display);
}
