#include <config.h>
#include <asm-offsets.h>
#include <asm/cacheops.h>
#include <asm/regdef.h>
#include <asm/mipsregs.h>
#include <asm/addrspace.h>
#include <asm/asm.h>

LEAF(mips_mach_early_init)

	move    s0, ra

	bal	__dummy

__dummy:
	/* Get the actual address that we are running at */
	PTR_LA	t4, __dummy
	subu	t3, ra, t4	/* t3 now has reloc offset */

	PTR_LA	t1, _start
	addu	t0, t1, t3	/* t0 now has actual address of _start */

	/* Calculate end address of copy loop */
	PTR_LA	t2, _end
	ins	t2, zero, 0, 7	/* Round up to cache line for memcpy */

1:
	lw	a0, 0(t0)
	lw	a1, 4(t0)
	lw	a2, 8(t0)
	lw	a3, 12(t0)
	sw	a0, 0(t1)
	sw	a1, 4(t1)
	sw	a2, 8(t1)
	sw	a3, 12(t1)
	addiu	t0, 16
	addiu	t1, 16
	bne	t1, t2, 1b
	 nop

	sync

	/*
	 * Return to start.S now running from TEXT_BASE, which points
	 * to DRAM address space, which effectively is L2 cache now.
	 * This speeds up the init process extremely, especially the
	 * DDR init code.
	 */
	subu	s0, s0, t3	/* Fixup return address with reloc offset */
	jr.hb	s0		/* Jump back with hazard barrier */
	 nop

END(mips_mach_early_init)
