#ifndef BLOKE_COMPILER_OPTIONS_H
# define BLOKE_COMPILER_OPTIONS_H

# define _HAS_ITERATOR_DEBUGGING 0
# define _SECURE_SCL 0
# define _SECURE_SCL_THROWS 0
# define _NO_DEBUG_HEAP 1

# define BLOKE_COMPILER_OPTIONS_ARE_SET 1

#ifndef NO_VTABLE // Use on pure interfaces, which have no implementation
# ifdef _WIN32
#  define NO_VTABLE __declspec(novtable)
# else
#  define NO_VTABLE
# endif
#endif

#endif