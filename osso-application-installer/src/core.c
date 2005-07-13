/** 
    @file core.c

    Core functionality of Application Installer.
    <p>
*/

/*
 * This file is part of osso-application-installer
 *
 * Copyright (C) 2005 Nokia Corporation.
 *
 * Contact: Marius Vollmer <marius.vollmer@nokia.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
 */

#include "core.h"

gint dpkg_wrapper(const char *cmdline, char **cmd_stdout, 
		  char **cmd_stderr, gboolean wrap_it)
{
  if (!cmdline) return OSSO_INVALID;

  GString *wrapper = g_string_new("");
  if (wrap_it == TRUE)
    wrapper = g_string_append(wrapper, "/usr/bin/aiwrapper ");
  wrapper = g_string_append(wrapper, cmdline);

  ULOG_DEBUG("Wrapper: '%s'", wrapper->str);
  //fprintf(stderr, "Wrapper: '%s'\n", wrapper->str);

  setlocale(LC_ALL, "C");
  g_spawn_command_line_sync(wrapper->str, 
			    cmd_stdout, cmd_stderr, 
			    NULL, NULL);
  setlocale(LC_ALL, "");

  return OSSO_OK;
}



DpkgOutput use_dpkg(guint method, gchar **args, AppData *app_data)
{
  int i = 0;
  gchar **package_items = NULL;
  DpkgOutput output = { NULL, NULL, NULL };
  ListItems list_items;
  GString *cmdline = NULL;
  gchar *cmd_stdout = NULL, *cmd_stderr = NULL;
  gchar **buffer = NULL;

  if (!args) return output;

  cmdline = g_string_new("");
  
  ULOG_DEBUG("use_dpkg: started, method %d\n", method);
  ULOG_DEBUG("(1=list, 2=install, 3=uninstall, 4=fields, 5=status, 6=list built-in)\n");
  if ( (0 != method) && (0 < sizeof(args)) ) {
    g_assert(method);
    g_assert(args);

    output.out = g_string_new("");
    output.err = g_string_new("");
    output.list = g_array_new(FALSE, TRUE, sizeof(ListItems));

    i = 0;
    while (args[i] != NULL) {
      if (i > 0) cmdline = g_string_append(cmdline, " ");
      cmdline = g_string_append(cmdline, args[i++]);
    }

    /* app_data is != null only at install/uninstall */
    if ( (method == DPKG_METHOD_INSTALL) ||
	 (method == DPKG_METHOD_UNINSTALL) ) {
      ui_set_progressbar(app_data->app_ui_data, 0.33, method);

      /* This is disabled in pulse() for now */
      if (method == DPKG_METHOD_INSTALL)
	ui_pulse_progressbar(app_data->app_ui_data, 0.40);
      
      ui_forcedraw();
    }

    ULOG_INFO("use_dpkg: cmdline '%s'", cmdline->str);
    if ( (method == DPKG_METHOD_INSTALL) ||
	 (method == DPKG_METHOD_UNINSTALL) ) {
      dpkg_wrapper(cmdline->str, &cmd_stdout, &cmd_stderr, TRUE);
    } else {
      dpkg_wrapper(cmdline->str, &cmd_stdout, &cmd_stderr, FALSE);
    }


    /* removing progressbar updating timer after done */
    if ( (method == DPKG_METHOD_INSTALL) ||
	 (method == DPKG_METHOD_UNINSTALL) ) {
      ui_set_progressbar(app_data->app_ui_data, 0.80, method);
      ui_forcedraw();
    }

    /* Too much spam from built-in, even for debug */
    ULOG_DEBUG("cmd: '%s'\n", cmdline->str);
    if (method != DPKG_METHOD_LIST_BUILTIN) {
      ULOG_DEBUG("out: '%s'\n", cmd_stdout);
      ULOG_DEBUG("err: '%s'\n", cmd_stderr);
    }

    /* Were done with spawn, now parsing our outputs */
    ULOG_DEBUG("use_dpkg: preparing outputs\n");
    if (cmd_stdout == NULL && cmd_stderr == NULL) {
      ULOG_DEBUG("****************************************\n");
      ULOG_DEBUG("* FAKEROOT MISSING, UNABLE TO CONTINUE *\n");
      ULOG_DEBUG("****************************************\n");
    }
    g_assert(cmd_stdout != NULL);
    g_assert(cmd_stderr != NULL);
    
    output.out = g_string_new(cmd_stdout);
    output.err = g_string_new(cmd_stderr);

    
    /* If cmd_stderr includes "cannot access archive", its MMC cover open */
    /* or "failed to read archive" */
    if ( (0 < strlen(cmd_stderr)) && 
	 ((0 != g_strrstr(cmd_stderr, DPKG_STATUS_INSTALL_ERROR)) ||
	  (0 != g_strrstr(cmd_stderr, DPKG_STATUS_UNINSTALL_ERROR)) ||
	  (0 != g_strrstr(cmd_stderr, DPKG_ERROR_MEMORYFULL)) ||
	  (0 != g_strrstr(cmd_stderr, DPKG_ERROR_MEMORYFULL2)) ||
	  (0 != g_strrstr(cmd_stderr, DPKG_ERROR_CANNOT_ACCESS)) ||
	  (0 != g_strrstr(cmd_stderr, DPKG_ERROR_NO_SUCH_FILE))) ) {
      ULOG_DEBUG("use_dpkg: mmc/memoryfull error.. dialog follows\n");
      return output;
    }

    ULOG_DEBUG("use_dpkg: going to stdout read loop\n");
    buffer = g_strsplit(cmd_stdout, "\n", 0);
    i = 0;
    while (buffer[i] != NULL) {
      /* break on empty line */
      if (0 == g_strcasecmp(g_strstrip(buffer[i]), "")) { 
	ULOG_INFO("use_dpkg: breaking on empty line at line %d\n", i+1);
	break; 
      }
      
      if (DPKG_METHOD_LIST == method) {
	/* Split out of the string which is formed as below: */
	/* package-name version size */
	package_items = g_strsplit(buffer[i], " ", 0);
	list_items.name = strdup(package_items[0]);
	list_items.version = strdup(package_items[1]);
	list_items.size = strdup(package_items[2]);
	g_array_append_val(output.list, list_items);
      }

      i++;
    }
    
    /* Terminate list with NULL */
    if (DPKG_METHOD_LIST == method) {
      list_items.name = list_items.version = list_items.size = NULL;
      g_array_append_val(output.list, list_items);
    }
    
    g_strfreev(buffer);
  }

  ULOG_DEBUG("use_dpkg: finished method '%d'\n", method);
  return output;
}



