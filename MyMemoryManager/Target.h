#ifndef __TARGET_H
#define __TARGET_H

#if defined(WIN32)
#include "Target.Win32.h"
#else
#error "Must include platform target file."
#endif // WIN32

#ifndef CACHE_LINE_ALIGNMENT_BYTES
#error "Must define CACHE_LINE_ALIGNMENT_BYTES."
#endif CACHE_LINE_ALIGNMENT_BYTES

#ifndef DEBUGGER_BREAK
#error "Must define DEBUGGER_BREAK function."
#endif

#endif // __TARGET_H