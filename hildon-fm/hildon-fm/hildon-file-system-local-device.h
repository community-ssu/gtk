#ifndef __HILDON_FILE_SYSTEM_LOCAL_DEVICE_H__
#define __HILDON_FILE_SYSTEM_LOCAL_DEVICE_H__ 

#include "hildon-file-system-special-location.h"

G_BEGIN_DECLS

#define HILDON_TYPE_FILE_SYSTEM_LOCAL_DEVICE            (hildon_file_system_local_device_get_type ())
#define HILDON_FILE_SYSTEM_LOCAL_DEVICE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), HILDON_TYPE_FILE_SYSTEM_LOCAL_DEVICE, HildonFileSystemLocalDevice))
#define HILDON_FILE_SYSTEM_LOCAL_DEVICE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), HILDON_TYPE_FILE_SYSTEM_LOCAL_DEVICE, HildonFileSystemLocalDeviceClass))
#define HILDON_IS_FILE_SYSTEM_LOCAL_DEVICE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), HILDON_TYPE_FILE_SYSTEM_LOCAL_DEVICE))
#define HILDON_IS_FILE_SYSTEM_LOCAL_DEVICE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), HILDON_TYPE_FILE_SYSTEM_LOCAL_DEVICE))
#define HILDON_FILE_SYSTEM_LOCAL_DEVICE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), HILDON_TYPE_FILE_SYSTEM_LOCAL_DEVICE, HildonFileSystemLocalDeviceClass))


typedef struct _HildonFileSystemLocalDevice HildonFileSystemLocalDevice;
typedef struct _HildonFileSystemLocalDeviceClass HildonFileSystemLocalDeviceClass;


struct _HildonFileSystemLocalDevice
{
    HildonFileSystemSpecialLocation parent_instance;

    /* private */
    gulong signal_handler_id;
};

struct _HildonFileSystemLocalDeviceClass
{
    HildonFileSystemSpecialLocationClass parent_class;
};

GType hildon_file_system_local_device_get_type (void) G_GNUC_CONST;

G_END_DECLS

#endif
