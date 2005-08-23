/**
 * @file osso-state.c
 * This file implements the state saving related features.
 * 
 * Copyright (C) 2005 Nokia Corporation.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

#include "osso-state.h"
#include "osso-log.h"

/**
 * This internal function performs a simple validation for the application
 * and version information of the osso_context regarding their validity
 * as components of the filesystem (no slashes, value not NULL etc)
 * @param osso The osso context containing the application name and
 * version information
 * @return TRUE if the context passes the validation, FALSE otherwise.
 */

/* TODO: Move validation to osso_initialization?
 */

/************************************************************************/
static gboolean _validate(osso_context_t * osso)
{
    if ((osso == NULL) || (osso->application  == NULL) ||
	(osso->version == NULL)) {
	return FALSE;
    }
    if ((strstr(osso->application, "/") != NULL)) {
	return FALSE;
    }
    if ((strstr(osso->version, "/") != NULL)) {
	return FALSE;
    }
    return TRUE;
}

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

    if (_validate_state(state) == FALSE)
    {
	ULOG_ERR_F("NULL state pointer, or state size invalid");
	return OSSO_INVALID;
    }
    if (_validate(osso) == FALSE) {
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
	path = g_strconcat(FALLBACK_PREFIX, "/",
			   osso->application, "/",
			   osso->version, NULL);
      }

    if (path == NULL) {
	ULOG_ERR_F("Allocation of application/version string failed");
	return OSSO_ERROR;
    }
    
    _write_state(path, state);
    
    free(path);

    return OSSO_OK;
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
    if (_validate(osso) == FALSE) {
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
	path = g_strconcat(FALLBACK_PREFIX, "/",
			   osso->application, "/",
			   osso->version, NULL);
      }

    if (path == NULL) {
	ULOG_ERR_F("Allocation of application/version string failed");
	return OSSO_ERROR;
    }

    ret = _read_state(path, state);

    free(path);

    return ret;
}

