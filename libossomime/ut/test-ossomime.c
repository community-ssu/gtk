/* -*- mode: C; c-file-style: "stroustrup"; indent-tabs-mode: nil; -*- */
/* Copyright (C) 2004 Nokia Corporation. */
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <fcntl.h>

/* this is required */
#include <outo.h>

#include <libgnomevfs/gnome-vfs.h>
#include <libgnomevfs/gnome-vfs-mime-handlers.h> 
#include "osso-mime.h"

int test_mime_open_invalid_dbus(void);
int test_mime_open_invalid_file(void);
int test_mime_open(void);

testcase *get_tests(void);

static DBusConnection * _dbus_connect(void);
static void _dbus_disconnect(DBusConnection *conn);

char *outo_name = "osso MIME open";

#define APP_NAME "unit_test"
#define APP_VERSION "0.0.1"

int test_mime_open_invalid_dbus(void)
{
    int r;
    char *uri;
    
    uri = gnome_vfs_get_uri_from_local_path(TESTFILE);
    r = osso_mime_open(NULL, uri);
    
    return (!r);
}

int test_mime_open_invalid_file(void)
{    
    int r;
    DBusConnection *conn;

    conn = _dbus_connect();
    
    assert(conn != NULL);
    r = osso_mime_open(conn, NULL);
            
    _dbus_disconnect(conn);
    
    return (!r);
    
}

int test_mime_open(void)
{    
    int r;
    DBusConnection *conn;
    char *uri;
    
    uri = gnome_vfs_get_uri_from_local_path(TESTFILE);

    conn = _dbus_connect();
    
    assert(conn != NULL);
    r = osso_mime_open(conn, NULL);
            
    _dbus_disconnect(conn);
    
    return(r);
}

testcase cases[] = {
    {test_mime_open_invalid_dbus, "Open with invalid D-BUS", EXPECT_OK},
    {test_mime_open_invalid_file, "Open with invalid file", EXPECT_OK},
    {test_mime_open, "osso_mime_open", EXPECT_OK},
    {0}	/* remember the terminating null */
};

testcase *get_tests(void)
{
    return cases;
}

static DBusConnection * _dbus_connect(void)
{
    DBusConnection *conn;

    conn = dbus_bus_get(DBUS_BUS_SESSION, NULL);
    assert(conn != NULL);
    return conn;
}

static void _dbus_disconnect(DBusConnection *conn)
{
    assert(conn != NULL);
    dbus_connection_disconnect(conn);
    dbus_connection_unref(conn);
}