GtkTreeModel *list_packages(AppUIData *app_ui_data)
{
  gint i = 0;
  GtkListStore *store;
  GtkTreeIter iter;
  ListItems *package;
  DpkgOutput output;
  gchar *args[5];

  /* Dpkg query params for needed information */
  args[0] = DPKG_QUERY_BINARY;
  args[1] = "-W";
  args[2] = "--admindir=/var/lib/install/var/lib/dpkg";
  args[3] = "--showformat=\\${Package}\\ \\${Version}\\ \\${Installed-Size}\\\\n";
  args[4] = (gchar *) NULL;

  output = use_dpkg(DPKG_METHOD_LIST, args, NULL);

  store = gtk_list_store_new (NUM_COLUMNS,
			      G_TYPE_OBJECT,
                              G_TYPE_STRING,
                              G_TYPE_STRING,
                              G_TYPE_STRING);

  /* No packages installed */
  if ( ((package = &g_array_index(output.list, ListItems, 1)) == NULL) || 
       (package->name == NULL) ) {

    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter,
		       COLUMN_ICON, NULL,
		       COLUMN_NAME, _("ai_ti_application_installer_nopackages"),
		       COLUMN_VERSION, "",
		       COLUMN_SIZE, "",
		       -1
		       );
    return GTK_TREE_MODEL(store);
  }


  /* Some packages installed */
  for (i = 0; (package = &g_array_index(output.list, ListItems, i)) &&
       (NULL != package->name); i++) {
    if (0 != g_strcasecmp(package->name, META_PACKAGE)) {
      GString *version = g_string_append(g_string_new("v"), package->version);
      GString *size = g_string_append(g_string_new(package->size), "kB");

      /* Load icon */
      GdkPixbuf *icon = NULL;
      icon = gtk_icon_theme_load_icon(gtk_icon_theme_get_default(),
				      "qgn_list_gene_default_app",
				      26,
				      0,
				      NULL);

      gtk_list_store_append(store, &iter);
      gtk_list_store_set(store, &iter,
			 COLUMN_ICON, icon, 
                         COLUMN_NAME, package->name,
                         COLUMN_VERSION, version->str,
                         COLUMN_SIZE, size->str,
                         -1);

      if (app_ui_data != NULL) {
	app_ui_data->max_name = MAX(strlen(package->name), app_ui_data->max_name);      
	app_ui_data->max_version = MAX(strlen(version->str), app_ui_data->max_version);      
	app_ui_data->max_size = MAX(strlen(size->str), app_ui_data->max_size);
      }

      /* Release icon space */
      g_object_unref(icon);
    }
  }

  if (app_ui_data != NULL) {
    ULOG_DEBUG("max name: %d\n", app_ui_data->max_name);
    ULOG_DEBUG("max size: %d\n", app_ui_data->max_size);
    ULOG_DEBUG("max vers: %d\n", app_ui_data->max_version);
  }

  return GTK_TREE_MODEL(store);
}



