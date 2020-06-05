find_package(MADNESS 0.10.1 CONFIG QUIET COMPONENTS world HINTS "${MADNESS_ROOT_DIR}")

if (NOT TARGET MADworld)

  set(MADNESS_BUILD_MADWORLD_ONLY ON CACHE BOOL "Whether to build MADNESS runtime only")
  set(ENABLE_PARSEC ON CACHE BOOL "Whether to use PaRSEC as the task backend of MADWorld")
  FetchContent_Declare(
          MADNESS
          GIT_REPOSITORY      https://github.com/m-a-d-n-e-s-s/madness.git
          GIT_TAG             ${TTG_TRACKED_MADNESS_TAG}
  )
  FetchContent_MakeAvailable(MADNESS)
  FetchContent_GetProperties(MADNESS
          SOURCE_DIR MADNESS_SOURCE_DIR
          BINARY_DIR MADNESS_BINARY_DIR
          )
  set_property(DIRECTORY ${MADNESS_SOURCE_DIR} PROPERTY EXCLUDE_FROM_ALL TRUE)

endif(NOT TARGET MADworld)

# postcond check
if (NOT TARGET MADworld)
  message(FATAL_ERROR "FindOrFetchMADNESS could not make MADworld target available")
endif(NOT TARGET MADworld)
