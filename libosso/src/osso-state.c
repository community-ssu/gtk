/**
 * @file osso-state.c
 * This file implements the state saving related features.
 * 
 * This file is part of libosso
 *
 * Copyright (C) 2005 Nokia Corporation. All rights reserved.
 *
 * Contact: Kimmo Hämäläinen <kimmo.hamalainen@nokia.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * version 2.1 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 */

#include "osso-state.h"
#include "osso-log.h"
#include "osso-internal.h"

static osso_return_t _write_state(const gchar *statefile, osso_state_t *state);
static osso_return_t _read_state(const gchar *statefile, osso_state_t *state);

static gboolean _validate_state(osso_state_t *state)
{
    if((state == NULL) || (state->state_data == NULL) ||
       (state->state_size == 0) )
    {
	return FALSE;
    }
    else {
	return TRUE;
    }
}

/************************************************************************/
osso_return_t osso_state_write(osso_context_t *osso, osso_state_t *state)
{
    gchar *path;
    gchar *tmpdir_path = NULL;
    osso_return_t ret;

    if (_validate_state(state) == FALSE)
    {
	ULOG_ERR_F("NULL state pointer, or state size invalid");
	return OSSO_INVALID;
    }
    if (!validate_osso_context(osso)) {
	ULOG_ERR_F("appname/version invalid or osso context NULL");
	return OSSO_INVALID;
    }

    /* Create the filename path from application name and version. */
    /* If STATEDIR has been defined, use it as the base. */
    /* Note: tmpdir_path does not leak memory. */ 

    tmpdir_path = getenv(LOCATION_VAR);

    if (tmpdir_path != NULL)
      {
	path = g_strconcat(tmpdir_path, "/",
			   osso->application, "/",
			   osso->version, NULL);
      }
    else 
      {
	path = g_strconcat(FALLBACK_PREFIX "/",
			   osso->application, "/",
			   osso->version, NULL);
      }

    if (path == NULL) {
	ULOG_ERR_F("g_strconcat failed");
	return OSSO_ERROR;
    }
    
    ret = _write_state(path, state);
    
    g_free(path);

    return ret;
}


/************************************************************************/
osso_return_t osso_state_read(osso_context_t *osso, osso_state_t *state)
{
    gchar *path = NULL, *tmpdir_path = NULL;
    osso_return_t ret;

    if (state == NULL)
    {
	ULOG_ERR_F("NULL state pointer");
	return OSSO_INVALID;
    }
    if (!validate_osso_context(osso)) {
	ULOG_ERR_F("appname/version invalid or osso context NULL");
	return OSSO_INVALID;
    }
    
    /* Create the filename path from application name and version. */
    
    tmpdir_path = getenv(LOCATION_VAR);

    if (tmpdir_path != NULL)
      {
	path = g_strconcat(tmpdir_path, "/",
			   osso->application, "/",
			   osso->version, NULL);
      }
    else
      {
	path = g_strconcat(FALLBACK_PREFIX "/",
			   osso->application, "/",
			   osso->version, NULL);
      }

    if (path == NULL) {
	ULOG_ERR_F("Allocation of application/version string failed");
	return OSSO_ERROR;
    }

    ret = _read_state(path, state);

    g_free(path);

    return ret;
}

/************************************************************************/
int osso_state_open_write(osso_context_t *osso)
{
    gint fd = -1;
    gchar *tmpdir_path  = NULL;
    gchar *path = NULL, *app_path = NULL;
    struct stat statbuf;

    if (!validate_osso_context(osso)) {
      ULOG_ERR_F("appname/version invalid or osso context NULL");
      return -1;
    }
    
    tmpdir_path = getenv(LOCATION_VAR);
    
    if (tmpdir_path != NULL)
      {
	path = g_strconcat(tmpdir_path, "/",
			   osso->application, "/",
			   osso->version, NULL);
      }
    else
      {
	path = g_strconcat(FALLBACK_PREFIX "/",
			   osso->application, "/",
			   osso->version, NULL);
      }

    if (path == NULL) {
      ULOG_ERR_F("Allocation of application/version string failed");
      return -1;
    }
    
    /* Check for the existence of application directory. */

    if (tmpdir_path == NULL)
      {
	app_path = g_strconcat(FALLBACK_PREFIX "/",
			       osso->application,
			       NULL);
      }
    else {
      app_path = g_strconcat(tmpdir_path, "/", osso->application, NULL);
      
    }
    if (app_path == NULL) {
      ULOG_ERR_F("Allocation of application string failed");
      g_free(path);
      return -1;
    }
    if (stat(app_path, &statbuf) != -1) {
      if (!S_ISDIR(statbuf.st_mode)) {
	ULOG_ERR_F("Other type of file instead of app directory");
	g_free(app_path);
	g_free(path);
	return -1;
      }
    } else {

      /* It's the responsibility of the startup scripts to create
	 the directory path contained in the STATESAVEDIR env var */

      if (mkdir(app_path, S_IRUSR | S_IWUSR | S_IXUSR) != 0) {
	ULOG_ERR_F("Could not create application state directory");
	g_free(app_path);
	g_free(path);
	return -1;
      }
    }
    
    g_free(app_path);
    if ((fd = open(path, O_WRONLY | O_CREAT | O_TRUNC,
		   S_IRUSR | S_IWUSR)) == -1) {
      ULOG_ERR_F("Opening of state file failed");
    }
    g_free(path);
    return fd;
}