const gchar *list_builtin_packages(void) {
  DpkgOutput output;
  gchar *args[4];

  /* Dpkg query params for needed information */
  args[0] = DPKG_QUERY_BINARY;
  args[1] = "-W";
  args[2] = "--showformat=\\${Package}\\ ";
  args[3] = (gchar *) NULL;

  output = use_dpkg(DPKG_METHOD_LIST_BUILTIN, args, NULL);

  return output.out->str;
}



void add_columns(AppUIData *app_ui_data, GtkTreeView *treeview)
{
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;
 
  /* Icon column */
  renderer = gtk_cell_renderer_pixbuf_new();
  column = gtk_tree_view_column_new_with_attributes("Icon",
						    renderer,
						    "pixbuf",
						    COLUMN_ICON,
						    NULL);
  gtk_tree_view_column_set_sort_column_id(column, COLUMN_ICON);
  gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_FIXED);
  gtk_tree_view_column_set_fixed_width(column, COLUMN_ICON_WIDTH);
  gtk_tree_view_append_column(treeview, column);

  /* Name column */
  renderer = gtk_cell_renderer_text_new();
  column = gtk_tree_view_column_new_with_attributes("Name",
                                                    renderer,
                                                    "text",
                                                    COLUMN_NAME,
                                                    NULL);
  gtk_tree_view_column_set_sort_column_id(column, COLUMN_NAME);
  gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_GROW_ONLY);
  gtk_tree_view_append_column(treeview, column);
  app_ui_data->name_column = column;


  /* Version column */
  renderer = gtk_cell_renderer_text_new();
  column = gtk_tree_view_column_new_with_attributes("Version",
                                                    renderer,
                                                    "text",
                                                    COLUMN_VERSION,
                                                    NULL);
  gtk_tree_view_column_set_sort_column_id(column, COLUMN_VERSION);
  gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_GROW_ONLY);
  gtk_tree_view_append_column(treeview, column);
  app_ui_data->version_column = column;


  /* Size column */
  renderer = gtk_cell_renderer_text_new();
  column = gtk_tree_view_column_new_with_attributes("Size",
                                                    renderer,
                                                    "text",
                                                    COLUMN_SIZE,
                                                    NULL);
  gtk_tree_view_column_set_sort_column_id(column, COLUMN_SIZE);
  gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_GROW_ONLY);
  gtk_tree_view_append_column(treeview, column);
  app_ui_data->size_column = column;
}