/************************************************************************/
int osso_state_open_write(osso_context_t *osso)
{
    gint fd = -1;
    gchar *tmpdir_path  = NULL;
    gchar *path, *app_path = NULL;
    struct stat statbuf;

    if (_validate(osso) == FALSE) {
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
	path = g_strconcat(FALLBACK_PREFIX, "/",
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
	app_path = g_strconcat(FALLBACK_PREFIX, "/",
			       osso->application,
			       NULL);
      }
    else {
      app_path = g_strconcat(tmpdir_path, "/", osso->application, NULL);
      
    }
    if (app_path == NULL) {
      ULOG_ERR_F("Allocation of application string failed");
      free(path);
      return -1;
    }
    if (stat(app_path, &statbuf) != -1) {
      if (!S_ISDIR(statbuf.st_mode)) {
	ULOG_ERR_F("Other type of file instead of app directory");
	free(app_path);
	free(path);
	return -1;
      }
    } else {

      /* It's the responsibility of the startup scripts to create
	 the directory path contained in the STATESAVEDIR env var */

      if (mkdir(app_path, S_IRUSR | S_IWUSR | S_IXUSR) != 0) {
	ULOG_ERR_F("Could not create application state directory");
	free(app_path);
	free(path);
	return -1;
      }
    }
    
    free(app_path);
    if ((fd = open(path, O_WRONLY | O_CREAT | O_TRUNC,
		   S_IRUSR | S_IWUSR)) != -1) {
      free(path);
      return fd;
    }
    free(path);

    ULOG_ERR_F("Opening statefile failed");
    return fd;
}


/************************************************************************/
int osso_state_open_read(osso_context_t *osso)
{
    gint fd = -1;
    gchar *tmpdir_path  = NULL;
    gchar *path = NULL;

    if (_validate(osso) == FALSE) {
      ULOG_ERR_F("appname/version invalid or osso context NULL");
      return -1;
    }

    /* Create the filename path from application name and version. */
    tmpdir_path = getenv(LOCATION_VAR);
    if (tmpdir_path == NULL)
      {
	path = g_strconcat(FALLBACK_PREFIX, "/",
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
    fd = open(path, O_RDONLY);
    if ( fd != -1) {
      free(path);
      return fd;
    }
    free(path);
    return fd;
}

/************************************************************************/
void osso_state_close(osso_context_t * osso, gint fd)
{
    if (close(fd) != 0) {
      ULOG_ERR_F("Closing statefile failed");
    }
}

/************************************************************************/
osso_return_t _read_state(const gchar *statefile, osso_state_t *state)
{
    osso_return_t ret=OSSO_OK;
    guint32 size;
    gint fd = -1;
    ssize_t bytes_read=0, total_bytes=0;
    gpointer p;
    
    fd = open(statefile, O_RDONLY);
    if ( fd == -1) {	
	ret = OSSO_ERROR_NO_STATE;
	goto _get_state_ret1;
    }
    
    if(read(fd, &size, sizeof(guint32))==-1) {
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
	    ret = OSSO_ERROR_STATE_SIZE;
	    goto _get_state_ret1;
	}
    }
    dprint("statefile = '%s', of size %d",statefile, state->state_size);
    
    if(state->state_data == NULL) {
	state->state_data = malloc(size);
	if(state->state_data == NULL) {
	    ULOG_ERR_F("Error allocating memory for state data");
	    ret = OSSO_ERROR;
	    goto _get_state_ret1;
	}
    }
    p = state->state_data;
    do {
	bytes_read = read(fd, p, state->state_size - total_bytes);
	if(bytes_read == -1) {
	    ULOG_ERR_F("Failed to read state data from file '%s': %s",
			statefile, strerror(errno));
	    ret = OSSO_ERROR;
	    break;
	}
	total_bytes += bytes_read;
	p += bytes_read;
    }while(total_bytes < state->state_size);

    if(total_bytes < state->state_size) {
	ULOG_ERR_F("Read %u bytes out of %u",
		   total_bytes, state->state_size);
	ret = OSSO_ERROR_STATE_SIZE;
    }
    dprint("Read %u bytes out of %u", total_bytes, state->state_size);

    do {
	int r;
	r = close(fd);
	if(r == 0) {
	    break;
	}
	else {
	    if(errno != EINTR) {
		ULOG_ERR_F("Unable to close file '%s': %s",
			   statefile, strerror(errno));
		ret = OSSO_ERROR;
		break;
	    }
	}
    }while(1);

    _get_state_ret1:
#ifdef DEBUG	
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
    return ret;
}

/************************************************************************/
osso_return_t _write_state(const gchar *statefile, osso_state_t *state)
{
    gchar *tempfile, *statedir;
    gint fd;
    gpointer pt;
    osso_return_t ret = OSSO_OK;
    ssize_t bytes, total_bytes;
    struct stat statbuf;
    
#ifdef DEBUG	
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
	ret = OSSO_ERROR;
	goto _set_state_ret0;
    }
    fd = g_mkstemp(tempfile);
    if(fd == -1) {
	ULOG_ERR_F("Unable to open temporary file for state");
	ret = OSSO_ERROR;
	goto _set_state_ret1;
    }

    bytes = write(fd, &state->state_size, sizeof(guint32));
    if(bytes == -1) {
	ULOG_ERR_F("Failed to write state data to file '%s': %s",
		    tempfile, strerror(errno));
	goto _set_state_ret1;
    }
    dprint("wrote size (%d) to statefile",state->state_size);
    pt = state->state_data;
    total_bytes = 0;
    do {
	bytes = write(fd, pt, state->state_size - total_bytes);
	dprint("wrote %d bytes to statefile",bytes);
/*	if(bytes == state->state_size)
	    break;
*/	if(bytes == -1) {
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

    do {
	int r;
	r = close(fd);
	if(r == 0) {
	    break;
	}
	else {
	    if(errno != EINTR) {
		ULOG_ERR_F("Unable to close file '%s': %s",
			   tempfile, strerror(errno));
		ret = OSSO_ERROR;
		break;
	    }
	}
    }while(1);
    
    /* if writing the state or closing the file failed, bail out */
    if(ret == OSSO_ERROR)
	goto _set_state_ret1;

    if(rename(tempfile, statefile) == -1) {
	ULOG_ERR_F("Unable to rename tempfile '%s' to '%s': %s",
		   tempfile, statefile, strerror(errno));
	ret = OSSO_ERROR;
    }
    _set_state_ret1:
    free(tempfile);	
    _set_state_ret0:
    return ret;
}
