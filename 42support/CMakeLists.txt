cmake_minimum_required(VERSION 3.16)
project(LunarCapture LANGUAGES C CXX)
set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 11)
option(QTPLOT "Build Qt graphs" ON)
option(GUI3D "Build original 42 3D windows" ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
include_directories(../42/Include ../42/Kit/Include ../lunar)
set(BASE
../42/Kit/Source/dcmkit.c
../42/Kit/Source/envkit.c
../42/Kit/Source/fswkit.c
../42/Kit/Source/geomkit.c
../42/Kit/Source/iokit.c
../42/Kit/Source/mathkit.c
../42/Kit/Source/msis86kit.c
../42/Kit/Source/nrlmsise00kit.c
../42/Kit/Source/orbkit.c
../42/Kit/Source/sigkit.c
../42/Kit/Source/sphkit.c
../42/Kit/Source/timekit.c
../lunar/nos3_stub.c
../42/Source/42actuators.c
../42/Source/42cmd.c
../42/Source/42dynamics.c
../42/Source/42environs.c
../42/Source/42ephem.c
../42/Source/42fsw.c
../42/Source/42init.c
../42/Source/42ipc.c
../42/Source/42jitter.c
../42/Source/42joints.c
../42/Source/42optics.c
../42/Source/42perturb.c
../42/Source/42report.c
../42/Source/42sensors.c
../42/Source/AcApp.c
../42/Source/AutoCode/TxRxIPC.c
../42/Source/AutoCode/WriteAcToCsv.c
../42/Source/AutoCode/WriteScToCsv.c
../lunar/moon_core.c
../lunar/moon_mission.c
../lunar/moon_exec.c)
if(QTPLOT)
  find_package(Qt5 5.12 REQUIRED COMPONENTS Widgets)
  list(APPEND BASE ../lunar/qt_main.cpp)
else()
  list(APPEND BASE ../42/Source/42main.c)
endif()
add_executable(42sh_moon ${BASE})
if(QTPLOT)
  target_compile_definitions(42sh_moon PRIVATE _USE_QTPLOT_)
  target_link_libraries(42sh_moon PRIVATE Qt5::Widgets)
endif()
if(GUI3D)
  target_sources(42sh_moon PRIVATE ../42/Source/42gl.c ../lunar/moon_glut.c ../42/Source/42gpgpu.c ../42/Kit/Source/glkit.c)
  target_compile_definitions(42sh_moon PRIVATE _ENABLE_GUI_ _USE_SHADERS_ _USE_GLUT_)
  if(APPLE)
    target_link_libraries(42sh_moon PRIVATE "-framework OpenGL" "-framework GLUT")
  else()
    target_compile_definitions(42sh_moon PRIVATE GLEW_STATIC)
    target_sources(42sh_moon PRIVATE glew/src/glew.c)
    target_include_directories(42sh_moon PRIVATE glew/include glew/include/GL freeglut/include freeglut/include/GL)
    if(WIN32)
      if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(FG ${CMAKE_CURRENT_SOURCE_DIR}/freeglut/lib/x64/libfreeglut.a)
        set(FG_DLL ${CMAKE_CURRENT_SOURCE_DIR}/freeglut/bin/x64/freeglut.dll)
      else()
        set(FG ${CMAKE_CURRENT_SOURCE_DIR}/freeglut/lib/libfreeglut.a)
        set(FG_DLL ${CMAKE_CURRENT_SOURCE_DIR}/freeglut/bin/freeglut.dll)
      endif()
      target_link_libraries(42sh_moon PRIVATE ${FG} opengl32 glu32)
      add_custom_command(TARGET 42sh_moon POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different ${FG_DLL} $<TARGET_FILE_DIR:42sh_moon>)
    else()
      target_link_libraries(42sh_moon PRIVATE GL GLU glut)
    endif()
  endif()
endif()
if(WIN32)
  target_link_libraries(42sh_moon PRIVATE ws2_32)
else()
  target_link_libraries(42sh_moon PRIVATE m)
endif()

add_library(moon_solver SHARED ../lunar/moon_core.c ../42/Kit/Source/orbkit.c ../42/Kit/Source/mathkit.c ../42/Kit/Source/dcmkit.c ../42/Kit/Source/timekit.c ../42/Kit/Source/iokit.c)
set_target_properties(moon_solver PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)