/************************************************************************/
int osso_state_open_read(osso_context_t *osso)
{
    gint fd = -1;
    gchar *tmpdir_path  = NULL;
    gchar *path = NULL;

    if (!validate_osso_context(osso)) {
      ULOG_ERR_F("appname/version invalid or osso context NULL");
      return -1;
    }

    /* Create the filename path from application name and version. */
    tmpdir_path = getenv(LOCATION_VAR);
    if (tmpdir_path == NULL)
      {
	path = g_strconcat(FALLBACK_PREFIX "/",
			   osso->application, "/",
			   osso->version, NULL);
      }
    else {
      path = g_strconcat(tmpdir_path, "/",
			 osso->application, "/",
			 osso->version, NULL);
      }
    if (path == NULL) {
      ULOG_ERR_F("Allocation of application/version string failed");
      return -1;
    }
    if ((fd = open(path, O_RDONLY)) == -1) {
      ULOG_ERR_F("Opening of state file failed");
    }
    g_free(path);
    return fd;
}

static gboolean reliable_close(int fd)
{
    do {
        if (close(fd) == 0) {
            return TRUE;
        } else if (errno != EINTR) {
            ULOG_ERR_F("Unable to close state file: %s", strerror(errno));
            return FALSE;
        }
    } while (1);
}

/************************************************************************/
void osso_state_close(osso_context_t * osso, gint fd)
{
    reliable_close(fd);
}

/************************************************************************/
static osso_return_t _read_state(const gchar *statefile, osso_state_t *state)
{
    osso_return_t ret=OSSO_OK;
    guint32 size;
    gint fd = -1;
    ssize_t bytes_read=0, total_bytes=0;
    gpointer p;
    gboolean free_state_data_on_error = FALSE;
    
    fd = open(statefile, O_RDONLY);
    if (fd == -1) {	
	ret = OSSO_ERROR_NO_STATE;
	goto _get_state_ret2;
    }
    
_read_state_read_again:
    bytes_read = read(fd, &size, sizeof(size));
    if (bytes_read == -1 && errno == EINTR) {
        goto _read_state_read_again;
    }
    if (bytes_read != sizeof(size)) {
	ULOG_ERR_F("Error reading size from statefile '%s': %s",
		   statefile, strerror(errno));
	ret = OSSO_ERROR_STATE_SIZE;
	goto _get_state_ret1;
    }

    if(state->state_size == 0) {
	state->state_size = size;
    }
    else {
	if(state->state_size != size) {
            ULOG_ERR_F("specified size does not match read size");
	    ret = OSSO_ERROR_STATE_SIZE;
	    goto _get_state_ret1;
	}
    }
    dprint("statefile = '%s', of size %d",statefile, state->state_size);
    
    if(state->state_data == NULL) {
	state->state_data = calloc(1, size);
	if(state->state_data == NULL) {
	    ULOG_ERR_F("Error allocating memory for state data");
	    ret = OSSO_ERROR;
	    goto _get_state_ret1;
	}
        free_state_data_on_error = TRUE;
    }
    p = state->state_data;
    do {
	bytes_read = read(fd, p, state->state_size - total_bytes);
	if (bytes_read == 0) {
            /* there is no state_size bytes to read */
            ULOG_ERR_F("specified size does not match data size");
	    ret = OSSO_ERROR_STATE_SIZE;
	    goto _get_state_ret1;
	}
	if (bytes_read == -1 && errno != EINTR) {
	    ULOG_ERR_F("Failed to read state data from file '%s': %s",
			statefile, strerror(errno));
	    ret = OSSO_ERROR;
	    goto _get_state_ret1;
	}
	total_bytes += bytes_read;
	p += bytes_read;
    }while(total_bytes < state->state_size);

    if(total_bytes < state->state_size) {
	ULOG_ERR_F("Read %u bytes out of %u",
		   total_bytes, state->state_size);
	ret = OSSO_ERROR_STATE_SIZE;
        goto _get_state_ret1;
    }
    dprint("Read %u bytes out of %u", total_bytes, state->state_size);

    _get_state_ret1:
    if (!reliable_close(fd)) {
	ULOG_ERR_F("Unable to close file '%s': %s", statefile,
                   strerror(errno));
	if (ret == OSSO_OK) {
	    ret = OSSO_ERROR;
        }
    }

    _get_state_ret2:
#ifdef LIBOSSO_DEBUG	
	{
	    guint i, sz;
	    guint32 *p;

	    sz = state->state_size / sizeof(guint32);
	    p=state->state_data;
	    
	    dprint("State data:");
	    for(i=0;i<sz;i++) {
		dprint("%d 0x%x",*p, *p);
		p++;
	    }
	}
#endif
    if (ret != OSSO_OK && free_state_data_on_error) {
        free(state->state_data);
        state->state_data = NULL;
    }
    return ret;
}

