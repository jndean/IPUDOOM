//
// Copyright(C) 1993-1996 Id Software, Inc.
// Copyright(C) 2005-2014 Simon Howard
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// DESCRIPTION:
//	Fixed point implementation.
//



extern "C" {
#include <limits.h>
#include <stdint.h>

#include "m_fixed.h"
}
#include <poplar/Vertex.hpp>

#include "print.h"


extern "C"
__SUPER__ 
int abs(int x) {
    return (x < 0) ? -x : x;
}

// Fixme. __USE_C_FIXED__ or something.

extern "C"
__SUPER__ 
fixed_t FixedMul(fixed_t a, fixed_t b)
{
    return ((int64_t) a * (int64_t) b) >> FRACBITS;
    // JOSEF: What about this? fewer inst.
    // return (((a >> FRACBITS) * (b >> FRACBITS)) << FRACBITS)
    //      | (((a * b) >> FRACBITS) & 0xffff);
    // OR:
    // return (((a >> FRACBITS) * b) & 0xffff0000u)
    //      | ((a * b) >> FRACBITS);
}



struct DivWorker : public poplar::Vertex {
    fixed_t a, b;
    void compute() {
        // Using int64_t caused some weird errors. So use double instead...?
        // a = (((int64_t) a) << FRACBITS) / b;
        a = ((double) a * FRACUNIT) / (double) b; 
    }
};


extern "C"
__SUPER__
fixed_t FixedDiv(fixed_t a, fixed_t b)
{
    if ((abs(a) >> 14) >= abs(b)) {
    	return (a^b) < 0 ? INT_MIN : INT_MAX;
    } else {

        DivWorker workerArgs;
        unsigned workerArgsPtr = (unsigned)&workerArgs - TMEM_BASE_ADDR;
        workerArgs.a = a;
        workerArgs.b = b;
        void* workerFn;
        asm volatile("setzi %0, __runCodelet_DivWorker" : [workerFn] "+r"(workerFn));
        asm volatile(
            "run %[workerFn], %[workerArgs], 0\n"
            "sync %[zone]\n"
            : [workerFn] "+r"(workerFn), 
              [workerArgs] "+r"(workerArgsPtr)
            : [zone] "i"(TEXCH_SYNCZONE_LOCAL)
        );
        return workerArgs.a;
    }
}
