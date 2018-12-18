/*
    Copyright (C) 2018  Jacek Danecki <jacek.m.danecki@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "timer.h"
#include "window.h"
#include <gtk/gtk.h>
#include <math.h>
#include <stdlib.h>

unsigned long render_time, render_times, avg, fps;
unsigned long tp1, tp2;
int i;

static gboolean draw_cb(GtkWidget* widget, cairo_t* cr, gpointer data)
{
    GdkPixbuf* pix;
    guchar* pix_data;

    int window_width = gtk_widget_get_allocated_width(widget);
    int window_height = gtk_widget_get_allocated_height(widget);

#if 0
    printf("width: %d\n", window_width);
    printf("height: %d\n", window_height);
#endif
    int w, h, stride;

    w = window_width > WIDTH ? WIDTH : window_width;
    h = window_height > HEIGHT ? HEIGHT : window_height;

    pix = gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, 8, w, h);
    pix_data = gdk_pixbuf_get_pixels(pix);

    w = gdk_pixbuf_get_width(pix);
    h = gdk_pixbuf_get_height(pix);
    stride = gdk_pixbuf_get_rowstride(pix);
#if 0 
    printf("pix width: %d\n", w);
    printf("pix height: %d\n", h);
    printf("pix stride: %d\n", stride);
#endif
    for (int y = 0; y < h; y++)
    {
        for (int x = 0; x < w; x++)
        {
            int r, g, b;
            r = 0x80 + 0x7f * sin(6.28 * x / w);
            g = 0x80 + 0x7f * sin(6.28 * y / h);
            b = 0x80 + 0x7f * cos(6.28 * (x + y) / (w + h));

            pix_data[stride * y + 3 * x] = r;
            pix_data[stride * y + 3 * x + 1] = g;
            pix_data[stride * y + 3 * x + 2] = b;
        }
    }

    tp1 = get_time_usec();
    gdk_cairo_set_source_pixbuf(cr, pix, 0, 0);
    cairo_paint(cr);
    tp2 = get_time_usec();

    avg = i ? render_times / i : 0;
    fps = avg ? 1000000 / avg : 0;
    render_time = tp2 - tp1;
    render_times += render_time;
    i++;
    printf("test gtk[%d]: render time=%lu avg=%lu fps=%lu\n", i, render_time, avg, fps);

#if 0
    cairo_surface_t * surf;
    surf = cairo_get_target(cr);
    
    cairo_surface_write_to_png(surf,"test_gtk.png");
#endif
    g_object_unref(pix);
    gtk_widget_queue_draw(widget);

    return TRUE;
}

int main(int argc, char** argv)
{
    GtkWidget* window;
    GtkWidget* area;

    gtk_init(&argc, &argv);

    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_widget_set_size_request(window, WIDTH, HEIGHT);

    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    area = gtk_drawing_area_new();
    gtk_container_add(GTK_CONTAINER(window), area);

    g_signal_connect(area, "draw", (GCallback)draw_cb, NULL);

    gtk_widget_set_app_paintable(area, TRUE);
    gtk_widget_show_all(window);

    gtk_main();

    return 0;
}
