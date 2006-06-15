#include "gnome-vfs-pthread.h"
#include "gnome-vfs-async-job-map.h"
#include "gnome-vfs-thread-pool.h"
#include "gnome-vfs-job-queue.h"

gboolean
gnome_vfs_pthread_init (gboolean init_deps)
{
	if (!g_threads_got_initialized) {
		g_thread_init(NULL);
	}
	
	_gnome_vfs_async_job_map_init ();
	_gnome_vfs_thread_pool_init ();
	_gnome_vfs_job_queue_init ();
	return TRUE;
}

int 
gnome_vfs_pthread_recursive_mutex_init (pthread_mutex_t *mutex)
{
	pthread_mutexattr_t attr;
	int result;

	pthread_mutexattr_init (&attr);
	pthread_mutexattr_settype (&attr, PTHREAD_MUTEX_RECURSIVE);
	result = pthread_mutex_init (mutex, &attr);
	pthread_mutexattr_destroy (&attr);
	
	return result;
}

