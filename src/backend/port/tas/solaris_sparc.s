!=======================================================================
! solaris_sparc.s -- compare and swap for solaris_sparc
!=======================================================================

! Fortunately the Sun compiler can process cpp conditionals with -P

! '/' is the comment for x86, while '!' is the comment for Sparc

#if defined(__sparcv9) || defined(__sparc)

	.section        ".text"
	.align  8
	.skip   24
	.align  4

	.global pg_atomic_cas
pg_atomic_cas:
	
	! "cas" only works on sparcv9 and sparcv8plus chips, and
	! requies a compiler targeting these CPUs.  It will fail
	! on a compiler targeting sparcv8, and of course will not
	! be understood by a sparcv8 CPU.  gcc continues to use
	! "ldstub" because it targets sparcv7.
	!
	! There is actually a trick for embedding "cas" in a 
	! sparcv8-targeted compiler, but it can only be run
	! on a sparcv8plus/v9 cpus:
	!
	!   http://cvs.opensolaris.org/source/xref/on/usr/src/lib/libc/sparc/threads/sparc.il
	!

#if defined(__sparcv9) || defined(__sparcv8plus)
	cas     [%o0],%o2,%o1
#else
	ldstub [%o0],%o1
#endif
	mov     %o1,%o0
	retl
	nop
	.type   pg_atomic_cas,2
	.size   pg_atomic_cas,(.-pg_atomic_cas)
#endif
