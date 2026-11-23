#ifndef SIZE_T_H
#define SIZE_T_H

#ifdef __CYMB_COFF__
typedef unsigned long long size_t;
#else
typedef unsigned long size_t;
#endif

#endif