PackageInfo package_info(gchar *package_or_deb, gint installed)
{
  gchar *dpkg_args[10];
  gchar *name = NULL, *size = NULL, *description = NULL, *version = NULL;
  //gchar *maintainer = NULL, *depends = NULL;
  gchar **row, **namerow, **sizerow, **descriptionrow, **versionrow;
  //gchar **maintainerrow, **dependsrow;
  DpkgOutput output;
  PackageInfo info;

  /* Nullify the struct */
  info.name = NULL;
  info.size = NULL;
  info.description = NULL;
  info.version = NULL;
  //info.depends = NULL;
  //info.maintainer = NULL;

  if (package_or_deb == NULL) {
    ULOG_DEBUG("Package_or_deb empty, returning NULL\n");
    return info;
  }

  /* Information from .deb file */
  if (installed == DPKG_INFO_PACKAGE) {
    dpkg_args[0] = DPKG_BINARY;
    dpkg_args[1] = "-f";
    dpkg_args[2] = package_or_deb;
    dpkg_args[3] = "Package";
    dpkg_args[4] = "Version";
    //dpkg_args[5] = "Maintainer";
    //dpkg_args[6] = "Depends";
    dpkg_args[5] = "Installed-Size";
    dpkg_args[6] = "Description";
    dpkg_args[7] = (gchar *)NULL;

    /* Send command to dpkg */
    output = use_dpkg(DPKG_METHOD_FIELDS, dpkg_args, NULL);


    /* If invalid files */
    if ( (0 != g_strrstr(output.err->str, DEB_FORMAT_FAILURE)) ||
	 (0 != g_strrstr(output.err->str, DEB_FORMAT_FAILURE2)) || 
	 (0 != g_strrstr(output.err->str, DPKG_ERROR_CANNOT_ACCESS)) ||
	 (0 != g_strrstr(output.err->str, DPKG_ERROR_NO_SUCH_FILE)) ) {
      ULOG_DEBUG("pkg_info: invalid format\n");
      return info;
    }

    /* Split rows from the output */
    row = g_strsplit(output.out->str, "\n", 4);
    /* Split fields from the row */
    namerow = g_strsplit(row[0], ":", 2);
    versionrow = g_strsplit(row[1], ":", 2);
    //dependsrow = g_strsplit(row[2], ":", 2);
    sizerow = g_strsplit(row[2], ":", 2);
    //maintainerrow = g_strsplit(row[4], ":", 2);
    descriptionrow = g_strsplit(row[3], ":", 2);
    /* Strip empty chars */
    name = g_strstrip(g_strdup(namerow[1]));
    version = g_strstrip(g_strdup(versionrow[1]));
    size = g_strstrip(g_strdup(sizerow[1]));
    description = g_strstrip(g_strdup(descriptionrow[1]));
    //maintainer = g_strstrip(g_strdup(maintainerrow[1]));
    //depends = g_strstrip(g_strdup(dependsrow[1]));

    /* Free string arrays */
    g_strfreev(row);
    g_strfreev(namerow);
    g_strfreev(versionrow);
    g_strfreev(sizerow);
    g_strfreev(descriptionrow);
    //g_strfreev(maintainerrow);
    //g_strfreev(dependsrow);
  }

  /* Information from installed package */
  else {
    if (0 == strcmp("", package_or_deb)) {
      return info;
    }

    dpkg_args[0] = DPKG_QUERY_BINARY;
    dpkg_args[1] = "--status";
    dpkg_args[2] = "--admindir=/var/lib/install/var/lib/dpkg";
    dpkg_args[3] = package_or_deb;
    dpkg_args[4] = (gchar *)NULL;

    output = use_dpkg(DPKG_METHOD_STATUS, dpkg_args, NULL);
  
    if ( (0 != g_strrstr(output.err->str, DPKG_NO_SUCH_PACKAGE)) ||
	 (0 != g_strrstr(output.err->str, DPKG_NO_SUCH_PACKAGE2)) ||
	 (0 != g_strrstr(output.err->str, DPKG_NO_SUCH_PACKAGE3)) ) {
      ULOG_DEBUG("No such package '%s' installed\n", package_or_deb);
      return info;
    }

    //fprintf(stderr, "********* OUT **********\n");
    //fprintf(stderr, "%s", output.err->str);
    //fprintf(stderr, "********* END **********\n");
    

    /* Lets dig up pointers to strings we need */
    gchar *version_ptr = g_strrstr(output.out->str, DPKG_FIELD_VERSION);
    gchar *size_ptr = g_strrstr(output.out->str, DPKG_FIELD_SIZE);
    gchar *desc_ptr = g_strrstr(output.out->str, DPKG_FIELD_DESCRIPTION);
    //gchar *maint_ptr = g_strrstr(output.out->str, DPKG_FIELD_MAINTAINER);
    //gchar *deps_ptr = g_strrstr(output.out->str, DPKG_FIELD_DEPENDS);

    g_assert(version_ptr != NULL);
    g_assert(size_ptr != NULL);
    g_assert(desc_ptr != NULL);
    //g_assert(maint_ptr != NULL);
    //g_assert(deps_ptr != NULL);

    /* Split them nicely to components we can use */
    versionrow = g_strsplit(version_ptr, ": ", 2);
    sizerow = g_strsplit_set(size_ptr, ":\n", 2);
    descriptionrow = g_strsplit(desc_ptr, ": ", 2);
    //maintainerrow = g_strsplit(maint_ptr, ": ", 2);
    //dependsrow = g_strsplit(deps_ptr, ": ", 2);

    /* And take a copy of them for us */
    name = package_or_deb;
    size = g_strstrip(g_strdup(sizerow[1]));
    version = g_strstrip(g_strdup(versionrow[1]));
    description = g_strstrip(g_strdup(descriptionrow[1]));
    //maintainer = g_strstrip(g_strdup(maintainerrow[1]));
    //depends = g_strstrip(g_strdup(dependsrow[1]));

    /* Free the arrays */
    g_strfreev(sizerow);
    g_strfreev(descriptionrow);
    g_strfreev(versionrow);
    //g_strfreev(maintainerrow);
    //g_strfreev(dependsrow);
  }

  /* Construct the info package */
  info.name = g_string_new(name);
  info.size = g_string_new(size);
  info.version = g_string_new(version);
  info.description = g_string_new(description);
  //info.maintainer = g_string_new(description);
  //info.depends = g_string_new(description);

  return info;
}
  


