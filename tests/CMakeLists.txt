# Tests for ps2stuff

# VU0 path: test_shared.cpp compiled with VU0 assembly (default)
add_executable(test_vu0.elf
	test_shared.cpp
)

target_include_directories(test_vu0.elf PRIVATE
	${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(test_vu0.elf
	ps2stuff
	m
)

find_program(PS2CLIENT_EXECUTABLE ps2client)
if(PS2CLIENT_EXECUTABLE)
	add_custom_target(run_test_vu0
		COMMAND ${PS2CLIENT_EXECUTABLE} execee host:test_vu0.elf
		DEPENDS test_vu0.elf
		WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
		COMMENT "Running test_vu0.elf on PS2 via ps2client"
	)
endif()

message(STATUS "Configured test target: test_vu0.elf")

add_executable(test_cpu_shared.elf
	test_shared.cpp
)

target_include_directories(test_cpu_shared.elf PRIVATE
	${CMAKE_SOURCE_DIR}/include
)

target_compile_definitions(test_cpu_shared.elf PRIVATE
	USE_CPU_COMPAT
)

target_link_libraries(test_cpu_shared.elf
	ps2stuff
	m
)

if(PS2CLIENT_EXECUTABLE)
	add_custom_target(run_test_cpu_shared
		COMMAND ${PS2CLIENT_EXECUTABLE} execee host:test_cpu_shared.elf
		DEPENDS test_cpu_shared.elf
		WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
		COMMENT "Running test_cpu_shared.elf on PS2 via ps2client"
	)
endif()

message(STATUS "Configured test target: test_cpu_shared.elf")
