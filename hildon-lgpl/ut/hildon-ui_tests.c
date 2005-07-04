/*
 * This file is part of hildon-lgpl
 *
 * Copyright (C) 2005 Nokia Corporation.
 *
 * Contact: Luc Pionchon <luc.pionchon@nokia.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
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
 *
 */

/* 
 * A unit tester for various functionality of Hildon UI.
 */

#include "hildon-app.h"
#include "hildon-appview.h"
#include <pango/pango.h>
#include <gtk/gtk.h>
#include <glib.h>

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <outo.h>

/*prototypes to keep the compiler happy*/
void init_test( void );
int test1a(void);
int test_app_set_appview_1(void);
int test_app_new_with_appview_1(void);
int test_app_new_with_appview_2(void);
int test_app_get_appview_1(void);
int test3b(void);
int test3c(void);
int test4a(void);
int test4b(void);
int test4d(void);
int test5a(void);
int test5b(void);
int test_app_set_title(void);
int test7a(void);
int test8a(void);
int test8b(void);
int test9a(void);
int test10a(void);
int test10b(void);
int test11b(void);
int test15a(void);
int test16a(void);
int test16b(void);
int test_appview_set_title_1(void);
int test_appview_get_title_1(void);
int test_appview_fullscreen_key_allowed_1(void);
int test_appview_fullscreen_key_allowed_2(void);
void fakefunction(GtkButton * b, int * checkvalue);

testcase* get_tests(void);
void init_test( void );

void init_test( void )
{
    int argc = 0;
    char **argv = NULL;
    gtk_init( &argc, &argv );
}

int test1a()
{
    HildonApp * testapp;
	
    testapp = HILDON_APP (hildon_app_new ());

    assert (HILDON_IS_APP (testapp));
    return 1;
}

int test_app_set_appview_1()
{
    HildonApp * testapp;
    HildonAppView *testview;
	
    testapp = HILDON_APP (hildon_app_new ());
    testview = HILDON_APPVIEW(hildon_appview_new("app view name"));
    assert (HILDON_IS_APPVIEW (testview));
    hildon_app_set_appview( testapp, testview );
    assert (testview = hildon_app_get_appview (testapp));
    return 1;
}

int test_app_new_with_appview_1()
{ 
    HildonApp * testapp;
    HildonAppView *testview;
	
    testview = HILDON_APPVIEW(hildon_appview_new("app view name"));
    testapp = HILDON_APP (hildon_app_new_with_appview (testview));
    assert (HILDON_IS_APPVIEW (testview));  
    testview = hildon_app_get_appview (testapp);  
    assert( testview );
    return 1;
}

int test_app_new_with_appview_2()
{ 
    HildonApp * testapp;
    HildonAppView *testview;
	
    testapp = HILDON_APP (hildon_app_new_with_appview (NULL));
    testview = hildon_app_get_appview( testapp );
    assert (testview == NULL);
    return 1;
}

int test_app_get_appview_1()
{
    HildonApp * testapp;
    HildonAppView *result;
	
    testapp = HILDON_APP (hildon_app_new ());
    result = hildon_app_get_appview (NULL);
    assert( result == NULL );
    return 1;
}

int test3b()
{
    HildonApp * testapp;
    HildonAppView *testview;
    int result = 0;

	
    testapp = HILDON_APP (hildon_app_new ());
    testview = HILDON_APPVIEW(hildon_appview_new(NULL));
    hildon_app_set_appview( testapp, testview );
    result = strcmp( "", hildon_appview_get_title( testview ) );
    assert( result == 0 );

    return 1;
}

int test3c()
{
    HildonAppView *testview;
	
    testview = HILDON_APPVIEW(hildon_appview_new("app view name"));
    hildon_app_set_appview( NULL, testview );
    return 1;
}

int test4a()
{
    HildonApp * testapp;
    HildonAppView *testview;
	
    testapp = HILDON_APP (hildon_app_new ());
    testview = HILDON_APPVIEW(hildon_appview_new( "app view name"));
    hildon_app_set_appview (testapp,  testview );
    assert (hildon_app_get_appview(testapp) == testview);
    return 1;
}

int test4b()
{
    HildonApp * testapp;
	
    testapp = HILDON_APP (hildon_app_new ());
    hildon_app_set_appview (testapp,  NULL);
    assert( NULL == hildon_app_get_appview( testapp ) );
    return 1;
}

int test4d()
{
    HildonApp * testapp;
    HildonAppView *testview;
	
    testapp = HILDON_APP (hildon_app_new ());
    testview = HILDON_APPVIEW(gtk_label_new( "app view name")); 
    hildon_app_set_appview (testapp,  testview);
    return 1;
}

int test5a()
{
    HildonApp * testapp;
    int result = 0;
	

    testapp = HILDON_APP (hildon_app_new ());
    hildon_app_set_title (testapp, "app title");
    result = strcmp (hildon_app_get_title (testapp), "app title");
    assert (result == 0);
    return 1;
}