/************************************************************************/
static osso_return_t _write_state(const gchar *statefile, osso_state_t *state)
{
    gchar *tempfile, *statedir;
    gint fd;
    gpointer pt;
    osso_return_t ret = OSSO_OK;
    ssize_t bytes, total_bytes;
    struct stat statbuf;
    
#ifdef LIBOSSO_DEBUG	
	{
	    guint i, sz;
	    guint32 *p;

	    sz = state->state_size / sizeof(guint32);
	    p=state->state_data;
	    
	    dprint("State data:");
	    for(i=0;i<sz;i++) {
		dprint("%d 0x%x",*p, *p);
		p++;
	    }
	}
#endif
    
    statedir = g_path_get_dirname(statefile);

    /* Check for the existence of application directory. */
    
    if (stat(statedir, &statbuf) != -1) {
	if (!S_ISDIR(statbuf.st_mode)) {
	    ULOG_ERR_F("Other type of file instead of app directory");
	    g_free(statedir);
	  return OSSO_ERROR;
	}
    }
    else if (mkdir(statedir, S_IRUSR | S_IWUSR | S_IXUSR) != 0) {
	if(errno != EEXIST) {
	    ULOG_ERR_F("Could not create plugin state directory '%s': %s",
		       statedir, strerror(errno));
	    g_free(statedir);
	    return OSSO_ERROR;
	}
    }
    g_free(statedir);
    
    tempfile = g_strconcat(statefile, ".tmpXXXXXX", NULL);
    if(tempfile == NULL) {
	ULOG_ERR_F("Unable to allocate memory for tempfile");
	return OSSO_ERROR;
    }
    fd = g_mkstemp(tempfile);
    if(fd == -1) {
	ULOG_ERR_F("Unable to open temporary file for state");
	ret = OSSO_ERROR;
	goto _set_state_ret1;
    }

_write_state_write_again:
    bytes = write(fd, &state->state_size, sizeof(guint32));
    if (bytes == -1 && errno == EINTR) {
        goto _write_state_write_again;
    } else if (bytes == -1) {
	ULOG_ERR_F("Failed to write state data to file '%s': %s",
		    tempfile, strerror(errno));
        ret = OSSO_ERROR;
	goto _set_state_ret1;
    }
    dprint("wrote size (%d) to statefile",state->state_size);
    pt = state->state_data;
    total_bytes = 0;
    do {
	bytes = write(fd, pt, state->state_size - total_bytes);
	dprint("wrote %d bytes to statefile",bytes);
	if (bytes == -1 && errno != EINTR) {
	    ULOG_ERR_F("Failed to write state data to file '%s': %s",
			tempfile, strerror(errno));
	    ULOG_ERR_F("Wrote %u bytes out of %u",
			total_bytes, state->state_size);
	    ret = OSSO_ERROR;
	    break;
	}
	pt += bytes;
	total_bytes += bytes;
    }while(total_bytes < state->state_size);

    if (!reliable_close(fd)) {
        ULOG_ERR_F("Unable to close file '%s': %s", tempfile,
                   strerror(errno));
        ret = OSSO_ERROR;
    }

    /* if writing the state or closing the file failed, bail out */
    if (ret == OSSO_ERROR) {
	goto _set_state_ret1;
    }

    if (rename(tempfile, statefile) == -1) {
	ULOG_ERR_F("Unable to rename tempfile '%s' to '%s': %s",
		   tempfile, statefile, strerror(errno));
	ret = OSSO_ERROR;
    }
    _set_state_ret1:
    unlink(tempfile); /* ok to fail */
    g_free(tempfile);	
    return ret;
}
