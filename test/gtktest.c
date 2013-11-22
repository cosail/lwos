/*
 test whether the gtk devel library is ok
 compile:
 	gcc gtktest.c -o gtkmain `pkg-config --cflags --libs gtk+-2.0`
*/
#include <gtk/gtk.h>

int main(int argc, char* argv[])
{
	GtkWidget * window;
	gtk_init(&argc, &argv);
	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_widget_show(window);
	gtk_main();

	return 0;
}