/* Are there any packages installed (except maemo) */
gboolean any_packages_installed(GtkTreeModel *model) 
{
  GtkTreeIter iter;
  gboolean true_if_any = FALSE;
  gchar *size = NULL;

  /* Lets get a list of packages and its first entry */
  if (model == NULL) 
    model = list_packages(NULL);
  true_if_any = gtk_tree_model_get_iter_first(model, &iter);

  /* If entry size is empty, its "No Packages" text */
  gtk_tree_model_get(model, &iter, COLUMN_SIZE, &size, -1);
  if (0 == strcmp("", size))
    true_if_any = FALSE;
  g_free(size);

  ULOG_INFO("any_installed: List had items: %s", 
	    (true_if_any ? "true" : "false"));
  return true_if_any;
}



gchar *show_description(gchar *package, gint installed)
{
  PackageInfo info = package_info(package, installed);

  if (info.description != NULL) {
    return info.description->str;
  }

  return "";
}



gchar *show_remove_dependencies(gchar *package)
{
  gchar *dpkg_args[8];
  DpkgOutput output;
  GString *deps = g_string_new("");

  /* We can get dependencies only by --simulating --remove */
  dpkg_args[0] = FAKEROOT_BINARY;
  dpkg_args[1] = DPKG_BINARY;
  dpkg_args[2] = "--simulate";
  dpkg_args[3] = "--force-not-root";
  dpkg_args[4] = "--root=/var/lib/install";
  dpkg_args[5] = "--remove";
  dpkg_args[6] = package;
  dpkg_args[7] = (gchar *) NULL;

  output = use_dpkg(DPKG_METHOD_STATUS, dpkg_args, NULL);
  
  /* If we have erroneous output, it probably means dependency issues */
  if (0 < output.err->len) {

    /* If the whole err has even one "depends on", we have problem */
    if (0 != g_strrstr(output.err->str, DPKG_ERROR_DEPENDENCY)) {
      gint i = 0;
      gchar **errors = g_strsplit_set(output.err->str, " \n", 0);
      GString *prev = NULL;

      /* Checking all lines for dependent packages */
      for (i=0; errors[i] != NULL; i++) {
	GString *this = g_string_new(errors[i]);

	/* Trying to find lines matching to this: */
	/*  example: 
	    file-roller depends on bzip2 (>= 1.0.1). */
	if (this->len > 0 && 0 == g_strcasecmp(this->str, "depends")) {
	  deps = g_string_append(deps, prev->str);
	  deps = g_string_append(deps, " ");

	} else {
	  prev = g_string_new(this->str);
	}
      }
      
      g_strfreev(errors);
    }
  }

  return deps->str;
}



