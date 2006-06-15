/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/* gnome-vfs-job.h - Jobs for asynchronous operation of the GNOME
   Virtual File System (version for POSIX threads).

   Copyright (C) 1999 Free Software Foundation

   The Gnome Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The Gnome Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the Gnome Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.

   Author: Ettore Perazzoli <ettore@gnu.org>
*/

#ifndef GNOME_VFS_JOB_H
#define GNOME_VFS_JOB_H

/*
 * The following includes help Solaris copy with its own headers.  (With 64-
 * bit stuff enabled they like to #define open open64, etc.)
 * See http://bugzilla.gnome.org/show_bug.cgi?id=71184 for details.
 */
#ifndef _WIN32
#include <unistd.h>
#include <fcntl.h>
#endif

#include <libgnomevfs/gnome-vfs-async-ops.h>
#include <libgnomevfs/gnome-vfs-module-callback.h>
#include <libgnomevfs/gnome-vfs-module-callback-private.h>

typedef struct GnomeVFSJob GnomeVFSJob;

#define GNOME_VFS_JOB_DEBUG 0

#if GNOME_VFS_JOB_DEBUG

#include <stdio.h>

extern GStaticMutex debug_mutex;

#define JOB_DEBUG_PRINT(x)			\
G_STMT_START{					\
	struct timeval _tt;			\
	gettimeofday(&_tt, NULL);		\
	printf ("%ld:%6.ld ", _tt.tv_sec, _tt.tv_usec); \
	g_static_mutex_lock (&debug_mutex);	\
	fputs (__FUNCTION__, stdout);		\
	printf (": %d ", __LINE__);		\
	printf x;				\
	fputc ('\n', stdout);			\
	fflush (stdout);			\
	g_static_mutex_unlock (&debug_mutex);	\
}G_STMT_END

#endif

#if GNOME_VFS_JOB_DEBUG
#include <sys/time.h>

extern char *job_debug_types[];

#define JOB_DEBUG(x) JOB_DEBUG_PRINT(x)
#define JOB_DEBUG_ONLY(x) x
#define JOB_DEBUG_TYPE(x) (job_debug_types[(x)])

#else
#define JOB_DEBUG(x)
#define JOB_DEBUG_ONLY(x)
#define JOB_DEBUG_TYPE(x)

#endif

/* GNOME_VFS_OP_MODULE_CALLBACK: is not a real OpType; 
 * its intended to mark GnomeVFSAsyncModuleCallback's in the 
 * job_callback queue
 */

enum GnomeVFSOpType {
	GNOME_VFS_OP_OPEN,
	GNOME_VFS_OP_OPEN_AS_CHANNEL,
	GNOME_VFS_OP_CREATE,
	GNOME_VFS_OP_CREATE_SYMBOLIC_LINK,
	GNOME_VFS_OP_CREATE_AS_CHANNEL,
	GNOME_VFS_OP_CLOSE,
	GNOME_VFS_OP_READ,
	GNOME_VFS_OP_WRITE,
	GNOME_VFS_OP_SEEK,
	GNOME_VFS_OP_READ_WRITE_DONE,
	GNOME_VFS_OP_LOAD_DIRECTORY,
	GNOME_VFS_OP_FIND_DIRECTORY,
	GNOME_VFS_OP_XFER,
	GNOME_VFS_OP_GET_FILE_INFO,
	GNOME_VFS_OP_SET_FILE_INFO,
	GNOME_VFS_OP_MODULE_CALLBACK,
	GNOME_VFS_OP_FILE_CONTROL
};

typedef enum GnomeVFSOpType GnomeVFSOpType;

typedef struct {
	GnomeVFSURI *uri;
	GnomeVFSOpenMode open_mode;
} GnomeVFSOpenOp;

typedef struct {
	GnomeVFSAsyncOpenCallback callback;
	void *callback_data;
	GnomeVFSResult result;
} GnomeVFSOpenOpResult;

typedef struct {
	GnomeVFSURI *uri;
	GnomeVFSOpenMode open_mode;
	guint advised_block_size;
} GnomeVFSOpenAsChannelOp;

typedef struct {
	GnomeVFSAsyncOpenAsChannelCallback callback;
	void *callback_data;
	GnomeVFSResult result;
	GIOChannel *channel;
} GnomeVFSOpenAsChannelOpResult;

typedef struct {
	GnomeVFSURI *uri;
	GnomeVFSOpenMode open_mode;
	gboolean exclusive;
	guint perm;
} GnomeVFSCreateOp;

typedef struct {
	GnomeVFSAsyncCreateCallback callback;
	void *callback_data;
	GnomeVFSResult result;
} GnomeVFSCreateOpResult;

typedef struct {
	GnomeVFSURI *uri;
	char *uri_reference;
} GnomeVFSCreateLinkOp;

