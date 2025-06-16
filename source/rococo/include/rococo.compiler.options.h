#ifndef ROCOCO_COMPILER_OPTIONS_H
# define ROCOCO_COMPILER_OPTIONS_H

# define _ITERATOR_DEBUG_LEVEL 0 // This can speed execution in debug mode quite a bit
# define _SECURE_SCL_THROWS 0
# define _NO_DEBUG_HEAP 1

# define BLOKE_COMPILER_OPTIONS_ARE_SET 1

#ifndef ROCOCO_NO_VTABLE // Use on pure interfaces, which have no implementation
# ifdef _WIN32
#  define ROCOCO_NO_VTABLE __declspec(novtable)
# else
#  define ROCOCO_NO_VTABLE
# endif
#endif

 // #define USE_VSTUDIO_SAL 1 // Enable to detect errors in formatted print statements such as SafeFormat(...)

# define ROCOCO_INTERFACE struct ROCOCO_NO_VTABLE

#ifdef _WIN32
# define CALLTYPE_C __cdecl
#else
# define CALLTYPE_C
#endif

#ifdef _WIN32
//#pragma warning ( disable: 26812 )
#endif

#ifdef _WIN32
# define FORCE_INLINE __forceinline
# define NOT_INLINE __declspec(noinline)
#else
# define FORCE_INLINE __attribute__((always_inline)) inline
# define NOT_INLINE __attribute__((noinline))
#endif

#endif // ROCOCO
