#include <gtk/gtk.h>

extern GtkWidget *mainwindow, *tabs;
extern GtkBuilder *main_builder;

// ------------------------Make the history height not be limited by the conversion tab------------------------
static void on_switch_page(GtkNotebook *nb, GtkWidget *page, guint page_num, gpointer user_data) {
	int n = gtk_notebook_get_n_pages(nb);
	for (int i = 0; i < n; i++) {
		GtkWidget *child = gtk_notebook_get_nth_page(nb, i);
		if (i == (int)page_num) gtk_widget_show(child);
		else gtk_widget_hide(child);
	}
}

static void history_no_fixed_height()
{
	g_signal_connect(tabs, "switch-page", G_CALLBACK(on_switch_page), NULL);

	// Force it to run once
	GtkNotebook *nb = GTK_NOTEBOOK(tabs);
	guint page_num = (guint)gtk_notebook_get_current_page(nb);
	on_switch_page(nb, gtk_notebook_get_nth_page(nb, page_num), page_num, NULL);
}

// ----------------------------No title bar and move window with middle mouse click----------------------------
static gboolean on_window_button_press(GtkWidget *w, GdkEventButton *e, gpointer user_data) {
	if (e->type == GDK_BUTTON_PRESS && e->button == 2) {
		gtk_window_begin_move_drag(GTK_WINDOW(w), e->button, e->x_root, e->y_root, e->time);
		return TRUE;
	}
	return FALSE;
}

static void no_titlebar_middle_click_move()
{
	gtk_widget_add_events(mainwindow, GDK_BUTTON_PRESS_MASK);
	g_signal_connect(mainwindow, "button-press-event", G_CALLBACK(on_window_button_press), NULL);

	GtkWidget *hb = gtk_header_bar_new();
	gtk_header_bar_set_show_close_button(GTK_HEADER_BAR(hb), FALSE);
	gtk_window_set_titlebar(GTK_WINDOW(mainwindow), hb);
}

// ---------------------------Menu bar appears on alt and disappears when done using---------------------------
typedef struct {
	GtkWidget *menubar;
	gboolean alt_down;
	gboolean alt_used_with_other_key;
} MenuAltState;
static MenuAltState *g_menu_state = NULL;

static void hide_menubar(MenuAltState *s) {
	gtk_widget_hide(s->menubar);
	s->alt_down = FALSE;
	s->alt_used_with_other_key = FALSE;
}

static gboolean on_key_press(GtkWidget *w, GdkEventKey *e, gpointer user_data) {
	if (e->keyval == GDK_KEY_Alt_L || e->keyval == GDK_KEY_Alt_R) gtk_widget_show(((MenuAltState*)user_data)->menubar);
	return FALSE;
}

static gboolean on_key_release(GtkWidget *w, GdkEventKey *e, gpointer user_data) {
	if (e->keyval != GDK_KEY_Alt_L && e->keyval != GDK_KEY_Alt_R) return FALSE;

	MenuAltState *s = (MenuAltState*)user_data;
	gboolean focus_menu = !s->alt_used_with_other_key;
	s->alt_down = s->alt_used_with_other_key = FALSE;
	if (!focus_menu) hide_menubar(s);
	else {
		gtk_menu_shell_select_first(GTK_MENU_SHELL(s->menubar), TRUE);
		gtk_widget_grab_focus(s->menubar);
	}

	return focus_menu;
}

static void gdk_event_tap(GdkEvent *event, gpointer data)
{
	if (g_menu_state && (event->type == GDK_KEY_PRESS || event->type == GDK_KEY_RELEASE)) {
		GdkEventKey *ke = (GdkEventKey*)event;
		bool is_alt=(ke->keyval == GDK_KEY_Alt_L || ke->keyval == GDK_KEY_Alt_R);
		if(is_alt) g_menu_state->alt_down = event->type == GDK_KEY_PRESS;
		if(event->type == GDK_KEY_PRESS) g_menu_state->alt_used_with_other_key = !is_alt && g_menu_state->alt_down;
	}
	gtk_main_do_event(event);
}

static void on_menubar_deactivate(GtkMenuShell *shell, gpointer user_data) {
	hide_menubar((MenuAltState*)user_data);
}

static void setup_toggle_menu_bar()
{
	MenuAltState *s = g_menu_state = g_new0(MenuAltState, 1);
	s->menubar = GTK_WIDGET(gtk_builder_get_object(main_builder, "menubar"));
	gtk_widget_hide(s->menubar);

	g_signal_connect(mainwindow, "key-press-event",   G_CALLBACK(on_key_press),   s);
	g_signal_connect(mainwindow, "key-release-event", G_CALLBACK(on_key_release), s);
	gdk_event_handler_set(gdk_event_tap, NULL, NULL);
	g_signal_connect(s->menubar, "deactivate", G_CALLBACK(on_menubar_deactivate), s);
}

// -----------------------------------------------------Initialize-----------------------------------------------------
static gboolean run_after_startup(gpointer) {
	history_no_fixed_height();
	no_titlebar_middle_click_move();
	setup_toggle_menu_bar();

	return G_SOURCE_REMOVE;
}

__attribute__((constructor)) static void inject_ctor() {
	 g_idle_add_full(G_PRIORITY_DEFAULT_IDLE, run_after_startup, nullptr, nullptr);
}