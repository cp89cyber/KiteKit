#ifndef KITEKIT_RUNTIME_H
#define KITEKIT_RUNTIME_H

#include <gdk/gdk.h>

typedef enum {
    DISPLAY_BACKEND_UNKNOWN,
    DISPLAY_BACKEND_X11,
    DISPLAY_BACKEND_WAYLAND,
} DisplayBackendKind;

typedef struct {
    const gchar *xdg_session_type;
    const gchar *gdk_backend_env;
    const gchar *wayland_display_env;
} LaunchEnvironment;

const gchar *choose_allowed_backends(const LaunchEnvironment *env);
DisplayBackendKind detect_display_backend(GdkDisplay *display);
const gchar *display_backend_kind_to_string(DisplayBackendKind kind);
gboolean is_wayland_session(const LaunchEnvironment *env);
gboolean should_show_performance_warning(const LaunchEnvironment *env, DisplayBackendKind actual_backend);
gchar *build_performance_warning_message(const LaunchEnvironment *env, DisplayBackendKind actual_backend);

#endif