int test5b()
{
    int result;
	HildonApp * testapp;
    
    testapp = HILDON_APP (hildon_app_new ());
    hildon_app_set_title (testapp, NULL);
    result = strcmp (hildon_app_get_title (testapp), "");
    assert ( result == 0);
    return 1;
}

int test_app_set_title()
{

    HildonApp * testapp;
	
    testapp = HILDON_APP (hildon_app_new ());
    hildon_app_set_title (NULL, "Blah");
    return 1;
}

int test7a()
{
    HildonApp * testapp;
	
    testapp = HILDON_APP (hildon_app_new ());
    hildon_app_set_zoom (testapp, HILDON_ZOOM_SMALL);
    assert (hildon_app_get_zoom (testapp) == HILDON_ZOOM_SMALL);
    hildon_app_set_zoom (testapp, HILDON_ZOOM_MEDIUM);
    assert (hildon_app_get_zoom (testapp) == HILDON_ZOOM_MEDIUM);
    hildon_app_set_zoom (testapp, HILDON_ZOOM_LARGE);
    assert (hildon_app_get_zoom (testapp) == HILDON_ZOOM_LARGE);
    return 1;
}

int test8a()
{
    PangoFontDescription * testfd;
    HildonApp * testapp;
	
    testapp = HILDON_APP (hildon_app_new ());
    testfd = hildon_app_get_default_font (testapp);
    assert (testfd != NULL);
    pango_font_description_free (testfd);
    return 1;
}

int test8b()
{
    HildonApp * testapp;
    PangoFontDescription * testfd;
	
    testapp = HILDON_APP (hildon_app_new ());
    hildon_app_set_zoom (testapp, HILDON_ZOOM_SMALL);
    testfd = hildon_app_get_zoom_font (testapp);
    assert (testfd != NULL);
    pango_font_description_free (testfd);
    hildon_app_set_zoom (testapp, HILDON_ZOOM_MEDIUM);
    testfd = hildon_app_get_zoom_font (testapp);
    assert (testfd != NULL);
    pango_font_description_free (testfd);
    hildon_app_set_zoom (testapp, HILDON_ZOOM_LARGE);
    testfd = hildon_app_get_zoom_font (testapp);
    assert (testfd != NULL);
    pango_font_description_free (testfd);
    return 1;
}

int test9a()
{
    HildonApp * testapp;
	
    testapp = HILDON_APP (hildon_app_new ());
    hildon_app_set_two_part_title (testapp, TRUE);
    assert (hildon_app_get_two_part_title (testapp) == TRUE);
    hildon_app_set_two_part_title (testapp, FALSE);
    assert (hildon_app_get_two_part_title (testapp) == FALSE);
    return 1;
}

int test10a()
{
    HildonApp * testapp;
    HildonAppView *testview;
	
    testapp = HILDON_APP (hildon_app_new ());
    testview = HILDON_APPVIEW(hildon_appview_new("app view name"));
    hildon_app_set_appview( testapp, testview );
    hildon_appview_set_fullscreen (testview, TRUE);
    assert (hildon_appview_get_fullscreen (testview) == TRUE);
    return 1;
}

int test10b()
{
    HildonApp * testapp;
    HildonAppView *testview;
	
    testapp = HILDON_APP (hildon_app_new ());
    testview = HILDON_APPVIEW(hildon_appview_new ( "app view name"));
    hildon_app_set_appview( testapp, testview );
    hildon_appview_set_fullscreen (testview, TRUE);
    hildon_appview_set_fullscreen (testview, FALSE);
    assert (hildon_appview_get_fullscreen (testview) == FALSE);
    return 1;
}

int test11b()
{
    HildonApp * testapp;
    HildonAppView *testview;
	
    testapp = HILDON_APP (hildon_app_new ());
    testview = HILDON_APPVIEW(hildon_appview_new( "app view name"));
    hildon_app_set_appview( testapp, testview );
    return 1;
}

int test15a()
{
    GtkMenu *testmenu;
    HildonApp * testapp;
    HildonAppView *testview;

	
    testapp = HILDON_APP (hildon_app_new ());
    testview = HILDON_APPVIEW(hildon_appview_new( "app view name"));
    hildon_app_set_appview( testapp, testview );
    testmenu = hildon_appview_get_menu (testview);
    assert (GTK_IS_MENU (testmenu) == TRUE);
    return 1;
}

int test16a()
{
    HildonApp * testapp;
    HildonAppView *testview;
    int result;
	
    testapp = HILDON_APP (hildon_app_new ());
    testview = HILDON_APPVIEW(hildon_appview_new( "app view name"));
    hildon_app_set_appview( testapp, testview );
    hildon_appview_set_title (testview, "appviewname");
    result = strcmp (hildon_appview_get_title (testview), "appviewname");
    assert (result == 0);
    return 1;
}

