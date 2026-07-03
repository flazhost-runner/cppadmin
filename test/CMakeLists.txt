# test/CMakeLists.txt — subdirectory of the root project (no cmake_minimum_required/project here)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(CppAdmin_test
  test_main.cc
  test_auth.cc
  ${CMAKE_SOURCE_DIR}/vendor/bcrypt/bcrypt.cc
  ${CMAKE_SOURCE_DIR}/vendor/bcrypt/HtmlEscapeImpl.cc
)

target_include_directories(CppAdmin_test PRIVATE
  ${CMAKE_SOURCE_DIR}
  ${CMAKE_SOURCE_DIR}/include
  ${CMAKE_SOURCE_DIR}/models
  ${CMAKE_SOURCE_DIR}/services
  ${CMAKE_SOURCE_DIR}/vendor
)

target_link_libraries(CppAdmin_test PRIVATE
  Drogon::Drogon
  OpenSSL::SSL
  OpenSSL::Crypto
  crypt
)

ParseAndAddDrogonTests(CppAdmin_test)
