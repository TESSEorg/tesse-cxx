set(MPI_C_COMPILER "$ENV{INSTALL_PREFIX}/mpich/bin/mpicc" CACHE STRING "MPI C compiler wrapper")
set(MPI_CXX_COMPILER "$ENV{INSTALL_PREFIX}/mpich/bin/mpicxx" CACHE STRING "MPI C++ compiler wrapper")
set(MPI_CXX_SKIP_MPICXX ON CACHE BOOL "Whether to disable MPI-2 C++ bindings")

set(CMAKE_C_FLAGS_INIT             "-std=c99  -m64 -I/usr/include" CACHE STRING "Inital C compile flags")
set(CMAKE_C_FLAGS_DEBUG            "-g -Wall" CACHE STRING "Inital C debug compile flags")
set(CMAKE_C_FLAGS_MINSIZEREL       "-Os -march=native -DNDEBUG" CACHE STRING "Inital C minimum size release compile flags")
set(CMAKE_C_FLAGS_RELEASE          "-O3 -march=native -DNDEBUG" CACHE STRING "Inital C release compile flags")
set(CMAKE_C_FLAGS_RELWITHDEBINFO   "-O2 -g -Wall" CACHE STRING "Inital C release with debug info compile flags")
set(CMAKE_CXX_FLAGS_INIT           "" CACHE STRING "Inital C++ compile flags")
set(CMAKE_CXX_FLAGS_DEBUG          "-g -Wall" CACHE STRING "Inital C++ debug compile flags")
set(CMAKE_CXX_FLAGS_MINSIZEREL     "-Os -march=native -DNDEBUG" CACHE STRING "Inital C++ minimum size release compile flags")
# clang issue with mismatched alloc/free in Eigen goes away if NDEBUG is not defined ... just a workaround
set(CMAKE_CXX_FLAGS_RELEASE        "-O3 -march=native" CACHE STRING "Inital C++ release compile flags")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g -Wall" CACHE STRING "Inital C++ release with debug info compile flags")

# Libraries

set(BLAS_LINKER_FLAGS "-L/usr/lib/libblas" "-lblas" "-L/usr/lib/lapack" "-llapack" "-L/usr/lib" "-llapacke" CACHE STRING "BLAS linker flags")
set(LAPACK_LIBRARIES ${BLAS_LINKER_FLAGS} CACHE STRING "LAPACK linker flags")
set(LAPACK_INCLUDE_DIRS "/usr/include" CACHE STRING "LAPACK include directories")
set(LAPACK_COMPILE_DEFINITIONS BTAS_HAS_CBLAS=1;BTAS_HAS_LAPACKE=1;MADNESS_LINALG_USE_LAPACKE CACHE STRING "LAPACK preprocessor definitions")
set(INTEGER4 TRUE CACHE BOOL "Set Fortran integer size to 4 bytes")
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared libraries")
