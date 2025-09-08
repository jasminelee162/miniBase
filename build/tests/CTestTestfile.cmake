# CMake generated Testfile for 
# Source directory: E:/SepExp/miniBase/tests
# Build directory: E:/SepExp/miniBase/build/tests
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
if(CTEST_CONFIGURATION_TYPE MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
  add_test(storage_test "E:/SepExp/miniBase/build/tests/Debug/storage_test.exe")
  set_tests_properties(storage_test PROPERTIES  _BACKTRACE_TRIPLES "E:/SepExp/miniBase/tests/CMakeLists.txt;33;add_test;E:/SepExp/miniBase/tests/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
  add_test(storage_test "E:/SepExp/miniBase/build/tests/Release/storage_test.exe")
  set_tests_properties(storage_test PROPERTIES  _BACKTRACE_TRIPLES "E:/SepExp/miniBase/tests/CMakeLists.txt;33;add_test;E:/SepExp/miniBase/tests/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
  add_test(storage_test "E:/SepExp/miniBase/build/tests/MinSizeRel/storage_test.exe")
  set_tests_properties(storage_test PROPERTIES  _BACKTRACE_TRIPLES "E:/SepExp/miniBase/tests/CMakeLists.txt;33;add_test;E:/SepExp/miniBase/tests/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
  add_test(storage_test "E:/SepExp/miniBase/build/tests/RelWithDebInfo/storage_test.exe")
  set_tests_properties(storage_test PROPERTIES  _BACKTRACE_TRIPLES "E:/SepExp/miniBase/tests/CMakeLists.txt;33;add_test;E:/SepExp/miniBase/tests/CMakeLists.txt;0;")
else()
  add_test(storage_test NOT_AVAILABLE)
endif()
subdirs("../_deps/googletest-build")