typedef struct {
	GnomeVFSURI *uri;
	GnomeVFSOpenMode open_mode;
	gboolean exclusive;
	guint perm;
} GnomeVFSCreateAsChannelOp;

typedef struct {
	GnomeVFSAsyncCreateAsChannelCallback callback;
	void *callback_data;
	GnomeVFSResult result;
	GIOChannel *channel;
} GnomeVFSCreateAsChannelOpResult;

typedef struct {
	char dummy; /* ANSI C does not allow empty structs */
} GnomeVFSCloseOp;

typedef struct {
	GnomeVFSAsyncCloseCallback callback;
	void *callback_data;
	GnomeVFSResult result;
} GnomeVFSCloseOpResult;

typedef struct {
	GnomeVFSFileSize num_bytes;
	gpointer buffer;
} GnomeVFSReadOp;

typedef struct {
	GnomeVFSAsyncReadCallback callback;
	void *callback_data;
	GnomeVFSFileSize num_bytes;
	gpointer buffer;
	GnomeVFSResult result;
	GnomeVFSFileSize bytes_read;
} GnomeVFSReadOpResult;

typedef struct {
	GnomeVFSFileSize num_bytes;
	gconstpointer buffer;
} GnomeVFSWriteOp;

typedef struct {
	GnomeVFSAsyncWriteCallback callback;
	void *callback_data;
	GnomeVFSFileSize num_bytes;
	gconstpointer buffer;
	GnomeVFSResult result;
	GnomeVFSFileSize bytes_written;
} GnomeVFSWriteOpResult;

typedef struct {
	GnomeVFSSeekPosition whence;
	GnomeVFSFileOffset offset;
} GnomeVFSSeekOp;

typedef struct {
	GnomeVFSAsyncSeekCallback callback;
	void *callback_data;
	GnomeVFSResult result;
} GnomeVFSSeekOpResult;

typedef struct {
	GList *uris; /* GnomeVFSURI* */
	GnomeVFSFileInfoOptions options;
} GnomeVFSGetFileInfoOp;

typedef struct {
	GnomeVFSAsyncGetFileInfoCallback callback;
	void *callback_data;
	GList *result_list; /* GnomeVFSGetFileInfoResult* */
} GnomeVFSGetFileInfoOpResult;

typedef struct {
	GnomeVFSURI *uri;
	GnomeVFSFileInfo *info;
	GnomeVFSSetFileInfoMask mask;
	GnomeVFSFileInfoOptions options;
} GnomeVFSSetFileInfoOp;

typedef struct {
	GnomeVFSAsyncSetFileInfoCallback callback;
	void *callback_data;
	GnomeVFSResult set_file_info_result;
	GnomeVFSResult get_file_info_result;
	GnomeVFSFileInfo *info;
} GnomeVFSSetFileInfoOpResult;

typedef struct {
	GList *uris; /* GnomeVFSURI* */
	GnomeVFSFindDirectoryKind kind;
	gboolean create_if_needed;
	gboolean find_if_needed;
	guint permissions;
} GnomeVFSFindDirectoryOp;

typedef struct {
	GnomeVFSAsyncFindDirectoryCallback callback;
	void *callback_data;
	GList *result_list; /* GnomeVFSFindDirectoryResult */
} GnomeVFSFindDirectoryOpResult;

typedef struct {
	GnomeVFSURI *uri;
	GnomeVFSFileInfoOptions options;
	guint items_per_notification;
} GnomeVFSLoadDirectoryOp;

typedef struct {
	GnomeVFSAsyncDirectoryLoadCallback callback;
	void *callback_data;
	GnomeVFSResult result;
	GList *list;
	guint entries_read;
} GnomeVFSLoadDirectoryOpResult;

typedef struct {
	GList *source_uri_list;
	GList *target_uri_list;
	GnomeVFSXferOptions xfer_options;
	GnomeVFSXferErrorMode error_mode;
	GnomeVFSXferOverwriteMode overwrite_mode;
	GnomeVFSXferProgressCallback progress_sync_callback;
	gpointer sync_callback_data;
} GnomeVFSXferOp;

typedef struct {
	GnomeVFSAsyncXferProgressCallback callback;
	void *callback_data;
	GnomeVFSXferProgressInfo *progress_info;
	int reply;
} GnomeVFSXferOpResult;

typedef struct {
	GnomeVFSAsyncModuleCallback    callback;
	gpointer                       user_data;
	gconstpointer		       in;
	size_t			       in_size;
	gpointer                       out;
	size_t			       out_size;
	GnomeVFSModuleCallbackResponse response;
	gpointer                       response_data;
} GnomeVFSModuleCallbackOpResult;

typedef struct {
	char *operation;
	gpointer operation_data;
	GDestroyNotify operation_data_destroy_func;
} GnomeVFSFileControlOp;

