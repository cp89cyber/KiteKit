#include <gtk/gtk.h>
#include <webkit2/webkit2.h>

#include "runtime.h"

#include <string.h>

typedef struct {
    GtkWidget *window;
    GtkWidget *uri_entry;
    GtkWidget *back_button;
    GtkWidget *forward_button;
    GtkWidget *reload_button;
    GtkWidget *performance_warning_bar;
    GtkWidget *performance_warning_label;
    WebKitWebView *web_view;
} BrowserUi;

static gboolean has_prefix_case_insensitive(const gchar *text, const gchar *prefix) {
    gsize prefix_length = strlen(prefix);

    return g_ascii_strncasecmp(text, prefix, prefix_length) == 0;
}

static gchar *normalize_uri(const gchar *input) {
    static const gchar *known_schemes[] = {
        "http://",
        "https://",
        "file://",
        "about:",
        "data:",
    };

    gchar *trimmed;
    guint index;

    if (input == NULL) {
        return NULL;
    }

    trimmed = g_strdup(input);
    g_strstrip(trimmed);

    if (trimmed[0] == '\0') {
        g_free(trimmed);
        return NULL;
    }

    for (index = 0; index < G_N_ELEMENTS(known_schemes); index++) {
        if (has_prefix_case_insensitive(trimmed, known_schemes[index])) {
            return trimmed;
        }
    }

    {
        gchar *normalized = g_strdup_printf("https://%s", trimmed);
        g_free(trimmed);
        return normalized;
    }
}

static void update_navigation_buttons(BrowserUi *ui) {
    gtk_widget_set_sensitive(ui->back_button, webkit_web_view_can_go_back(ui->web_view));
    gtk_widget_set_sensitive(ui->forward_button, webkit_web_view_can_go_forward(ui->web_view));
}

static LaunchEnvironment get_launch_environment(void) {
    LaunchEnvironment env = {
        .xdg_session_type = g_getenv("XDG_SESSION_TYPE"),
        .gdk_backend_env = g_getenv("GDK_BACKEND"),
        .wayland_display_env = g_getenv("WAYLAND_DISPLAY"),
    };

    return env;
}

static const gchar *hardware_acceleration_policy_to_string(WebKitHardwareAccelerationPolicy policy) {
    switch (policy) {
        case WEBKIT_HARDWARE_ACCELERATION_POLICY_ON_DEMAND:
            return "on-demand";
        case WEBKIT_HARDWARE_ACCELERATION_POLICY_ALWAYS:
            return "always";
        case WEBKIT_HARDWARE_ACCELERATION_POLICY_NEVER:
            return "never";
        default:
            return "unknown";
    }
}

static void configure_web_view(WebKitWebView *web_view) {
    WebKitSettings *settings = webkit_web_view_get_settings(web_view);

    webkit_settings_set_enable_smooth_scrolling(settings, TRUE);
    webkit_settings_set_enable_2d_canvas_acceleration(settings, TRUE);
}

static void log_runtime_diagnostics(WebKitWebView *web_view, const LaunchEnvironment *env, DisplayBackendKind actual_backend) {
    WebKitSettings *settings = webkit_web_view_get_settings(web_view);

    g_debug(
        "startup runtime: XDG_SESSION_TYPE=%s GDK_BACKEND=%s WAYLAND_DISPLAY=%s actual_backend=%s "
        "smooth_scrolling=%d canvas_2d_acceleration=%d hardware_acceleration_policy=%s",
        env->xdg_session_type != NULL ? env->xdg_session_type : "(unset)",
        env->gdk_backend_env != NULL ? env->gdk_backend_env : "(unset)",
        env->wayland_display_env != NULL ? env->wayland_display_env : "(unset)",
        display_backend_kind_to_string(actual_backend),
        webkit_settings_get_enable_smooth_scrolling(settings),
        webkit_settings_get_enable_2d_canvas_acceleration(settings),
        hardware_acceleration_policy_to_string(webkit_settings_get_hardware_acceleration_policy(settings))
    );
}

static void update_performance_warning(BrowserUi *ui) {
    LaunchEnvironment env = get_launch_environment();
    DisplayBackendKind actual_backend = detect_display_backend(gtk_widget_get_display(ui->window));
    gchar *warning_message;

    warning_message = build_performance_warning_message(&env, actual_backend);
    log_runtime_diagnostics(ui->web_view, &env, actual_backend);

    if (warning_message == NULL) {
        gtk_widget_hide(ui->performance_warning_bar);
        return;
    }

    gtk_label_set_text(GTK_LABEL(ui->performance_warning_label), warning_message);
    gtk_widget_show(ui->performance_warning_bar);
    g_warning("%s", warning_message);
    g_free(warning_message);
}

