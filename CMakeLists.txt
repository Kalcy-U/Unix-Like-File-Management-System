cmake_minimum_required(VERSION 3.0.0)
# 可以在linux上使用cmake --version 命令查看cmake版本，在该txt文件中写的最小版本比它小就行
project(FileManagementSystem)

SET(CMAKE_BUILD_TYPE "Debug")  


include_directories(./includes)##将目录1（dir1）与目录2（dir2）等等路径 添加为头文件搜索路径
#link_directories(dir1 dir2)#将目录1（dir1）与目录2（dir2）等等路径 添加为库文件搜索路径
add_compile_options(-Wall -std=c++11 -g)#添加编译参数-Wall -std=c++11 -g



#定义SRC变量，其值为当前目录下所有的源代码文件
aux_source_directory(. SRC)
#aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR} SUB_SOURCES)
aux_source_directory(./src src)  #debug: aux应该不能递归搜索，需要显示地写出cpp文件的目录

#编译SRC变量所代表的源代码文件，生成main可执行件
add_executable(fsys ${SRC} ${src})
