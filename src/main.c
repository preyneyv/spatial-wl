#define _POSIX_C_SOURCE 200809L

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <wayland-server-core.h>
#include <wlr/util/log.h>
#include <wlr/backend.h>
#include <wlr/types/wlr_output.h>
#include <wlr/render/allocator.h>
#include <wlr/render/wlr_renderer.h>

struct c_tinywl_server
{
    struct wl_display *display;
    struct wl_listener new_output;
    struct wlr_renderer *renderer;
    struct wlr_allocator *allocator;
    struct timespec last_frame;
    struct wl_list outputs;
    float color[4];
    int dec;
};

struct ctwl_output
{
    struct wl_list link;
    struct c_tinywl_server *server;
    struct wlr_output *output;
    struct wl_listener frame;
    struct wl_listener destroy;
    float color[4];
};

static void output_frame_notify(struct wl_listener *listener, void *data)
{
    struct ctwl_output *ctwl_output = wl_container_of(listener, ctwl_output, frame);
    struct c_tinywl_server *server = ctwl_output->server;
    struct wlr_output *wlr_output = ctwl_output->output;

    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    // wlr_log(WLR_INFO, "paint %d", now.tv_sec);

    // DO PAINT
    struct wlr_output_state state;
    wlr_output_state_init(&state);
    struct wlr_render_pass *pass = wlr_output_begin_render_pass(wlr_output, &state, NULL, NULL);
    wlr_render_pass_add_rect(pass, &(struct wlr_render_rect_options){
                                       .box = {.width = wlr_output->width, .height = wlr_output->height},
                                       .color = {
                                           .r = ctwl_output->color[0],
                                           .g = ctwl_output->color[1],
                                           .b = ctwl_output->color[2],
                                           .a = ctwl_output->color[3]}});
    wlr_render_pass_submit(pass);
    wlr_output_commit_state(wlr_output, &state);
    wlr_output_state_finish(&state);
    server->last_frame = now;
}

static void output_destroy_notify(struct wl_listener *listener, void *data)
{
    struct ctwl_output *ctwl_output = wl_container_of(listener, ctwl_output, destroy);
    wlr_log(WLR_ERROR, "output removed");
    wl_list_remove(&ctwl_output->frame.link);
    wl_list_remove(&ctwl_output->destroy.link);
    wl_list_remove(&ctwl_output->link);
    free(ctwl_output);
}

static void new_output_notify(struct wl_listener *listener, void *data)
{
    struct wlr_output *output = data;
    struct c_tinywl_server *server = wl_container_of(listener, server, new_output);

    wlr_output_init_render(output, server->allocator, server->renderer);

    // Construct a holding struct.
    struct ctwl_output *ctwl_output = calloc(1, sizeof(*ctwl_output));
    ctwl_output->output = output;
    ctwl_output->server = server;

    // Set up a frame render listener
    ctwl_output->frame.notify = output_frame_notify;
    wl_signal_add(&output->events.frame, &ctwl_output->frame);

    // Set up a destroy event listener
    ctwl_output->destroy.notify = output_destroy_notify;
    wl_signal_add(&output->events.destroy, &ctwl_output->destroy);

    wl_list_insert(&server->outputs, &ctwl_output->link);

    wlr_log(WLR_ERROR, "acquired output name: '%s' non_desktop: %d\n", output->name, output->non_desktop);
    int w = output->width;
    wlr_log(WLR_ERROR, "new output w %d", w);

    struct wlr_output_state state;
    wlr_output_state_init(&state);
    wlr_output_state_set_enabled(&state, true);
    struct wlr_output_mode *mode = wlr_output_preferred_mode(output);
    if (mode != NULL)
    {
        wlr_output_state_set_mode(&state, mode);
    }
    wlr_output_commit_state(output, &state);
    wlr_output_state_finish(&state);
}

int main(int argc, char **argv)
{
    wlr_log_init(WLR_DEBUG, NULL);
    wlr_log(WLR_ERROR, "HELLO MR PERSON");

    // Create a Wayland display.
    struct wl_display *display = wl_display_create();

    struct c_tinywl_server server = {
        .color = {1., 0., 0., 1.},
        .dec = 0,
        .last_frame = {0},
        .display = display,
    };

    wl_list_init(&server.outputs);

    // Create a wlroots backend
    struct wlr_backend *backend = wlr_backend_autocreate(wl_display_get_event_loop(display), NULL);
    if (!backend)
    {
        wlr_log(WLR_ERROR, "failed to create wlr_backend");
        return 1;
    }

    server.renderer = wlr_renderer_autocreate(backend);
    server.allocator = wlr_allocator_autocreate(backend, server.renderer);

    // Bind to output event
    wl_signal_add(&backend->events.new_output, &server.new_output);
    server.new_output.notify = new_output_notify;

    clock_gettime(CLOCK_MONOTONIC, &server.last_frame);

    if (!wlr_backend_start(backend))
    {
        wlr_log(WLR_ERROR, "failed to start backend");
        wlr_backend_destroy(backend);
        return 1;
    }

    wlr_log(WLR_ERROR, "start time: %d\n", server.last_frame.tv_nsec);
    // Start the WL display
    wl_display_run(display);

    // Cleanup
    wl_display_destroy(display);
}
