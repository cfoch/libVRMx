#ifndef __VRMX_IO_H__
#define __VRMX_IO_H__
#include <ostream>
#include "vrmx-types.h"

std::ostream &operator << (std::ostream &os, const vrmx::VRMMeta &meta);

#endif /* __VRMX_IO_H__ */
