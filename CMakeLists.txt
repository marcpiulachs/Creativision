# creatiVision cross platform cmake script
#
# Create build directory within the source folder
# then change to it.
#
# On windows for x86
# cmake -G "MSYS Makefiles" -DBITS=32 ../
# or for x64
# cmake -G "MSYS Makefiles" -DBITS=64 ../
#

cmake_minimum_required(VERSION 2.8 FATAL_ERROR)
project("creatiVision emulator")

# Generate platform.h
set(FAKE_TARGET fakeTarget)
add_custom_target(fakeTarget sh ${CMAKE_SOURCE_DIR}/gp.sh ${CMAKE_SOURCE_DIR}/platform.h COMMENT Generating platform.h)

# Set compile flags
set(CMAKE_C_FLAGS "-Wall -static-libgcc -Wno-sign-compare -Os")

# Utilities
add_executable(bas2cas utils/bas2cas/main.c)
add_executable(txt2cas utils/txt2cas/main.c)
add_executable(cas2wav utils/cas2wav/main.c)
add_executable(cvgfx utils/cvgfx/cvgfx.c)
add_executable(smbloader utils/smbloader/smbloader.c)

target_link_libraries(cas2wav m)

set(UTILS_DIR /utils)

# Windows builds only tested with MinGW-W64
if(WIN32)

    set(EXE_SUFFIX .exe)

    # Set compile flags
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mno-ms-bitfields -s -O3")

	# Get architecture
	if(BITS STREQUAL 32)
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -march=i686")
	endif()

	if(BITS STREQUAL 64)
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -march=x86-64")
	endif()
	
    # Set gcc definitions
    add_definitions(-DLSB_FIRST -DBPP32 -DDEBUG -DWIN32 -D_REENTRANT)

    # Set source files, including resource
    add_executable(creatiVision cassette.c cpu_intrf.c Debug.c demorec.c DRV9918.c joystick.c laserdos.c M6502.c main.c memory.c netplay.c pia_intf.c png.c printer.c sn76496sp.c snapshot.c snd_intrf.c TMS9918.c vdp_intrf.c creatiVision.rc)

    # Set libs
    target_link_libraries(creatiVision mingw32 SDL2main SDL2 SDL2_net png z)

else()

    # Linux (Ubuntu, Debian)

    # Set gcc definitions
    if(CMAKE_SYSTEM_PROCESSOR STREQUAL armv6l)
        add_definitions(-DLSB_FIRST -DBPP32 -DDEBUG -DRPI -D_REENTRANT)
    else()
        add_definitions(-DLSB_FIRST -DBPP32 -DDEBUG -D_REENTRANT)
		
		# Get architecture
		if(BITS STREQUAL 32)
			set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -march=i686")
		endif()
	
		if(BITS STREQUAL 64)
			set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -march=x86-64")
		endif()
		
    endif()
	
    # Set source files
    add_executable(creatiVision cassette.c cpu_intrf.c Debug.c demorec.c DRV9918.c joystick.c laserdos.c M6502.c main.c memory.c netplay.c pia_intf.c png.c printer.c sn76496sp.c snapshot.c snd_intrf.c TMS9918.c vdp_intrf.c)

    # Set libs
    target_link_libraries(creatiVision SDL2main SDL2 SDL2_net png z)

endif()

# Strip for release
add_custom_command(TARGET creatiVision POST_BUILD COMMAND ${CMAKE_STRIP} creatiVision${EXE_SUFFIX})
add_custom_command(TARGET bas2cas POST_BUILD COMMAND ${CMAKE_STRIP} utils/bas2cas${EXE_SUFFIX})
add_custom_command(TARGET txt2cas POST_BUILD COMMAND ${CMAKE_STRIP} utils/txt2cas${EXE_SUFFIX})
add_custom_command(TARGET cas2wav POST_BUILD COMMAND ${CMAKE_STRIP} utils/cas2wav${EXE_SUFFIX})
add_custom_command(TARGET cvgfx POST_BUILD COMMAND ${CMAKE_STRIP} utils/cvgfx${EXE_SUFFIX})
add_custom_command(TARGET smbloader POST_BUILD COMMAND ${CMAKE_STRIP} utils/smbloader${EXE_SUFFIX})

add_dependencies(creatiVision fakeTarget)
set_target_properties(bas2cas txt2cas cas2wav cvgfx smbloader PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}${UTILS_DIR}")
