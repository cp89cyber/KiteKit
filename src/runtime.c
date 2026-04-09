#include "runtime.h"

#include <string.h>

static gboolean string_has_text(const gchar *value) {
    return value != NULL && value[0] != '\0';
}

static gboolean has_prefix_case_insensitive(const gchar *text, const gchar *prefix) {
    gsize prefix_length;

    if (!string_has_text(text) || !string_has_text(prefix)) {
        return FALSE;
    }

    prefix_length = strlen(prefix);

    return g_ascii_strncasecmp(text, prefix, prefix_length) == 0;
}

const gchar *choose_allowed_backends(const LaunchEnvironment *env) {
    if (env == NULL) {
        return NULL;
    }

    if (string_has_text(env->gdk_backend_env)) {
        return NULL;
    }

    if (is_wayland_session(env)) {
        return "wayland,x11,*";
    }

    return NULL;
}

DisplayBackendKind detect_display_backend(GdkDisplay *display) {
    const gchar *type_name;

    if (display == NULL) {
        return DISPLAY_BACKEND_UNKNOWN;
    }

    type_name = G_OBJECT_TYPE_NAME(display);

    if (g_strcmp0(type_name, "GdkX11Display") == 0) {
        return DISPLAY_BACKEND_X11;
    }

    if (g_strcmp0(type_name, "GdkWaylandDisplay") == 0) {
        return DISPLAY_BACKEND_WAYLAND;
    }

    return DISPLAY_BACKEND_UNKNOWN;
}

const gchar *display_backend_kind_to_string(DisplayBackendKind kind) {
    switch (kind) {
        case DISPLAY_BACKEND_X11:
            return "x11";
        case DISPLAY_BACKEND_WAYLAND:
            return "wayland";
        case DISPLAY_BACKEND_UNKNOWN:
        default:
            return "unknown";
    }
}

gboolean is_wayland_session(const LaunchEnvironment *env) {
    if (env == NULL) {
        return FALSE;
    }

    if (string_has_text(env->wayland_display_env)) {
        return TRUE;
    }

    return string_has_text(env->xdg_session_type)
        && g_ascii_strcasecmp(env->xdg_session_type, "wayland") == 0;
}

gboolean should_show_performance_warning(const LaunchEnvironment *env, DisplayBackendKind actual_backend) {
    return is_wayland_session(env) && actual_backend == DISPLAY_BACKEND_X11;
}

gchar *build_performance_warning_message(const LaunchEnvironment *env, DisplayBackendKind actual_backend) {
    GString *message;

    if (!should_show_performance_warning(env, actual_backend)) {
        return NULL;
    }

    message = g_string_new(
        "KiteKit is running through X11 inside a Wayland session, which can make "
        "scrolling and compositing feel less smooth."
    );

    if (env != NULL && string_has_text(env->gdk_backend_env)) {
        if (has_prefix_case_insensitive(env->gdk_backend_env, "x11")) {
            g_string_append(
                message,
                " Relaunch without forcing GDK_BACKEND=x11 to let GTK use Wayland when available."
            );
        } else {
            g_string_append(
                message,
                " GTK selected X11 for this run based on the current backend configuration."
            );
        }
    } else {
        g_string_append(message, " GTK fell back to X11 for this run.");
    }

    return g_string_free(message, FALSE);
}
