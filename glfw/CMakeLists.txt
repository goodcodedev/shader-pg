fips_begin_lib(flextgl)
    fips_vs_warning_level(3)
    fips_src(flextgl)
fips_end_lib()

fips_begin_app(shader-pg cmdline)
    fips_vs_warning_level(3)
    fips_files(shader-pg.cpp)
    fips_deps(shader-things glfw3 flextgl)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DPROJECT_ROOT=\\\"${FIPS_PROJECT_DIR}\\\"")
fips_end_app()