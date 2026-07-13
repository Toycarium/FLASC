cmake_minimum_required(VERSION 3.15)
project(FLASC CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(FLASC
    src/main.cpp
    src/VehicleJsonBuilder.cpp
    src/HeliBoatRules.cpp
    src/CarSchema.cpp
    src/IniLoader.cpp
    src/JoaatHash.cpp
    src/Json.cpp
)

if(MSVC)
    # Static CRT so the .exe doesn't depend on the VC++ Redistributable being
    # installed on the user's machine - same "zero dependencies" goal we had
    # with the .NET version's Native AOT build.
    set_property(TARGET FLASC PROPERTY
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

    target_compile_options(FLASC PRIVATE
        $<$<CONFIG:Release>:/O1>   # optimize for size, not speed
        /GR-                       # no RTTI - we never use dynamic_cast/typeid
        /W4
    )
else()
    # Non-MSVC build (e.g. testing on Linux/macOS, or MinGW).
    target_compile_options(FLASC PRIVATE -Os -Wall -Wextra)
    target_link_options(FLASC PRIVATE -s)
endif()
