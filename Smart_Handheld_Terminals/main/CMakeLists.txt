file(GLOB_RECURSE SOURCES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} FOLLOW_SYMLINKS *.c)# 所有.c文件

idf_component_register(SRCS # "main.c" 
                        ${SOURCES}
                        INCLUDE_DIRS "." 
                        "./sdcard" 
                        "./myi2c" 
                        "./sensor" 
                        "./myspi" )

