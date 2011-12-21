#ifndef INC_sr_port_types_H
#define INC_sr_port_types_H
typedef char  * caddr_t; // TODO : is this the right places
#include <sys/types.h>
#include "../sr_unix/types.h"

//#include "gdsfhead.h"
//#include "../sr_unix/filestruct.h" //

#ifndef   key_t
typedef int key_t;
#endif

#endif // INC_sr_port_types_H
