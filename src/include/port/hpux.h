#ifndef			BIG_ENDIAN
#define			BIG_ENDIAN		4321
#endif
#ifndef			LITTLE_ENDIAN
#define			LITTLE_ENDIAN	1234
#endif
#ifndef			PDP_ENDIAN
#define			PDP_ENDIAN		3412
#endif

#if defined(__hppa)

#ifndef			BYTE_ORDER
#define			BYTE_ORDER		BIG_ENDIAN
#endif

#elif defined(__ia64)

#ifndef			BYTE_ORDER
#define			BYTE_ORDER		LITTLE_ENDIAN
#endif

#else
#error unrecognized CPU type for HP-UX

#endif
