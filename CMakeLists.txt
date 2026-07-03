cmake_minimum_required(VERSION 3.16)
project(CppAdmin CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ─── Options ──────────────────────────────────────────────────────────────────
option(ENABLE_WEB_UI "Build with web UI (views, sessions, CSRF filters)" ON)

# ─── Packages ─────────────────────────────────────────────────────────────────
find_package(Drogon CONFIG REQUIRED)
find_package(OpenSSL REQUIRED)

find_package(CURL REQUIRED)

# libjwt — optional; if absent, CppAdmin uses its own OpenSSL-based JWT helper
find_package(PkgConfig QUIET)
pkg_check_modules(LIBJWT QUIET libjwt)

# libvips optional — used for image re-encoding
pkg_check_modules(VIPS QUIET libvips>=8.0)

# ─── Sources ──────────────────────────────────────────────────────────────────
aux_source_directory(controllers/web/v1 WEB_CTL_SRC)
aux_source_directory(controllers/api/v1 API_CTL_SRC)
aux_source_directory(services        SVC_SRC)
aux_source_directory(filters         FILTER_SRC)
aux_source_directory(models          MODEL_SRC)
aux_source_directory(vendor/bcrypt   BCRYPT_SRC)

# ─── Executable ───────────────────────────────────────────────────────────────
add_executable(${PROJECT_NAME}
  main.cc
  ${WEB_CTL_SRC}
  ${API_CTL_SRC}
  ${SVC_SRC}
  ${FILTER_SRC}
  ${MODEL_SRC}
  ${BCRYPT_SRC}
)

# ─── CSP views (compile-time — must come AFTER add_executable) ────────────────
if(ENABLE_WEB_UI)
  # TRUE = use full relative path as view name (avoids conflicts when multiple
  # directories have identically-named .csp files, e.g. show.csp in each module).
  drogon_create_views(${PROJECT_NAME}
    ${CMAKE_CURRENT_SOURCE_DIR}/views
    ${CMAKE_CURRENT_BINARY_DIR}
    TRUE)
endif()

target_include_directories(${PROJECT_NAME} PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${CMAKE_CURRENT_SOURCE_DIR}/models
  ${CMAKE_CURRENT_SOURCE_DIR}/services
  ${CMAKE_CURRENT_SOURCE_DIR}/controllers
  ${CMAKE_CURRENT_SOURCE_DIR}/vendor
)

target_link_libraries(${PROJECT_NAME} PRIVATE
  Drogon::Drogon
  OpenSSL::SSL
  OpenSSL::Crypto
  CURL::libcurl
  crypt
)

# Link libjwt if installed — otherwise our OpenSSL-based JwtHelper is used
if(LIBJWT_FOUND)
  target_include_directories(${PROJECT_NAME} PRIVATE ${LIBJWT_INCLUDE_DIRS})
  target_link_libraries(${PROJECT_NAME} PRIVATE ${LIBJWT_LIBRARIES})
  target_compile_definitions(${PROJECT_NAME} PRIVATE HAVE_LIBJWT=1)
endif()

if(ENABLE_WEB_UI)
  target_compile_definitions(${PROJECT_NAME} PRIVATE ENABLE_WEB_UI=1)
endif()

if(VIPS_FOUND)
  target_include_directories(${PROJECT_NAME} PRIVATE ${VIPS_INCLUDE_DIRS})
  target_link_libraries(${PROJECT_NAME} PRIVATE ${VIPS_LIBRARIES})
  target_compile_definitions(${PROJECT_NAME} PRIVATE HAVE_LIBVIPS=1)
endif()

# ─── Convention checker (CI gate) ─────────────────────────────────────────────
add_custom_target(check_conventions
  COMMAND python3 ${CMAKE_SOURCE_DIR}/tools/check_conventions.py ${CMAKE_SOURCE_DIR}
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  COMMENT "Checking CppAdmin code conventions..."
)

# ─── Tests ────────────────────────────────────────────────────────────────────
enable_testing()
add_subdirectory(test)
