/*
 * This file is part of apt-https.
 *
 * Copyright (C) 2006 Nokia Corporation.
 *
 * Contact: Marius Vollmer <marius.vollmer@nokia.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version
 * 2 as published by the Free Software Foundation.
 *
 * As a special exception, linking this program with OpenSSL is
 * allowed.
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

#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libintl.h>

#include <openssl/ssl.h>
#include <cst.h>

#include <gtk/gtk.h>
#include <libosso.h>
#include <hildon-widgets/hildon-note.h>
#include <osso-applet-certman.h>

#define RESPONSE_DETAILS 1

#define _(x) (x)

static void
ask_user_response (GtkDialog *dialog, gint response, gpointer cert)
{
  if (response == RESPONSE_DETAILS)
    certmanui_simple_certificate_details_dialog (NULL, (X509 *)cert);
  else
    {
      gtk_widget_destroy (GTK_WIDGET (dialog));
      exit ((response == GTK_RESPONSE_OK)? 0 : 1);
    }
}

static void
pack_label (GtkWidget *box, const char *label)
{
  GtkWidget *l = gtk_label_new (label);
  gtk_misc_set_alignment (GTK_MISC (l), 0.0, 0.0);
  gtk_box_pack_start_defaults (GTK_BOX (box), l);
}

void
ask_user (X509 *cert, int state)
{
  osso_context_t *osso;
  GtkWidget *dialog;
  GtkWidget *vbox;

  gtk_init (0, NULL);

  osso = osso_initialize (PACKAGE, VERSION, TRUE, NULL);
  if (osso == NULL)
    {
      fprintf (stderr, "can't init osso\n");
      exit (2);
    }

  certmanui_init (osso);

  dialog = gtk_dialog_new_with_buttons (_("Secure connection"),
					NULL, GTK_DIALOG_MODAL,
					_("Continue"), GTK_RESPONSE_OK,
					_("Details"), RESPONSE_DETAILS,
					_("Cancel"), GTK_RESPONSE_CANCEL,
     NULL);

  vbox = GTK_DIALOG (dialog)->vbox;

  pack_label (vbox, _("Catalogue has sent an untrusted\ncertificate."));

  if (state == -1)
    pack_label (vbox, _("* Certificate could not validated."));
  else
    {
      if (state & CST_STATE_EXPIRED)
	pack_label (vbox, _("* Certificate has expired."));
      if (state & CST_STATE_REVOKED)
	pack_label (vbox, _("* Certificate has been revoked."));
      if (state & CST_STATE_NOTVALID)
	pack_label (vbox, _("* Certificate is invalid."));
    }
    
  g_signal_connect (dialog, "response",
		    G_CALLBACK (ask_user_response), cert);

  gtk_widget_show_all (dialog);
  gtk_main ();
}

int
main (int argc, char **argv)
{
  FILE *f;
  X509 *cert;
  CST *store;
  int state;

  if (argc != 2)
    {
      fprintf (stderr, "Usage: maemo-cert-gui asn1-certfile\n");
      exit (2);
    }

  g_type_init ();
  SSL_library_init ();
  
  f = fopen (argv[1], "rb");
  if (f == NULL)
    goto out;

  cert = d2i_X509_fp (f, NULL);
  if (cert == NULL)
    goto out;

  store = CST_open (FALSE, NULL);
  state = CST_get_state (store, cert);
  CST_free (store);

  if (state == CST_STATE_VALID)
    exit (0);

  ask_user (cert, state);

 out:
  perror (argv[1]);
  exit (2);
}
