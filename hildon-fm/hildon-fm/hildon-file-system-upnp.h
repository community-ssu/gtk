#ifndef __HILDON_FILE_SYSTEM_UPNP_H__
#define __HILDON_FILE_SYSTEM_UPNP_H__ 

#include "hildon-file-system-remote-device.h"

G_BEGIN_DECLS

#define HILDON_TYPE_FILE_SYSTEM_UPNP            (hildon_file_system_upnp_get_type ())
#define HILDON_FILE_SYSTEM_UPNP(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), HILDON_TYPE_FILE_SYSTEM_UPNP, HildonFileSystemUpnp))
#define HILDON_FILE_SYSTEM_UPNP_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), HILDON_TYPE_FILE_SYSTEM_UPNP, HildonFileSystemUpnpClass))
#define HILDON_IS_FILE_SYSTEM_UPNP(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), HILDON_TYPE_FILE_SYSTEM_UPNP))
#define HILDON_IS_FILE_SYSTEM_UPNP_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), HILDON_TYPE_FILE_SYSTEM_UPNP))
#define HILDON_FILE_SYSTEM_UPNP_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), HILDON_TYPE_FILE_SYSTEM_UPNP, HildonFileSystemUpnpClass))


typedef struct _HildonFileSystemUpnp HildonFileSystemUpnp;
typedef struct _HildonFileSystemUpnpClass HildonFileSystemUpnpClass;


struct _HildonFileSystemUpnp
{
    HildonFileSystemRemoteDevice parent_instance;
};

struct _HildonFileSystemUpnpClass
{
    HildonFileSystemRemoteDeviceClass parent_class;
};

GType hildon_file_system_upnp_get_type (void) G_GNUC_CONST;

G_END_DECLS

#endif