gboolean install_package(gchar *deb, AppData *app_data) 
{
  gchar *install_args[12];
  GString *tmp = g_string_new("");

  /* Start installation if deb package is given */
  if (0 != g_strcasecmp(deb, "")) {
    ULOG_INFO("Started installing '%s'..", deb);

    /* Install command */
    install_args[0] = FAKEROOT_BINARY;
    install_args[1] = DPKG_BINARY;
    install_args[2] = "--force-not-root";
    install_args[3] = "--root=/var/lib/install";
    install_args[4] = "--refuse-depends";
    install_args[5] = "--status-fd";
    install_args[6] = "1";
    install_args[7] = "--abort-after";
    install_args[8] = "1";
    install_args[9] = "--install";
    install_args[10] = deb;
    install_args[11] = (gchar *) NULL;

    PackageInfo deb_info = package_info(deb, DPKG_INFO_PACKAGE);

    /* If package is built-in show error and quit */
    tmp = g_string_assign(tmp, deb_info.name->str);
    tmp = g_string_append(tmp, " ");
    if (NULL != g_strstr_len(app_data->app_ui_data->builtin_packages,
        strlen(app_data->app_ui_data->builtin_packages), tmp->str)) {      

      /* Installation failed, do you wanna see details */
      if (represent_confirmation(app_data, DIALOG_SHOW_DETAILS,
          g_strdup_printf(_("ai_ti_installation_failed_text"),
          deb_info.name->str))) {

        /* Show error to user */
        represent_error(app_data, NULL,
         g_strdup_printf(_("ai_error_builtin"), deb_info.name->str));
      }
      
      return FALSE;
    }
    
    /* If package is already installed, quit & remove it first */
    if (NULL != deb_info.name) {
      PackageInfo pkg_info = package_info(deb_info.name->str, 
					  DPKG_INFO_INSTALLED);
      
      if (NULL != pkg_info.name) {
	represent_error(app_data, pkg_info.name->str,
			_("ai_error_alreadyinstalled"));
	return FALSE;
      }
    }

    /* Confirm installation from user */
    ULOG_INFO("Trying to confirm install..");
    if (!represent_confirmation(app_data, DIALOG_CONFIRM_INSTALL, deb)) {
      ULOG_INFO("User cancelled installation.");
      return FALSE;
    }

    /* Use dpkg to install package and get output */
    ui_create_progressbar_dialog(app_data->app_ui_data,
				 _("ai_ti_installing_installing"),
				 DPKG_METHOD_INSTALL);
    DpkgOutput output = use_dpkg(DPKG_METHOD_INSTALL, install_args,
				 app_data);
    ui_cleanup_progressbar_dialog(app_data->app_ui_data, DPKG_METHOD_INSTALL);
    ULOG_DEBUG("checking installation\n");

    /* If the file is not a deb package */
    if (NULL != g_strrstr(output.err->str, DEB_FORMAT_FAILURE) ||
        NULL != g_strrstr(output.err->str, DEB_FORMAT_FAILURE2)) {
      ULOG_INFO("Tried to install a non-deb file: '%s'.", deb);
     
      /* Show notification */
      represent_error(app_data, deb, _("ai_error_corrupted"));
      return FALSE;
    }

    /* If installation succeeds */
    if (NULL != g_strrstr(output.out->str, DPKG_STATUS_SUFFIX_INSTALL)) {
      ULOG_INFO("Installation of '%s' succeeded.", deb);

      /* Get package's name from deb package and show notification */
      PackageInfo info = package_info(deb, DPKG_INFO_PACKAGE);

      /* Installation succeeded */
      represent_notification(app_data,
			     info.name->str,
			     _("ai_ti_application_installed_text"),
			     _("ai_bd_application_installed__ok"),
			     info.name->str);

      gtk_text_buffer_set_text(GTK_TEXT_BUFFER(
       app_data->app_ui_data->main_label), MESSAGE_DOUBLECLICK, -1);

      return TRUE;

    /* If installation fails */
    } else {
      ULOG_INFO("Installation of '%s' failed.", deb);
      ULOG_DEBUG("Installation of '%s' failed.\n", deb);

      /* Checking package name, or using deb name if package unavailable
       * eg. MMC that was removed */
      PackageInfo info = package_info(deb, DPKG_INFO_PACKAGE);
      if (info.name == NULL) {
	ULOG_DEBUG("name was null, inserted '%s' instead.\n", deb);
	info.name = g_string_new(deb);
      }
      gchar *err = g_strdup_printf(_("ai_ti_installation_failed_text"),
				   info.name->str);

      /* Installation failed, do you wanna see details */
      if (represent_confirmation(app_data, DIALOG_SHOW_DETAILS, err)) {

	/* Show error to user */
	gchar *errormsg = verbalize_error(output.err->str);
	represent_error(app_data, info.name->str, errormsg);
      }

      /* Remove any remains if any left from that, many packages
	 install as non-configured, if they fail deps */
      gchar *uninstall_args[9];
      uninstall_args[0] = FAKEROOT_BINARY;
      uninstall_args[1] = DPKG_BINARY; 
      uninstall_args[2] = "--force-not-root";
      uninstall_args[3] = "--root=/var/lib/install";
      uninstall_args[4] = "--status-fd";
      uninstall_args[5] = "1";
      uninstall_args[6] = "--purge";
      uninstall_args[7] = info.name->str;
      uninstall_args[8] = (gchar *) NULL;
      
      use_dpkg(DPKG_METHOD_UNINSTALL, uninstall_args, app_data);

      g_free(err);
      return FALSE;
    }
  }

  return FALSE;
}