int test16b()
{
    HildonApp * testapp;
    HildonAppView *testview;
    int result;
	
    testapp = HILDON_APP (hildon_app_new ());
    testview = HILDON_APPVIEW(hildon_appview_new( "app view name"));
    hildon_app_set_appview( testapp, testview );
    hildon_appview_set_title (testview, NULL);
    result = strcmp (hildon_appview_get_title (testview), "");
    assert (result== 0);
    return 1;
}

int test_appview_set_title_1()
{
    HildonApp * testapp;
    HildonAppView *testview;
	
    testapp = HILDON_APP (hildon_app_new ());
    testview = HILDON_APPVIEW(hildon_appview_new("app view name"));
    hildon_app_set_appview( testapp, testview );
    hildon_appview_set_title (NULL, "Blah");
    return 1;
}

int test_appview_get_title_1()
{
    HildonApp * testapp;
    HildonAppView *testview;
	
    testapp = HILDON_APP (hildon_app_new ());
    testview = HILDON_APPVIEW(hildon_appview_new("app view name"));
    hildon_app_set_appview( testapp, testview );
    hildon_appview_get_title (NULL);
    return 1;
}


int test_appview_fullscreen_key_allowed_1()
{
    HildonApp * testapp;
    HildonAppView *testview;
	
    testapp = HILDON_APP (hildon_app_new ());
    testview = HILDON_APPVIEW(hildon_appview_new("app view name"));
    hildon_app_set_appview( testapp, testview );
    hildon_appview_set_fullscreen_key_allowed(NULL, FALSE);
    return 1;  
}

int test_appview_fullscreen_key_allowed_2()
{
    HildonApp * testapp;
    HildonAppView *testview;
	
    testapp = HILDON_APP (hildon_app_new ());
    testview = HILDON_APPVIEW(hildon_appview_new("app view name"));
    hildon_app_set_appview( testapp, testview );
    hildon_appview_get_fullscreen_key_allowed(NULL);
    return 1;  
}

void fakefunction(GtkButton * b, int * checkvalue)
{
    *checkvalue = 1;
}

testcase tcases[] =
{
    {*test1a, "test 1a. app_new", EXPECT_OK},
    {*test_app_set_appview_1, "test_app_set_appview_1", EXPECT_OK},
    {*test_app_get_appview_1, "test_app_get_appview_1", EXPECT_ASSERT},
    {*test_app_new_with_appview_1, "test_app_new_with_appview_1", EXPECT_OK},
    {*test_app_new_with_appview_2, "test_app_new_with_appview_2", EXPECT_OK},
    {*test3b, "test 3b. - with NULL string", EXPECT_OK}, 
    {*test3c, "test 3c. - with NULL object", EXPECT_ASSERT},
    {*test4a, "test 4a. app_switch_to_appview", EXPECT_OK},
    {*test4b, "test 4b. - with NULL string", EXPECT_OK},
    {*test4d, "test 4d. - with invalid target appview", EXPECT_ASSERT},
    {*test5a, "test 5a. app_set/get_title", EXPECT_OK},
    {*test5b, "test 5b. - with NULL string", EXPECT_OK},
    {*test_app_set_title, "test_app_set_title", EXPECT_ASSERT},
    {*test7a, "test 7a. app_set/get zoom", EXPECT_OK},
    {*test8a, "test 8a. app_get_default_font", EXPECT_OK},
    {*test8b, "test 8b. app_get_zoom_font", EXPECT_OK},
    {*test9a, "test 9a. app_set/get_twoparttitle", EXPECT_OK},
    {*test10a, "test 10a. appview_set/get_fullscreen TRUE", EXPECT_OK},
    {*test10b, "test 10b. - with FALSE value", EXPECT_OK},
    {*test11b, "test 11b. - with FALSE value", EXPECT_OK},
    {*test15a, "test 15a. appview_get_menu", EXPECT_OK},
    {*test16a, "test 16a. appview_set/get_title", EXPECT_OK},
    {*test16b, "test 16b. - with NULL string", EXPECT_OK},
    {*test_appview_set_title_1, "test_appview_set_title_1", EXPECT_ASSERT},
    {*test_appview_get_title_1, "test_appview_get_title_1", EXPECT_ASSERT},
    {*test_appview_fullscreen_key_allowed_1, "test_appview_fullscreen_key_allowed_1", EXPECT_ASSERT},
    {*test_appview_fullscreen_key_allowed_2, "test_appview_fullscreen_key_allowed_2", EXPECT_ASSERT},

    {0} /*REMEMBER THE TERMINATING NULL*/
};
/*
   use EXPECT_ASSERT for the tests that are _ment_ to throw assert so they are 
   consider passed when they throw assert and failed when tehy do not.
   use EXPECT_OK for all other tests
   */

/** this must be present in library as this is called by the tester*/
testcase* get_tests(void)
{
    g_log_set_always_fatal (G_LOG_LEVEL_ERROR | G_LOG_LEVEL_CRITICAL | G_LOG_LEVEL_WARNING);
    return tcases;
}
