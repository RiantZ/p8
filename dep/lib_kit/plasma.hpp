/*
 * plasma_spin - portable macros for processor-specific spin loops
 *
 *   inline assembly and compiler intrinsics for building spin loops / spinlocks
 *
 *
 * Copyright (c) 2012, Glue Logic LLC. All rights reserved. code()gluelogic.com
 *
 *  This file is part of plasma.
 *
 *  plasma is free software: you can redistribute it and/or modify it under
 *  the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation, either version 2.1 of the License, or
 *  (at your option) any later version.
 *
 *  plasma is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with plasma.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef INCLUDED_PLASMA_SPIN_H
#define INCLUDED_PLASMA_SPIN_H

// #include "plasma_feature.h"
// #include "plasma_attr.h"
// #include "plasma_membar.h"
// #include "plasma_atomic.h"
// #include "plasma_endian.h"
// PLASMA_ATTR_Pragma_once

#if defined(__APPLE__) && defined(__MACH__)
    #include <libkern/OSAtomic.h>
    #include <sched.h>
#endif

#ifndef PLASMA_SPIN_C99INLINE
    #define PLASMA_SPIN_C99INLINE C99INLINE
#endif
#ifndef NO_C99INLINE
    #ifndef PLASMA_SPIN_C99INLINE_FUNCS
        #define PLASMA_SPIN_C99INLINE_FUNCS
    #endif
#endif

/* spinlocks / busy-wait loops that pause briefly (versus spinning on nop
 * instructions) benefit modern processors by reducing memory hazards upon loop
 * exit as well as potentially saving CPU power and reducing resource contention
 * with other CPU threads (SMT) (a.k.a strands) on the same core. */

/* plasma_spin_pause() */

/* The asm memory constraint (:::"memory") is not required for pause, but
 * if provided acts as a compiler fence to disable compiler optimization,
 * e.g. in a spin loop where the spin variable is (incorrectly) not a load
 * of _Atomic, or is not volatile.  The __volatile__ on the asm (where present
 * in the 'pause' implementations below) effects appropriate compiler fence.
 * NB: volatile -does not- appear to result in a compiler fence for IBM xlC */

#if defined(_MSC_VER)

    /* prefer using intrinsics directly instead of winnt.h macro */
    /* http://software.intel.com/en-us/forums/topic/296168 */
    /* https://learn.microsoft.com/en-us/cpp/intrinsics/arm-intrinsics */
    #include <intrin.h>
    #if defined(_M_AMD64) || defined(_M_IX86)
        #pragma intrinsic(_mm_pause)
        #define plasma_spin_pause() _mm_pause()
    /* (if pause not supported by older x86 assembler, "rep nop" is equivalent)*/
    /*#define plasma_spin_pause()  __asm rep nop */
    #elif defined(_M_IA64)
        #pragma intrinsic(__yield)
        #define plasma_spin_pause() __yield()
    #elif defined(_M_ARM) || defined(_M_ARMT)
        #pragma intrinsic(__yield)
        #define plasma_spin_pause() __yield()
    #elif defined(_M_ARM64) || defined(_M_ARM64EC)
        /* https://web.archive.org/web/20231004132033/https://bugs.java.com/bugdatabase/view_bug.do?bug_id=8258604
         */
        /* https://bugs.mysql.com/bug.php?id=100664 */
        #pragma intrinsic(__isb)
        #define plasma_spin_pause() __isb()
    #else
        #define plasma_spin_pause() YieldProcessor()
    #endif
    #if 0 /* do not set these in header; calling file should determine need */
  /* http://msdn.microsoft.com/en-us/library/windows/desktop/ms687419%28v=vs.85%29.aspx */
  /* NB: caller needs to include proper header for YieldProcessor()
   *     (e.g. #include <windows.h>)
   * http://stackoverflow.com/questions/257134/weird-compile-error-dealing-with-winnt-h */
  /* http://support.microsoft.com/kb/166474 */
        #define VC_EXTRALEAN
        #define WIN32_LEAN_AND_MEAN
        #include <wtypes.h>
        #include <windef.h>
        #include <winnt.h>
    #endif

#elif defined(__x86_64__) || defined(__i386__)

    /* http://software.intel.com/sites/products/documentation/studio/composer/en-us/2011Update/compiler_c/intref_cls/common/intref_sse2_pause.htm
     * http://software.intel.com/sites/default/files/m/2/c/d/3/9/25602-17689_w_spinlock.pdf
     * http://software.intel.com/en-us/forums/topic/309231
     * http://siyobik.info.gf/main/reference/instruction/PAUSE
     * http://stackoverflow.com/questions/7086220/what-does-rep-nop-mean-in-x86-assembly
     * http://stackoverflow.com/questions/7371869/minimum-time-a-thread-can-pause-in-linux
     */
    /* http://gcc.gnu.org/onlinedocs/gcc/X86-Built_002din-Functions.html
     * GCC provides __builtin_ia32_pause()
     *   Generates the pause machine instruction with a compiler memory barrier.
     * (not sure if preferred vis-a-vis _mm_pause() or asm "pause" (below)) */
    /* avoid pulling in the large header xmmintrin.h just for plasma_spin_pause()
     * clang xmmintrin.h defines _mm_pause() as pause
     * #if defined(__clang__) || defined(__INTEL_COMPILER)
     *  ||(__GNUC_PREREQ(4,7) && defined(__SSE__))
     * #include <xmmintrin.h>
     * #define plasma_spin_pause()  _mm_pause()
     * gcc 4.7 xmmintrin.h defines _mm_pause() as rep; nop, which is portable to
     * chips pre-Pentium 4.  Although opcode generated is same, we want pause
     * (if pause not supported by older x86 assembler, "rep; nop" is equivalent)
     * #define plasma_spin_pause()  __asm__ __volatile__ ("rep; nop") */
    #define plasma_spin_pause() __asm__ __volatile__("pause")

