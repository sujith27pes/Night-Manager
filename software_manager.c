#include <gtk/gtk.h>
#include <vte/vte.h>
#include <stdlib.h>
#include <stdio.h>

GtkWidget *package_list;
GtkWidget *details_label;
GtkWidget *terminal;
GtkWidget *toggle_terminal_button;
gboolean terminal_visible = FALSE;

void on_package_selected(GtkTreeView *tree_view, GtkTreePath *path, GtkTreeViewColumn *col, gpointer user_data);
void on_install_clicked(GtkWidget *widget, gpointer data);
void on_uninstall_clicked(GtkWidget *widget, gpointer data);
void on_toggle_terminal_clicked(GtkWidget *widget, gpointer data);
void populate_package_list(GtkListStore *list_store);

void fetch_available_packages(GtkListStore *list_store) {
    FILE *fp;
    char output[1024];
    
    fp = popen("pacman -Slq", "r");
    if (fp == NULL) {
        perror("Failed to run pacman command");
        return;
    }

    GtkTreeIter iter;
    while (fgets(output, sizeof(output), fp) != NULL) {
        output[strcspn(output, "\n")] = 0; // Remove newline
        gtk_list_store_append(list_store, &iter);
        gtk_list_store_set(list_store, &iter, 0, output, -1);
    }
    pclose(fp);
}

void on_package_selected(GtkTreeView *tree_view, GtkTreePath *path, GtkTreeViewColumn *col, gpointer user_data) {
    GtkTreeModel *model;
    GtkTreeIter iter;
    char *selected_package;

    model = gtk_tree_view_get_model(tree_view);
    if (gtk_tree_model_get_iter(model, &iter, path)) {
        gtk_tree_model_get(model, &iter, 0, &selected_package, -1);
        
        gchar *details = g_strdup_printf("Package: %s\nVersion: 1.0.0\nSize: 10MB", selected_package);
        gtk_label_set_text(GTK_LABEL(details_label), details);
        g_free(details);
        g_free(selected_package);
    }
}




void on_install_clicked(GtkWidget *widget, gpointer data) {
    GtkTreeIter iter;
    GtkTreeModel *model;
    gchar *selected_package;

    // Get the selected package from the package list
    model = gtk_tree_view_get_model(GTK_TREE_VIEW(package_list));
    if (gtk_tree_selection_get_selected(gtk_tree_view_get_selection(GTK_TREE_VIEW(package_list)), &model, &iter)) {
        gtk_tree_model_get(model, &iter, 0, &selected_package, -1);
        
        // Construct the command to install the selected package
        gchar *command = g_strdup_printf("sudo pacman -S --noconfirm %s", selected_package);
        
        // Feed the command to the terminal (this will execute the command in the terminal)
        vte_terminal_feed_child(VTE_TERMINAL(terminal), command, strlen(command));

        // Free the allocated memory
        g_free(command);
        g_free(selected_package);
    }
}


void on_uninstall_clicked(GtkWidget *widget, gpointer data) {
    GtkTreeIter iter;
    GtkTreeModel *model;
    gchar *selected_package;

    // Get the selected package from the package list
    model = gtk_tree_view_get_model(GTK_TREE_VIEW(package_list));
    if (gtk_tree_selection_get_selected(gtk_tree_view_get_selection(GTK_TREE_VIEW(package_list)), &model, &iter)) {
        gtk_tree_model_get(model, &iter, 0, &selected_package, -1);
        
        // Construct the command to uninstall the selected package
        gchar *command = g_strdup_printf("sudo pacman -R --noconfirm %s", selected_package);
        
        // Feed the command to the terminal
        vte_terminal_feed_child(VTE_TERMINAL(terminal), command, strlen(command));

        // Free the allocated memory
        g_free(command);
        g_free(selected_package);
    }
}

// Toggle terminal visibility
void on_toggle_terminal_clicked(GtkWidget *widget, gpointer data) {
    terminal_visible = !terminal_visible;
    if (terminal_visible) {
        gtk_widget_show(terminal);
        gtk_button_set_label(GTK_BUTTON(toggle_terminal_button), "Hide Terminal");
    } else {
        gtk_widget_hide(terminal);
        gtk_button_set_label(GTK_BUTTON(toggle_terminal_button), "Show Terminal");
    }
}

int main(int argc, char *argv[]) {
    GtkWidget *window, *vbox, *scrolled_window, *button_box, *install_button, *uninstall_button;
    GtkListStore *list_store;
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;
    
    gtk_init(&argc, &argv);

    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Software Manager");
    gtk_window_set_default_size(GTK_WINDOW(window), 600, 400);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(window), vbox);

    scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_box_pack_start(GTK_BOX(vbox), scrolled_window, TRUE, TRUE, 0);

    // Package list
    list_store = gtk_list_store_new(1, G_TYPE_STRING);
    fetch_available_packages(list_store);  /

    package_list = gtk_tree_view_new_with_model(GTK_TREE_MODEL(list_store));
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("Available Packages", renderer, "text", 0, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(package_list), column);
    gtk_container_add(GTK_CONTAINER(scrolled_window), package_list);

    g_signal_connect(package_list, "row-activated", G_CALLBACK(on_package_selected), NULL);

    details_label = gtk_label_new("Select a package to view details.");
    gtk_box_pack_start(GTK_BOX(vbox), details_label, FALSE, FALSE, 0);


    button_box = gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_box_pack_start(GTK_BOX(vbox), button_box, FALSE, FALSE, 0);

    install_button = gtk_button_new_with_label("Install");
    g_signal_connect(install_button, "clicked", G_CALLBACK(on_install_clicked), NULL);
    gtk_container_add(GTK_CONTAINER(button_box), install_button);

    uninstall_button = gtk_button_new_with_label("Uninstall");
    g_signal_connect(uninstall_button, "clicked", G_CALLBACK(on_uninstall_clicked), NULL);
    gtk_container_add(GTK_CONTAINER(button_box), uninstall_button);

    // widget for terminal
    terminal = vte_terminal_new();
    vte_terminal_spawn_async(VTE_TERMINAL(terminal), VTE_PTY_DEFAULT, NULL, NULL, NULL, 0, NULL, NULL, NULL, -1, NULL, NULL, NULL);
    gtk_box_pack_start(GTK_BOX(vbox), terminal, TRUE, TRUE, 0);
    gtk_widget_hide(terminal);  // by default this is hidden

    // allowing users to toggle terminal based on tastes
    toggle_terminal_button = gtk_button_new_with_label("Show Terminal");
    g_signal_connect(toggle_terminal_button, "clicked", G_CALLBACK(on_toggle_terminal_clicked), NULL);
    gtk_box_pack_start(GTK_BOX(vbox), toggle_terminal_button, FALSE, FALSE, 0);

    gtk_widget_show_all(window);
    gtk_main();

    return 0;
}
