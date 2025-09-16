# tools/CMakeLists.txt

# SQL Dump 工具
add_library(sql_dump_lib
    sql_dump/sql_dumper.cpp
    sql_dump/sql_dumper.h
)

target_link_libraries(sql_dump_lib
    catalog_lib
    storage_lib
    util_lib
)

# SQL Import 工具
add_library(sql_import_lib
    sql_import/sql_importer.cpp
    sql_import/sql_importer.h
)

target_link_libraries(sql_import_lib
    catalog_lib
    storage_lib
    util_lib
    executor_lib
)

# 工具库集合
add_library(tools_lib INTERFACE)
target_link_libraries(tools_lib INTERFACE
    sql_dump_lib
    sql_import_lib
)
