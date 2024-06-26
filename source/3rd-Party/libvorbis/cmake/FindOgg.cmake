#[=======================================================================[.rst:

FindOgg
--------

Find the native Ogg includes and library.

IMPORTED Targets
^^^^^^^^^^^^^^^^

This module defines :prop_tgt:`IMPORTED` target ``Ogg::ogg``, if
Ogg has been found.

Result Variables
^^^^^^^^^^^^^^^^

This module defines the following variables:

::

  OGG_INCLUDE_DIRS    - where to find ogg.h, etc.
  OGG_LIBRARIES       - List of libraries when using ogg.
  OGG_FOUND           - True if ogg found.

::

  OGG_VERSION_STRING  - The version of ogg found (x.y.z)

Hints
^^^^^

A user may set ``OGG_ROOT`` to a ogg installation root to tell this
module where to look.
#]=======================================================================]

if(OGG_INCLUDE_DIR)
  # Already in cache, be silent
  set(OGG_FIND_QUIETLY TRUE)
endif()

find_package(PkgConfig QUIET)
pkg_check_modules(PC_OGG QUIET ogg)

set(OGG_VERSION_STRING ${PC_OGG_VERSION})

find_path(OGG_INCLUDE_DIR ogg/ogg.h
    HINTS
        ${PC_OGG_INCLUDEDIR}
        ${PC_OGG_INCLUDE_DIRS}
        ${OGG_ROOT}
    PATH_SUFFIXES
        include
)
# MSVC built ogg may be named ogg_static.
# The provided project files name the library with the lib prefix.
find_library(OGG_LIBRARY
    NAMES
        ogg
        ogg_static
        libogg
        libogg_static
    HINTS
        ${PC_OGG_LIBDIR}
        ${PC_OGG_LIBRARY_DIRS}
        ${OGG_ROOT}
    PATH_SUFFIXES
        lib
)

# Handle the QUIETLY and REQUIRED arguments and set OGG_FOUND
# to TRUE if all listed variables are TRUE.
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Ogg
    REQUIRED_VARS
        OGG_LIBRARY
        OGG_INCLUDE_DIR
    VERSION_VAR
        OGG_VERSION_STRING
)

if(OGG_FOUND)
    set(OGG_LIBRARIES ${OGG_LIBRARY})
    set(OGG_INCLUDE_DIRS ${OGG_INCLUDE_DIR})

    if(NOT TARGET Ogg::ogg)
    add_library(Ogg::ogg UNKNOWN IMPORTED)
        set_target_properties(Ogg::ogg PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${OGG_INCLUDE_DIRS}"
            IMPORTED_LOCATION "${OGG_LIBRARIES}"
        )
  endif()
endif()

mark_as_advanced(OGG_INCLUDE_DIR OGG_LIBRARY)
