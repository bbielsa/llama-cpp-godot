# SPDX-License-Identifier: Unlicense

# See: https://crascit.com/2016/04/09/using-ccache-with-cmake/
find_program( CCACHE_PROGRAM ccache )

if ( CCACHE_PROGRAM )
    message( STATUS "Found ccache: ${CCACHE_PROGRAM}" )

    set_target_properties( ${PROJECT_NAME}
        PROPERTIES
            CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}"
            C_COMPILER_LAUNCHER "${CCACHE_PROGRAM}"
        )
endif()