gboolean uninstall_package(gchar *package, AppData *app_data)
{
  gchar *uninstall_args[9];

  /* Arg array for dpkg uninstall */
  uninstall_args[0] = FAKEROOT_BINARY;
  uninstall_args[1] = DPKG_BINARY; 
  uninstall_args[2] = "--force-not-root";
  uninstall_args[3] = "--root=/var/lib/install";
  uninstall_args[4] = "--status-fd";
  uninstall_args[5] = "1";
  uninstall_args[6] = "--purge";
  uninstall_args[7] = package;
  uninstall_args[8] = (gchar *) NULL;


  /* Start uninstallation if package is given */
  if (0 != g_strcasecmp(package, "")) {
    ULOG_INFO("Started uninstalling '%s'..", package);
    gchar *deps = show_remove_dependencies(package);

    /* Some dependencies found */
    if (0 != g_strcasecmp(deps, "")) {
      represent_dependencies(app_data, deps);
      return FALSE;
    }

    /* Confirm uninstallation from user */
    if (!represent_confirmation(app_data, DIALOG_CONFIRM_UNINSTALL, 
				package)) {
      ULOG_INFO("User cancelled uninstallation.");
      return FALSE;
    }


    /* Use dpkg to uninstall package and get output */
    ui_create_progressbar_dialog(app_data->app_ui_data,
				 _("ai_ti_uninstall_progress_uninstalling"),
				 DPKG_METHOD_UNINSTALL);
    DpkgOutput output = use_dpkg(DPKG_METHOD_UNINSTALL, uninstall_args,
				 app_data);
    ui_cleanup_progressbar_dialog(app_data->app_ui_data,DPKG_METHOD_UNINSTALL);
    
    /* Uninstallation succeeds */
    if (0 == g_strrstr(output.err->str, DPKG_STATUS_UNINSTALL_ERROR)) {
      ULOG_INFO("Uninstallation of '%s' succeeded.", package);
      
      /* Show notification to user */
      represent_notification(app_data,
			     package,
			     _("ai_ti_application_uninstalled_text"), 
			     _("Ai_ti_application_uninstalled_ok"),
			     package);
      
      return TRUE;
      
      /* If uninstallation fails */
    } else {
      ULOG_INFO("Uninstallation of '%s' failed.", package);
      
      represent_notification(app_data, 
			     package,
			     _("ai_ti_uninstallation_failed_text"), 
			     _("ai_ti_uninstallation_failed_ok"),
			     package);

      return FALSE;
    }
  }

  return FALSE;
}




