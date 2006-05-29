#ifndef __HILDON_FILE_SYSTEM_REMOTE_DEVICE_H__
#define __HILDON_FILE_SYSTEM_REMOTE_DEVICE_H__ 

#include "hildon-file-system-special-location.h"

G_BEGIN_DECLS

#define HILDON_TYPE_FILE_SYSTEM_REMOTE_DEVICE            (hildon_file_system_remote_device_get_type ())
#define HILDON_FILE_SYSTEM_REMOTE_DEVICE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), HILDON_TYPE_FILE_SYSTEM_REMOTE_DEVICE, HildonFileSystemRemoteDevice))
#define HILDON_FILE_SYSTEM_REMOTE_DEVICE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), HILDON_TYPE_FILE_SYSTEM_REMOTE_DEVICE, HildonFileSystemRemoteDeviceClass))
#define HILDON_IS_FILE_SYSTEM_REMOTE_DEVICE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), HILDON_TYPE_FILE_SYSTEM_REMOTE_DEVICE))
#define HILDON_IS_FILE_SYSTEM_REMOTE_DEVICE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), HILDON_TYPE_FILE_SYSTEM_REMOTE_DEVICE))
#define HILDON_FILE_SYSTEM_REMOTE_DEVICE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), HILDON_TYPE_FILE_SYSTEM_REMOTE_DEVICE, HildonFileSystemRemoteDeviceClass))


typedef struct _HildonFileSystemRemoteDevice HildonFileSystemRemoteDevice;
typedef struct _HildonFileSystemRemoteDeviceClass HildonFileSystemRemoteDeviceClass;


struct _HildonFileSystemRemoteDevice
{
    HildonFileSystemSpecialLocation parent_instance;

    /* private */
    gulong signal_handler_id;
    gboolean accessible;
};

struct _HildonFileSystemRemoteDeviceClass
{
    HildonFileSystemSpecialLocationClass parent_class;
};

GType hildon_file_system_remote_device_get_type (void) G_GNUC_CONST;

G_END_DECLS

#endif