#elif defined(__ia64__)

    /* http://www.intel.com/content/www/us/en/processors/itanium/itanium-architecture-vol-3-manual.html  volume 3
     * page 145 (3:145) */
    #if (defined(__HP_cc__) || defined(__HP_aCC__))
        #define plasma_spin_pause() _Asm_hint(_HINT_PAUSE)
    #else
        #define plasma_spin_pause() __asm__ __volatile__("hint @pause")
    #endif

#elif defined(__arm__) || defined(__thumb__)

    /* ARMv7 Architecture Reference Manual (for YIELD) */
    /* ARM Compiler toolchain Compiler Reference (for __yield() instrinsic) */
    #ifdef __CC_ARM
        #define plasma_spin_pause() __yield()
    #elif defined(__aarch64___)
        /* https://web.archive.org/web/20231004132033/https://bugs.java.com/bugdatabase/view_bug.do?bug_id=8258604
         */
        /* https://bugs.mysql.com/bug.php?id=100664 */
        #define plasma_spin_pause() __asm__ __volatile__("isb")
    #else
        #define plasma_spin_pause() __asm__ __volatile__("yield")
    #endif

#elif defined(__sparc) || defined(__sparc__)

    /* http://www.oracle.com/technetwork/systems/opensparc/sparc-architecture-2011-1728132.pdf
     *   5.5.12 Pause Count (PAUSE) Register (ASR 27)
     *   7.100 Pause
     *     PAUSE instruction
     *   7.141 Write Ancillary State Register
     *     WRPAUSE instruction
     *     wr regrs1, reg_or_imm,%pause
     *
     * See thorough DELAY_SPIN implementation in Solaris kernel:
     * https://github.com/joyent/illumos-joyent/blob/master/usr/src/common/atomic/sparcv9/atomic.s
     * https://hg.openindiana.org/upstream/illumos/illumos-gate/rev/7c80b70bb8de
     *
     * Simpler:
     * https://github.com/joyent/illumos-joyent/blob/master/usr/src/lib/libc/capabilities/sun4v/common/smt_pause.s
     */
    #if 0 /* barrier only prevents compiler from optimizing away a spin loop */
        #if (defined(__SUNPRO_C) && __SUNPRO_C >= 0x5110) || (defined(__SUNPRO_CC) && __SUNPRO_CC >= 0x5110)
  /* http://www.oracle.com/technetwork/server-storage/solarisstudio/documentation/oss-compiler-barriers-176055.pdf
   * http://www.oracle.com/technetwork/server-storage/solarisstudio/documentation/oss-memory-barriers-fences-176056.pdf */
            #include <mbarrier.h>
            #define plasma_spin_pause() __compiler_barrier()
        #else
            #define plasma_spin_pause() __asm __volatile__("membar #LoadLoad" ::: "memory")
        #endif
    #endif /* barrier only prevents compiler from optimizing away a spin loop */

    /* Note: SPARC-T4 and newer come with a 'pause' instruction.
     * The the length of the pause can be modified by writing to ASR 27
     *   e.g.  wr %%g0, 128, %%asr27
     * However, for compatibility with all SPARC, do a generic pause by reading
     * the condition code register a few times.  If instead of inline assembly,
     * this was implemented as a routine in a shared library, then a function
     * optimized for the specific architecture and model could be used */
    #if defined(__SUNPRO_C)
        #define plasma_spin_pause()                                                                                   \
            __asm __volatile__("rd %%ccr, %%g0 \n\t"                                                                  \
                               "rd %%ccr, %%g0 \n\t"                                                                  \
                               "rd %%ccr, %%g0")
    #else /* defined(__SUNPRO_CC) || defined(__GNUC__) */
        #define plasma_spin_pause()                                                                                   \
            __asm __volatile__("rd %ccr, %g0 \n\t"                                                                    \
                               "rd %ccr, %g0 \n\t"                                                                    \
                               "rd %ccr, %g0")
    #endif

#elif defined(__ppc__) || defined(_ARCH_PPC) || defined(_ARCH_PWR) || defined(_ARCH_PWR2) || defined(_POWER)

    /* http://stackoverflow.com/questions/5425506/equivalent-of-x86-pause-instruction-for-ppc
     * https://www.power.org/documentation/power-instruction-set-architecture-version-2-06/attachment/powerisa_v2-06b_v2_public/
     * (requires free account) 3.2 "or" Instruction "or" Shared Resource Hints For compatibility use 'or 27,27,27'
     * instead of 'yield' extended mnemonic (available with POWER 7) and use volatile instead of __volatile__ since
     * -qlanglvl=stdc99 rejects __volatile__, though -qlanglvl=extc99 supports it)
     */
    #define plasma_spin_pause() __asm__ volatile("or 27,27,27")

#else

    /* help prevent compiler from optimizing loop away; not a pause */
    #define plasma_spin_pause() plasma_membar_ccfence()

#endif
#endif