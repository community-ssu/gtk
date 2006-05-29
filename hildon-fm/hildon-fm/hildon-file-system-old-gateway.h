#ifndef __HILDON_FILE_SYSTEM_OLD_GATEWAY_H__
#define __HILDON_FILE_SYSTEM_OLD_GATEWAY_H__ 

#include "hildon-file-system-remote-device.h"

G_BEGIN_DECLS

#define HILDON_TYPE_FILE_SYSTEM_OLD_GATEWAY            (hildon_file_system_old_gateway_get_type ())
#define HILDON_FILE_SYSTEM_OLD_GATEWAY(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), HILDON_TYPE_FILE_SYSTEM_OLD_GATEWAY, HildonFileSystemOldGateway))
#define HILDON_FILE_SYSTEM_OLD_GATEWAY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), HILDON_TYPE_FILE_SYSTEM_OLD_GATEWAY, HildonFileSystemOldGatewayClass))
#define HILDON_IS_FILE_SYSTEM_OLD_GATEWAY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), HILDON_TYPE_FILE_SYSTEM_OLD_GATEWAY))
#define HILDON_IS_FILE_SYSTEM_OLD_GATEWAY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), HILDON_TYPE_FILE_SYSTEM_OLD_GATEWAY))
#define HILDON_FILE_SYSTEM_OLD_GATEWAY_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), HILDON_TYPE_FILE_SYSTEM_OLD_GATEWAY, HildonFileSystemOldGatewayClass))


typedef struct _HildonFileSystemOldGateway HildonFileSystemOldGateway;
typedef struct _HildonFileSystemOldGatewayClass HildonFileSystemOldGatewayClass;


struct _HildonFileSystemOldGateway
{
    HildonFileSystemRemoteDevice parent_instance;

    /* private */
    gboolean visible;
    gboolean available;
    gulong signal_handler_id;
};

struct _HildonFileSystemOldGatewayClass
{
    HildonFileSystemRemoteDeviceClass parent_class;
};

GType hildon_file_system_old_gateway_get_type (void) G_GNUC_CONST;

G_END_DECLS

#endif