gchar *verbalize_error(gchar *dpkg_err) 
{
  if (0 != g_strrstr(dpkg_err, DEB_FORMAT_FAILURE))
    return _("ai_error_corrupted");

  if (0 != g_strrstr(dpkg_err, DEB_FORMAT_FAILURE2))
    return _("ai_error_corrupted");

  if (0 != g_strrstr(dpkg_err, DPKG_ERROR_LOCKED))
    return "FATAL: Something went wrong, dpkg database area is locked. "
      "Reboot machine to unlock.";

  if (0 != g_strrstr(dpkg_err, DPKG_ERROR_CANNOT_ACCESS))
    return _("ai_error_memorycardopen");

  if (0 != g_strrstr(dpkg_err, DPKG_ERROR_INCOMPATIBLE))
    return _("ai_error_incompatible");

  if (0 != g_strrstr(dpkg_err, DPKG_ERROR_MEMORYFULL))
    return _("ai_info_notenoughmemory");

  if (0 != g_strrstr(dpkg_err, DPKG_ERROR_MEMORYFULL2))
    return _("ai_info_notenoughmemory");

  if (0 != g_strrstr(dpkg_err, DPKG_ERROR_TIMEDOUT))
    return _("ai_error_uninstall_timedout");

  if (0 != g_strrstr(dpkg_err, DPKG_ERROR_CLOSE_APP))
    return _("ai_error_uninstall_applicationrunning");

  if (0 != g_strrstr(dpkg_err, DPKG_FIELD_ERROR)) {
    GString *err = g_string_new("");
    dpkg_err = g_strstr_len(dpkg_err, 1024, DPKG_ERROR_INSTALL_DEPENDS);

    while ( (dpkg_err != NULL) && strlen(dpkg_err) > 0) {
      gchar **row = g_strsplit(dpkg_err, " ", 5);
      gchar *dep = g_strdup_printf(_("ai_error_componentmissing"), row[3]);

      //ULOG_DEBUG("found dep: '%s'\n", row[3]);
      err = g_string_append(err, dep);
      err = g_string_append(err, "\n");
      dpkg_err = (char *)g_strstr_len((char *)(dpkg_err+sizeof(char *)), 1024, 
				      DPKG_ERROR_INSTALL_DEPENDS);

      g_free(dep);
      g_strfreev(row);
    }

    return err->str;
  }

  return dpkg_err;
}



gint space_left_on_device(void)
{
  gint free_space_in_kb = -1;
  struct statvfs buf;

  if (statvfs("/", &buf) != 0)
    ULOG_DEBUG("statvfs failed!");

  free_space_in_kb = (buf.f_bsize * buf.f_bfree) / 1024;
  ULOG_DEBUG("free space on device: %d kb\n", free_space_in_kb);

  return free_space_in_kb;
}