typedef struct {
	GnomeVFSAsyncFileControlCallback callback;
	gpointer callback_data;
	GnomeVFSResult result;
	gpointer operation_data;
	GDestroyNotify operation_data_destroy_func;
} GnomeVFSFileControlOpResult;

typedef union {
	GnomeVFSOpenOp open;
	GnomeVFSOpenAsChannelOp open_as_channel;
	GnomeVFSCreateOp create;
	GnomeVFSCreateLinkOp create_symbolic_link;
	GnomeVFSCreateAsChannelOp create_as_channel;
	GnomeVFSCloseOp close;
	GnomeVFSReadOp read;
	GnomeVFSWriteOp write;
	GnomeVFSSeekOp seek;
	GnomeVFSLoadDirectoryOp load_directory;
	GnomeVFSXferOp xfer;
	GnomeVFSGetFileInfoOp get_file_info;
	GnomeVFSSetFileInfoOp set_file_info;
	GnomeVFSFindDirectoryOp find_directory;
	GnomeVFSFileControlOp file_control;
} GnomeVFSSpecificOp;

typedef struct {
	/* ID of the job (e.g. open, create, close...). */
	GnomeVFSOpType type;

	/* The callback for when the op is completed. */
	GFunc callback;
	gpointer callback_data;

	/* Details of the op. */
	GnomeVFSSpecificOp specifics;

	/* The context for cancelling the operation. */
	GnomeVFSContext *context;
	GnomeVFSModuleCallbackStackInfo *stack_info;
} GnomeVFSOp;

typedef union {
	GnomeVFSOpenOpResult open;
	GnomeVFSOpenAsChannelOpResult open_as_channel;
	GnomeVFSCreateOpResult create;
	GnomeVFSCreateAsChannelOpResult create_as_channel;
	GnomeVFSCloseOpResult close;
	GnomeVFSReadOpResult read;
	GnomeVFSWriteOpResult write;
	GnomeVFSSeekOpResult seek;
	GnomeVFSGetFileInfoOpResult get_file_info;
	GnomeVFSSetFileInfoOpResult set_file_info;
	GnomeVFSFindDirectoryOpResult find_directory;
	GnomeVFSLoadDirectoryOpResult load_directory;
	GnomeVFSXferOpResult xfer;
	GnomeVFSModuleCallbackOpResult callback;
	GnomeVFSFileControlOpResult file_control;
} GnomeVFSSpecificNotifyResult;

typedef struct {
	GnomeVFSAsyncHandle *job_handle;

	guint callback_id;

	/* By the time the callback got reached the job might have been cancelled.
	 * We find out by checking this flag.
	 */
	gboolean cancelled;
	
	/* ID of the job (e.g. open, create, close...). */
	GnomeVFSOpType type;

	GnomeVFSSpecificNotifyResult specifics;
} GnomeVFSNotifyResult;

/* FIXME bugzilla.eazel.com 1135: Move private stuff out of the header.  */
struct GnomeVFSJob {
	/* Handle being used for file access.  */
	GnomeVFSHandle *handle;

	/* By the time the entry routine for the job got reached
	 * the job might have been cancelled. We find out by checking
	 * this flag.
	 */
	gboolean cancelled;

	/* Read or create returned with an error - helps
	 * flagging that we do not expect a cancel
	 */
	gboolean failed;

	/* Global lock for accessing job's 'op' and 'handle' */
	GMutex *job_lock;

	/* This condition is signalled when the master thread gets a
           notification and wants to acknowledge it.  */
	GCond *notify_ack_condition;

	/* Operations that are being done and those that are completed and
	 * ready for notification to take place.
	 */
	GnomeVFSOp *op;
	
	/* Unique identifier of this job (a uint, really) */
	GnomeVFSAsyncHandle *job_handle;

	/* The priority of this job */
	int priority;
};

GnomeVFSJob 	*_gnome_vfs_job_new      	  (GnomeVFSOpType  	 type,
						   int			 priority,
				      		   GFunc           	 callback,
				      		   gpointer        	 callback_data);
void         	 _gnome_vfs_job_destroy  	  (GnomeVFSJob     	*job);
void         	 _gnome_vfs_job_set	  	  (GnomeVFSJob     	*job,
				      		   GnomeVFSOpType  	 type,
				      		   GFunc           	 callback,
				      		   gpointer        	 callback_data);
void         	 _gnome_vfs_job_go       	  (GnomeVFSJob     	*job);
void     	 _gnome_vfs_job_execute  	  (GnomeVFSJob     	*job);
void         	 _gnome_vfs_job_module_cancel  	  (GnomeVFSJob	 	*job);
int          	 gnome_vfs_job_get_count 	  (void);

gboolean	 _gnome_vfs_job_complete	  (GnomeVFSJob 		*job);

#endif /* GNOME_VFS_JOB_H */