static void update_window_title(BrowserUi *ui) {
    const gchar *page_title = webkit_web_view_get_title(ui->web_view);
    const gchar *uri = webkit_web_view_get_uri(ui->web_view);
    gchar *window_title;

    if (page_title != NULL && page_title[0] != '\0') {
        window_title = g_strdup_printf("%s - KiteKit Browser", page_title);
    } else if (uri != NULL && uri[0] != '\0') {
        window_title = g_strdup(uri);
    } else {
        window_title = g_strdup("KiteKit Browser");
    }

    gtk_window_set_title(GTK_WINDOW(ui->window), window_title);
    g_free(window_title);
}

static void load_uri_from_input(BrowserUi *ui, const gchar *input) {
    gchar *normalized = normalize_uri(input);

    if (normalized == NULL) {
        return;
    }

    gtk_entry_set_text(GTK_ENTRY(ui->uri_entry), normalized);
    webkit_web_view_load_uri(ui->web_view, normalized);
    g_free(normalized);
}

static void on_entry_activate(GtkEntry *entry, gpointer user_data) {
    BrowserUi *ui = user_data;

    load_uri_from_input(ui, gtk_entry_get_text(entry));
}

static void on_back_clicked(GtkButton *button, gpointer user_data) {
    BrowserUi *ui = user_data;

    (void)button;
    webkit_web_view_go_back(ui->web_view);
}

static void on_forward_clicked(GtkButton *button, gpointer user_data) {
    BrowserUi *ui = user_data;

    (void)button;
    webkit_web_view_go_forward(ui->web_view);
}

static void on_reload_clicked(GtkButton *button, gpointer user_data) {
    BrowserUi *ui = user_data;

    (void)button;
    webkit_web_view_reload(ui->web_view);
}

static void on_uri_changed(WebKitWebView *web_view, GParamSpec *pspec, gpointer user_data) {
    BrowserUi *ui = user_data;
    const gchar *uri = webkit_web_view_get_uri(web_view);

    (void)pspec;
    gtk_entry_set_text(GTK_ENTRY(ui->uri_entry), uri != NULL ? uri : "");
    update_window_title(ui);
}

static void on_title_changed(WebKitWebView *web_view, GParamSpec *pspec, gpointer user_data) {
    BrowserUi *ui = user_data;

    (void)web_view;
    (void)pspec;
    update_window_title(ui);
}

static void on_load_changed(WebKitWebView *web_view, WebKitLoadEvent load_event, gpointer user_data) {
    BrowserUi *ui = user_data;

    (void)web_view;
    (void)load_event;
    update_navigation_buttons(ui);
    update_window_title(ui);
}

static void on_navigation_capability_changed(WebKitWebView *web_view, GParamSpec *pspec, gpointer user_data) {
    BrowserUi *ui = user_data;

    (void)web_view;
    (void)pspec;
    update_navigation_buttons(ui);
}

static void on_performance_warning_response(GtkInfoBar *info_bar, gint response_id, gpointer user_data) {
    (void)response_id;
    (void)user_data;
    gtk_widget_hide(GTK_WIDGET(info_bar));
}

static void browser_ui_free(BrowserUi *ui) {
    g_free(ui);
}

