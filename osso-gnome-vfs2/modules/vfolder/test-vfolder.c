
#include <glib.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <libgnomevfs/gnome-vfs.h>

static gboolean
check_dir_exists (gchar *file_uri)
{
	GnomeVFSDirectoryHandle *handle;
	GnomeVFSResult result;

	result = gnome_vfs_directory_open (&handle,
					   file_uri,
					   GNOME_VFS_FILE_INFO_DEFAULT);
	if (result == GNOME_VFS_OK) {
		gnome_vfs_directory_close (handle);
		return TRUE;
	} else 
		return FALSE;
}

#if 0
static gint
check_dir_count (gchar *file_uri)
{
	GnomeVFSResult result;
	GList *files;
	gint retval;

	result = gnome_vfs_directory_list_load (&files,
						file_uri,
						GNOME_VFS_FILE_INFO_DEFAULT);
	if (result != GNOME_VFS_OK)
		return FALSE;

	retval = g_list_length (files);

	gnome_vfs_file_info_list_free (files);

	return retval;
}
#endif

static gboolean
check_file_exists (gchar *file_uri)
{
	GnomeVFSHandle *handle;
	GnomeVFSResult result;

	result = gnome_vfs_open (&handle,
				 file_uri,
				 GNOME_VFS_OPEN_READ);
	if (result == GNOME_VFS_OK) {
		gnome_vfs_close (handle);
		return TRUE;
	} else 
		return FALSE;
}

static gboolean
check_file_content (gchar *file_uri, gchar *content)
{
	GnomeVFSHandle *handle;
	GnomeVFSResult result;
	GnomeVFSFileSize readlen;
	gchar readbuf [2048];
	gint idx = 0, len;

	len = strlen (content);

	result = gnome_vfs_open (&handle,
				 file_uri,
				 GNOME_VFS_OPEN_READ);
	if (result != GNOME_VFS_OK)
		return FALSE;

	while (idx < len) {
		result = gnome_vfs_read (handle, 
					 readbuf, 
					 sizeof (readbuf),
					 &readlen);
		if (result != GNOME_VFS_OK) 
			goto ERROR;

		idx += readlen;
		if (idx > len)
			goto ERROR;

		if (memcmp (readbuf, 
			    &content [idx], 
			    MIN (len - idx, readlen)) != 0)
			goto ERROR;
	}
	
	gnome_vfs_close (handle);
	return TRUE;

 ERROR:
	gnome_vfs_close (handle);
	return FALSE;
}

#define TEST_FAILURE(errstr)                                   \
	do {                                                   \
		g_print (" " errstr ": %s\n",                  \
			 gnome_vfs_result_to_string (result)); \
	        return 1;                                      \
	} while (0)

#define TEST_SUCCESS g_print (" DONE.\n")

#define TEST_RESULT(predicate, error)        \
	if (predicate) TEST_FAILURE (error); \
	else TEST_SUCCESS

