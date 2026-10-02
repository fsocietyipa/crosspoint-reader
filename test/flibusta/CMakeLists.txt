add_executable(FlibustaTest
  FlibustaTest.cpp
  ${REPO_ROOT}/src/OpdsServerStore.cpp
  ${REPO_ROOT}/lib/OpdsParser/OpdsParser.cpp
  ${REPO_ROOT}/lib/expat/xmlparse.c
  ${REPO_ROOT}/lib/expat/xmlrole.c
  ${REPO_ROOT}/lib/expat/xmltok.c
)
target_include_directories(FlibustaTest PRIVATE
  stubs
  ${REPO_ROOT}/src
  ${REPO_ROOT}/lib/Serialization
  ${REPO_ROOT}/lib/OpdsParser
  ${REPO_ROOT}/lib/XmlParserUtils
  ${REPO_ROOT}/lib/expat
)
target_compile_definitions(FlibustaTest PRIVATE XML_GE=0 XML_CONTEXT_BYTES=1024)
target_link_libraries(FlibustaTest PRIVATE crosspoint_test_common GTest::gtest_main ArduinoJson)
gtest_discover_tests(FlibustaTest)
