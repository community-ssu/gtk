#ifndef __HILDON_FILE_SYSTEM_MMC_H__
#define __HILDON_FILE_SYSTEM_MMC_H__ 

#include "hildon-file-system-special-location.h"

G_BEGIN_DECLS

#define HILDON_TYPE_FILE_SYSTEM_MMC            (hildon_file_system_mmc_get_type ())
#define HILDON_FILE_SYSTEM_MMC(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), HILDON_TYPE_FILE_SYSTEM_MMC, HildonFileSystemMMC))
#define HILDON_FILE_SYSTEM_MMC_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), HILDON_TYPE_FILE_SYSTEM_MMC, HildonFileSystemMMCClass))
#define HILDON_IS_FILE_SYSTEM_MMC(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), HILDON_TYPE_FILE_SYSTEM_MMC))
#define HILDON_IS_FILE_SYSTEM_MMC_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), HILDON_TYPE_FILE_SYSTEM_MMC))
#define HILDON_FILE_SYSTEM_MMC_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), HILDON_TYPE_FILE_SYSTEM_MMC, HildonFileSystemMMCClass))


typedef struct _HildonFileSystemMMC HildonFileSystemMMC;
typedef struct _HildonFileSystemMMCClass HildonFileSystemMMCClass;


struct _HildonFileSystemMMC
{
    HildonFileSystemSpecialLocation parent_instance;

    /* private */
    gboolean available;
};

struct _HildonFileSystemMMCClass
{
    HildonFileSystemSpecialLocationClass parent_class;
};

GType hildon_file_system_mmc_get_type (void) G_GNUC_CONST;

G_END_DECLS

#endif