static gint 
test_vfolder_ops (void)
{
	GnomeVFSHandle *handle;
	GnomeVFSDirectoryHandle *dhandle;
	GnomeVFSResult result;
	gchar *uri, *realuri, *content;
	GnomeVFSFileSize writelen;

	uri = "test-vfolder:///";
	g_print ("Opening %s...\n", uri);
	result = gnome_vfs_directory_open (&dhandle,
					   uri,
					   GNOME_VFS_FILE_INFO_DEFAULT);
	TEST_RESULT (result != GNOME_VFS_OK, "ERROR OPENNING");

	/* 
	 * Directory Tests 
	 */

	/* 
	 * TEST 1: 
	 * Create a new directory and delete it 
	 */
	/* Simple directory create */
	uri = "test-vfolder:///MyTestFolder1";
	g_print ("Creating new directory...");
	result = gnome_vfs_make_directory (uri, GNOME_VFS_PERM_USER_ALL);
	TEST_RESULT (result != GNOME_VFS_OK || !check_dir_exists (uri),
		     "ERROR CREATING DIR");

	/* Simple directory delete */
	uri = "test-vfolder:///MyTestFolder1";
	g_print ("Deleting new empty directory...");
	result = gnome_vfs_remove_directory (uri);
	TEST_RESULT (result != GNOME_VFS_OK || check_dir_exists (uri),
		     "ERROR DELETING DIR");


	/* 
	 * TEST 2: 
	 * Create a new directory, add a file to it, check file exists,
	 * check we can't delete a non-empty dir, delete file, then delete
	 * directory.  
	 */
	/* Simple Create 2 */
	uri = "test-vfolder:///MyTestFolder2/";
	g_print ("Creating new directory...");
	result = gnome_vfs_make_directory (uri, GNOME_VFS_PERM_USER_ALL);
	TEST_RESULT (result != GNOME_VFS_OK || !check_dir_exists (uri),
		     "ERROR CREATING DIR");

	/* Create Empty File */
	uri = "test-vfolder:///MyTestFolder2/a_fake_file.desktop";
	g_print ("Creating new file...");
	result = gnome_vfs_create (&handle, 
				   uri, 
				   GNOME_VFS_OPEN_READ | GNOME_VFS_OPEN_WRITE, 
				   FALSE,
				   GNOME_VFS_PERM_USER_ALL);
	TEST_RESULT (result != GNOME_VFS_OK || 
		     !check_file_exists (uri) ||
		     !check_file_content (uri, ""),
		     "ERROR CREATING FILE");

	/* Try removing dir (should fail) */
	uri = "test-vfolder:///MyTestFolder2";
	g_print ("Deleting new non-empty directory...");
	result = gnome_vfs_remove_directory (uri);
	TEST_RESULT (result != GNOME_VFS_ERROR_DIRECTORY_NOT_EMPTY || 
		     !check_dir_exists (uri),
		     "ERROR DELETING DIR");

	/* Delete file */
	uri = "test-vfolder:///MyTestFolder2/a_fake_file.desktop";
	g_print ("Deleting new file...");
	result = gnome_vfs_unlink (uri);
	TEST_RESULT (result != GNOME_VFS_OK || check_file_exists (uri),
		     "ERROR DELETING FILE");

	/* Try removing dir */
	uri = "test-vfolder:///MyTestFolder2/";
	g_print ("Deleting new empty directory...");
	result = gnome_vfs_remove_directory (uri);
	TEST_RESULT (result != GNOME_VFS_OK || check_dir_exists (uri),
		     "ERROR DELETING DIR");

	
	/* 
	 * TEST 3:
	 * Check creating an existing directory fails.  Only run during first 
	 * iteration.
	 */
	/* Try to create existing empty dir (should fail) */
	uri = "test-vfolder:///EmptyFolder";
	if (check_dir_exists (uri)) {
		g_print ("Creating existing empty directory...");
		result = gnome_vfs_make_directory (uri, 
						   GNOME_VFS_PERM_USER_ALL);
		TEST_RESULT (result != GNOME_VFS_ERROR_FILE_EXISTS,
			     "ABLE TO CREATE EXISTING DIR");

		/* Try to delete existing empty dir */
		g_print ("Deleting existing empty directory...");
		result = gnome_vfs_remove_directory (uri);
		TEST_RESULT (result != GNOME_VFS_OK || check_dir_exists (uri),
			     "ERROR DELETING DIR");
	}

	/* 
	 * TEST 4: 
	 * Check we can't delete an existing hidden directory, check we can't
	 * add a file to an existing hidden directory, check we can create the
	 * existing hidden directory, create a file in now non-hidden dir,
	 * delete created file and check dir is still visible, delete dir.
	 */
	/* First, see if its hidden */
	uri = "test-vfolder:///EmptyHiddenFolder";
	g_print ("Checking if hidden directory is visible ...");
	TEST_RESULT (check_dir_exists (uri),
		     "ABLE TO SEE HIDDEN DIR");

	/* Try to delete existing empty hidden dir (should fail) */
	uri = "test-vfolder:///EmptyHiddenFolder";
	g_print ("Deleting existing empty hidden directory...");
	result = gnome_vfs_remove_directory (uri);
	TEST_RESULT (result == GNOME_VFS_OK || check_dir_exists (uri),
		     "ABLE TO DELETE HIDDEN DIR");

	/* Try to add file to existing empty hidden dir (should fail) */
	uri = "test-vfolder:///EmptyHiddenFolder/a_fake_file.desktop";
	g_print ("Creating file in existing empty hidden directory...");
	result = gnome_vfs_create (&handle, 
				   uri, 
				   GNOME_VFS_OPEN_WRITE, 
				   FALSE,
				   GNOME_VFS_PERM_USER_ALL);
	TEST_RESULT (result == GNOME_VFS_OK || check_dir_exists (uri),
		     "ABLE TO CREATE FILE IN HIDDEN DIR");

	/* Try to create existing empty hidden dir */
	uri = "test-vfolder:///EmptyHiddenFolder";
	g_print ("Deleting existing empty hidden directory...");
	result = gnome_vfs_make_directory (uri, GNOME_VFS_PERM_USER_ALL);
	TEST_RESULT (result != GNOME_VFS_OK || !check_dir_exists (uri),
		     "ERROR OVERRIDING HIDDEN DIR");

	/* Try to add file to existing empty (now) non-hidden dir */
	uri = "test-vfolder:///EmptyHiddenFolder/a_fake_file.desktop";
	g_print ("Creating file in existing empty hidden directory...");
	result = gnome_vfs_create (&handle, 
				   uri, 
				   GNOME_VFS_OPEN_WRITE, 
				   FALSE,
				   GNOME_VFS_PERM_USER_ALL);
	TEST_RESULT (result != GNOME_VFS_OK || !check_file_exists (uri),
		     "ERROR CREATING FILE IN DIR");

	/* Try to delete existing empty hidden dir */
	uri = "test-vfolder:///EmptyHiddenFolder";
	g_print ("Deleting existing empty hidden directory...");
	result = gnome_vfs_remove_directory (uri);
	TEST_RESULT (result == GNOME_VFS_OK || !check_dir_exists (uri),
		     "ABLE TO DELETE NON-EMPTY FOLDER");

	/* Try to delete the file we created */
	uri = "test-vfolder:///EmptyHiddenFolder/a_fake_file.desktop";
	g_print ("Deleting created file...");
	result = gnome_vfs_unlink (uri);
	TEST_RESULT (result != GNOME_VFS_OK || check_file_exists (uri),
		     "ERROR DELETING FILE");

	/* Try to delete the directory */
	uri = "test-vfolder:///EmptyHiddenFolder";
	g_print ("Deleting overridden directory...");
	result = gnome_vfs_remove_directory (uri);
	TEST_RESULT (result != GNOME_VFS_OK || check_dir_exists (uri),
		     "ERROR DELETING DIR");

	/* 
	 * TEST 5: 
	 * Add file to writedir with keywords, check presence in non-hidden
	 * folder, check presence in previsouly hidden folder, delete file,
	 * check it is not present in non-hidden folder, and check that the
	 * hidden folder is back to being not visible 
	 */
	/* Create file with keywords, check visibility */
	realuri = g_build_filename (getenv ("GNOME_VFS_VFOLDER_WRITEDIR"),
				    "test-vfolder",
				    "my_keyworded_file.desktop",
				    NULL);
	g_print ("Creating file in writedir to check keyword inclusion...");
	result = gnome_vfs_create (&handle, 
				   realuri, 
				   GNOME_VFS_OPEN_WRITE, 
				   FALSE,
				   GNOME_VFS_PERM_USER_ALL);
	TEST_RESULT (result != GNOME_VFS_OK, "ERROR CREATING FILE IN WRITEDIR");

	content = "Categories=FakeCategory1;FakeCategory2;FakeCategory3\n\n";
	g_print ("Writing content...");
	result = gnome_vfs_write (handle,
				  content,
				  strlen (content),
				  &writelen);
	TEST_RESULT (result != GNOME_VFS_OK, "ERROR WRITING FILE IN WRITEDIR");

	g_print ("Closing file...");
	result = gnome_vfs_close (handle);
	TEST_RESULT (result != GNOME_VFS_OK, "ERROR CLOSING FILE");

	uri = "test-vfolder:///KeywordFolder/my_keyworded_file.desktop";
	g_print ("Checking file presence in /KeywordFolder/...");
	TEST_RESULT (!check_file_content (uri, content),
		     "KEYWORDED FILE NOT PRESENT");

	uri = "test-vfolder:///EmptyHiddenFolder";
	g_print ("Checking /EmptyHiddenFolder/ is now visible...");
	TEST_RESULT (!check_dir_exists (uri),
		     "HIDDEN DIRECTORY NOT VISIBLE");

	uri = "test-vfolder:///EmptyHiddenFolder/my_keyworded_file.desktop";
	g_print ("Checking file presence in /EmptyHiddenFolder/...");
	TEST_RESULT (!check_file_content  (uri, content),
		     "KEYWORDED FILE NOT PRESENT IN HIDDEN DIRECTORY");

	g_print ("Deleting new file from writedir...");
	result = gnome_vfs_unlink (realuri);
	TEST_RESULT (result != GNOME_VFS_OK || !check_file_exists (realuri),
		     "ERROR DELETING FILE");

	uri = "test-vfolder:///KeywordFolder/my_keyworded_file.desktop";
	g_print ("Checking file is missing from /KeywordFolder/...");
	TEST_RESULT (check_file_exists (uri),
		     "KEYWORDED FILE STILL PRESENT");

	uri = "test-vfolder:///EmptyHiddenFolder";
	g_print ("Checking /EmptyHiddenFolder/ is hidden again...");
	TEST_RESULT (check_dir_exists (uri),
		     "HIDDEN DIRECTORY STILL VISIBLE");

	g_free (realuri);


	/* 
	 * TEST 6:
	 * Create file and move to a different directory.
	 */
	uri = "test-vfolder:///MyTestFolder1";
	g_print ("Creating src directory...");
	result = gnome_vfs_make_directory (uri, GNOME_VFS_PERM_USER_ALL);
	TEST_RESULT (result != GNOME_VFS_OK || !check_dir_exists (uri),
		     "ERROR CREATING DIR");

	uri = "test-vfolder:///MyTestFolder1/a_fake_file.desktop";
	g_print ("Creating src file...");
	result = gnome_vfs_create (&handle, 
				   uri, 
				   GNOME_VFS_OPEN_WRITE, 
				   FALSE,
				   GNOME_VFS_PERM_USER_ALL);
	TEST_RESULT (result != GNOME_VFS_OK || !check_file_exists (uri), 
		     "ERROR CREATING FILE");

	uri = "test-vfolder:///MyTestFolder2";
	g_print ("Creating dest directory...");
	result = gnome_vfs_make_directory (uri, GNOME_VFS_PERM_USER_ALL);
	TEST_RESULT (result != GNOME_VFS_OK || !check_dir_exists (uri),
		     "ERROR CREATING DIR");
	
	g_print ("Moving dir...");
	result = gnome_vfs_move ("test-vfolder:///MyTestFolder1",
				 "test-vfolder:///MyTestFolder2/MyButt",
				 TRUE);
	TEST_RESULT (result != GNOME_VFS_OK || 
		     !check_dir_exists ("test-vfolder:///MyTestFolder2/MyButt") ||
		     check_dir_exists ("test-vfolder:///MyTestFolder1"),
		     "ERROR MOVING DIR");

	g_print ("Moving file...");
	result = gnome_vfs_move ("test-vfolder:///MyTestFolder2/MyButt/a_fake_file.desktop",
				 "test-vfolder:///MyTestFolder2",
				 TRUE);
	TEST_RESULT (result != GNOME_VFS_OK || 
		     !check_file_exists ("test-vfolder:///MyTestFolder2/a_fake_file.desktop") ||
		     check_file_exists ("test-vfolder:///MyTestFolder2/MyButt/a_fake_file.desktop"),
		     "ERROR MOVING FILE");

	uri = "test-vfolder:///MyTestFolder2/a_fake_file.desktop";
	g_print ("Deleting dest file...");
	result = gnome_vfs_unlink (uri);
	TEST_RESULT (result != GNOME_VFS_OK || check_file_exists (uri),
		     "ERROR DELETING FILE");

	/* Leave these around for future tests */
	/*
	uri = "test-vfolder:///MyTestFolder1";
	g_print ("Deleting src directory...");
	result = gnome_vfs_remove_directory (uri);
	TEST_RESULT (result != GNOME_VFS_OK || check_dir_exists (uri),
		     "ERROR DELETING DIR");

	uri = "test-vfolder:///MyTestFolder2";
	g_print ("Deleting dest directory...");
	result = gnome_vfs_remove_directory (uri);
	TEST_RESULT (result != GNOME_VFS_OK || check_dir_exists (uri),
		     "ERROR DELETING DIR");
	*/

	/* 
	 * TEST 7:
	 * Create file and rename.
	 */
	uri = "test-vfolder:///MyTestFolder1/a_file_to_rename.desktop";
	g_print ("Creating file...");
	result = gnome_vfs_create (&handle, 
				   uri, 
				   GNOME_VFS_OPEN_WRITE, 
				   FALSE,
				   GNOME_VFS_PERM_USER_ALL);
	TEST_RESULT (result != GNOME_VFS_OK || check_file_exists (uri), 
		     "ERROR CREATING FILE");

	{
		GnomeVFSFileInfo info;
		gchar *desturi;

		info.name = "a_renamed_file.desktop";

		g_print ("Renaming file...");
		result = gnome_vfs_set_file_info (uri,
						  &info,
						  GNOME_VFS_SET_FILE_INFO_NAME);
		desturi = 
			"test-vfolder:///MyTestFolder1/a_renamed_file.desktop";
		TEST_RESULT (result != GNOME_VFS_OK || 
			     check_file_exists (uri) || 
			     !check_file_exists (desturi),
			     "ERROR RENAMING FILE");
	}

	/* 
	 * TEST 8:
	 * Move new directory inside another directory.
	 */

	/* 
	 * TEST 9:
	 * Move existing directory with children inside a new directory.
	 */

	/* create dir
	 * create dir
	 * delete new dir
	 * delete existing dir

	 * create file
	 * create file with keywords, check appearance in vfolders
	 * edit new file
	 * edit existing file
	 * move new file
	 * move existing file
	 * set filename new file
	 * set filename existing file
	 * delete new file
	 * delete existing file
 
	 * monitor file creation
	 * monitor file edit
	 * monitor file delete
	 * monitor dir creation
	 * monitor dir delete	
	 */
	
	return 0;
}

int 
main (int argc, char **argv)
{
	gint iterations = 10;
	gchar *cwd, cwdbuf [2048], *path;

	putenv ("GNOME_VFS_MODULE_PATH=" GNOME_VFS_MODULE_PATH);
	putenv ("GNOME_VFS_MODULE_CONFIG_PATH=" GNOME_VFS_MODULE_CONFIG_PATH);

	cwd = getcwd (cwdbuf, sizeof (cwdbuf));

	putenv (g_strconcat ("GNOME_VFS_VFOLDER_INFODIR=", cwd, NULL));

	path = g_build_filename (cwd, "test-vfolder-tmp", NULL);
	putenv (g_strconcat ("GNOME_VFS_VFOLDER_WRITEDIR=", path, NULL));
	g_free (path);

	gnome_vfs_init ();

	if (argc > 1)
		iterations = atoi (argv [1]);

	do {
		if (test_vfolder_ops () != 0) {
			g_print ("\nFAILURE!!\n");
			gnome_vfs_shutdown ();
			return 1;
		}
		g_print ("\n\n");
	} while (--iterations);

	gnome_vfs_shutdown ();

	g_print ("\nSUCCESS!\n");
	
	return 0;
}
