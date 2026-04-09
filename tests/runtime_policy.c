#include <glib.h>

#include <string.h>

#include "runtime.h"

static void test_choose_allowed_backends_prefers_wayland_when_available(void) {
    LaunchEnvironment env = {
        .xdg_session_type = "wayland",
        .gdk_backend_env = NULL,
        .wayland_display_env = "wayland-0",
    };

    g_assert_cmpstr(choose_allowed_backends(&env), ==, "wayland,x11,*");
}

static void test_choose_allowed_backends_returns_null_for_x11_session(void) {
    LaunchEnvironment env = {
        .xdg_session_type = "x11",
        .gdk_backend_env = NULL,
        .wayland_display_env = NULL,
    };

    g_assert_null(choose_allowed_backends(&env));
}

static void test_choose_allowed_backends_respects_explicit_backend(void) {
    LaunchEnvironment env = {
        .xdg_session_type = "wayland",
        .gdk_backend_env = "x11",
        .wayland_display_env = "wayland-0",
    };

    g_assert_null(choose_allowed_backends(&env));
}

static void test_should_show_warning_for_x11_inside_wayland(void) {
    LaunchEnvironment env = {
        .xdg_session_type = "wayland",
        .gdk_backend_env = NULL,
        .wayland_display_env = "wayland-0",
    };

    g_assert_true(should_show_performance_warning(&env, DISPLAY_BACKEND_X11));
}

static void test_should_not_show_warning_for_wayland_backend(void) {
    LaunchEnvironment env = {
        .xdg_session_type = "wayland",
        .gdk_backend_env = NULL,
        .wayland_display_env = "wayland-0",
    };

    g_assert_false(should_show_performance_warning(&env, DISPLAY_BACKEND_WAYLAND));
}

static void test_warning_message_mentions_forced_x11(void) {
    LaunchEnvironment env = {
        .xdg_session_type = "wayland",
        .gdk_backend_env = "x11",
        .wayland_display_env = "wayland-0",
    };
    gchar *message = build_performance_warning_message(&env, DISPLAY_BACKEND_X11);

    g_assert_nonnull(message);
    g_assert_nonnull(strstr(message, "GDK_BACKEND=x11"));
    g_free(message);
}

static void test_warning_message_mentions_x11_fallback(void) {
    LaunchEnvironment env = {
        .xdg_session_type = "wayland",
        .gdk_backend_env = NULL,
        .wayland_display_env = "wayland-0",
    };
    gchar *message = build_performance_warning_message(&env, DISPLAY_BACKEND_X11);

    g_assert_nonnull(message);
    g_assert_nonnull(strstr(message, "fell back to X11 for this run"));
    g_free(message);
}

int main(int argc, char **argv) {
    g_test_init(&argc, &argv, NULL);

    g_test_add_func(
        "/runtime/choose_allowed_backends/prefers_wayland_when_available",
        test_choose_allowed_backends_prefers_wayland_when_available
    );
    g_test_add_func(
        "/runtime/choose_allowed_backends/returns_null_for_x11_session",
        test_choose_allowed_backends_returns_null_for_x11_session
    );
    g_test_add_func(
        "/runtime/choose_allowed_backends/respects_explicit_backend",
        test_choose_allowed_backends_respects_explicit_backend
    );
    g_test_add_func(
        "/runtime/performance_warning/shows_for_x11_inside_wayland",
        test_should_show_warning_for_x11_inside_wayland
    );
    g_test_add_func(
        "/runtime/performance_warning/hides_for_wayland_backend",
        test_should_not_show_warning_for_wayland_backend
    );
    g_test_add_func(
        "/runtime/performance_warning/message_forced_x11",
        test_warning_message_mentions_forced_x11
    );
    g_test_add_func(
        "/runtime/performance_warning/message_x11_fallback",
        test_warning_message_mentions_x11_fallback
    );

    return g_test_run();
}
