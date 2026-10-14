include_guard(GLOBAL)

# Explicit local-prefix-only lookup. No FetchContent or network access.
function(visionflow_find_zxing_reader prefix)
  if(NOT IS_ABSOLUTE "${prefix}")
    message(FATAL_ERROR "VISIONFLOW_ZXING_PREFIX must be an absolute local prefix")
  endif()
  if(NOT EXISTS "${prefix}/lib/cmake/ZXing/ZXingConfig.cmake")
    message(FATAL_ERROR "Pinned ZXing reader missing from explicit offline prefix: ${prefix}")
  endif()
  # Do not let a stale cache or system installation override the selected prefix.
  set(ZXing_DIR "${prefix}/lib/cmake/ZXing" CACHE PATH "Pinned local ZXing config" FORCE)
  # Upstream static export references this target without finding it itself.
  find_package(Threads REQUIRED)
  find_package(ZXing 2.3.0 EXACT CONFIG REQUIRED
    PATHS "${prefix}/lib/cmake/ZXing" NO_DEFAULT_PATH)
  get_target_property(reader_type ZXing::ZXing TYPE)
  if(NOT reader_type STREQUAL "STATIC_LIBRARY")
    message(FATAL_ERROR "T0.2 requires the pinned static reader build")
  endif()
endfunction()