static void activate(GtkApplication *app, gpointer user_data) {
    GList *windows = gtk_application_get_windows(app);
    GtkWindow *existing_window = windows != NULL ? GTK_WINDOW(windows->data) : NULL;
    const gchar *startup_uri = g_object_get_data(G_OBJECT(app), "startup-uri");
    BrowserUi *ui;
    GtkWidget *content_box;
    GtkWidget *toolbar;
    GtkWidget *performance_warning_content;
    GtkWidget *dismiss_button;

    (void)user_data;

    if (existing_window != NULL) {
        BrowserUi *existing_ui = g_object_get_data(G_OBJECT(existing_window), "browser-ui");

        if (startup_uri != NULL && existing_ui != NULL) {
            load_uri_from_input(existing_ui, startup_uri);
        }

        gtk_window_present(existing_window);
        return;
    }

    content_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    toolbar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    ui = g_new0(BrowserUi, 1);
    ui->window = gtk_application_window_new(app);
    ui->back_button = gtk_button_new_with_label("Back");
    ui->forward_button = gtk_button_new_with_label("Forward");
    ui->reload_button = gtk_button_new_with_label("Reload");
    ui->uri_entry = gtk_entry_new();
    ui->web_view = WEBKIT_WEB_VIEW(webkit_web_view_new());
    ui->performance_warning_bar = gtk_info_bar_new();
    ui->performance_warning_label = gtk_label_new(NULL);

    gtk_window_set_default_size(GTK_WINDOW(ui->window), 1200, 800);
    gtk_window_set_title(GTK_WINDOW(ui->window), "KiteKit Browser");

    gtk_widget_set_margin_start(toolbar, 6);
    gtk_widget_set_margin_end(toolbar, 6);
    gtk_widget_set_margin_top(toolbar, 6);
    gtk_widget_set_margin_bottom(toolbar, 6);

    gtk_entry_set_placeholder_text(GTK_ENTRY(ui->uri_entry), "Enter URL");
    gtk_widget_set_hexpand(ui->uri_entry, TRUE);
    gtk_label_set_xalign(GTK_LABEL(ui->performance_warning_label), 0.0f);
    gtk_label_set_line_wrap(GTK_LABEL(ui->performance_warning_label), TRUE);
    gtk_info_bar_set_message_type(GTK_INFO_BAR(ui->performance_warning_bar), GTK_MESSAGE_WARNING);
    dismiss_button = gtk_info_bar_add_button(
        GTK_INFO_BAR(ui->performance_warning_bar),
        "Dismiss",
        GTK_RESPONSE_CLOSE
    );
    performance_warning_content = gtk_info_bar_get_content_area(GTK_INFO_BAR(ui->performance_warning_bar));
    gtk_container_add(GTK_CONTAINER(performance_warning_content), ui->performance_warning_label);
    gtk_widget_set_no_show_all(ui->performance_warning_bar, TRUE);
    gtk_widget_show(ui->performance_warning_label);
    gtk_widget_show(dismiss_button);
    configure_web_view(ui->web_view);

    gtk_box_pack_start(GTK_BOX(toolbar), ui->back_button, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(toolbar), ui->forward_button, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(toolbar), ui->reload_button, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(toolbar), ui->uri_entry, TRUE, TRUE, 0);

    gtk_box_pack_start(GTK_BOX(content_box), toolbar, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(content_box), ui->performance_warning_bar, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(content_box), GTK_WIDGET(ui->web_view), TRUE, TRUE, 0);

    gtk_container_add(GTK_CONTAINER(ui->window), content_box);

    g_object_set_data(G_OBJECT(ui->window), "browser-ui", ui);
    g_signal_connect_swapped(ui->window, "destroy", G_CALLBACK(browser_ui_free), ui);
    g_signal_connect(ui->uri_entry, "activate", G_CALLBACK(on_entry_activate), ui);
    g_signal_connect(ui->back_button, "clicked", G_CALLBACK(on_back_clicked), ui);
    g_signal_connect(ui->forward_button, "clicked", G_CALLBACK(on_forward_clicked), ui);
    g_signal_connect(ui->reload_button, "clicked", G_CALLBACK(on_reload_clicked), ui);
    g_signal_connect(ui->web_view, "notify::uri", G_CALLBACK(on_uri_changed), ui);
    g_signal_connect(ui->web_view, "notify::title", G_CALLBACK(on_title_changed), ui);
    g_signal_connect(ui->web_view, "load-changed", G_CALLBACK(on_load_changed), ui);
    g_signal_connect(ui->web_view, "notify::can-go-back", G_CALLBACK(on_navigation_capability_changed), ui);
    g_signal_connect(ui->web_view, "notify::can-go-forward", G_CALLBACK(on_navigation_capability_changed), ui);
    g_signal_connect(
        ui->performance_warning_bar,
        "response",
        G_CALLBACK(on_performance_warning_response),
        ui
    );

    update_navigation_buttons(ui);
    update_window_title(ui);
    update_performance_warning(ui);

    gtk_widget_show_all(ui->window);
    load_uri_from_input(ui, startup_uri != NULL ? startup_uri : "https://example.com");
}

static int on_command_line(GApplication *application, GApplicationCommandLine *command_line, gpointer user_data) {
    int argc;
    gchar **argv = g_application_command_line_get_arguments(command_line, &argc);

    (void)user_data;
    g_object_set_data_full(
        G_OBJECT(application),
        "startup-uri",
        argc > 1 ? g_strdup(argv[1]) : NULL,
        g_free
    );
    g_application_activate(application);
    g_strfreev(argv);

    return 0;
}

int main(int argc, char *argv[]) {
    GtkApplication *app;
    LaunchEnvironment env = get_launch_environment();
    const gchar *allowed_backends = choose_allowed_backends(&env);
    int status;

    if (allowed_backends != NULL) {
        gdk_set_allowed_backends(allowed_backends);
    }

    app = gtk_application_new("dev.kitekit.Browser", G_APPLICATION_HANDLES_COMMAND_LINE);

    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    g_signal_connect(app, "command-line", G_CALLBACK(on_command_line), NULL);
    status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);

    return status;
}
