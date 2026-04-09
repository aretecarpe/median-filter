if(GPU_BACKEND STREQUAL "HIP")
	enable_language(HIP)
else()
	enable_language(CUDA)
endif()

set(TARGET_NAME gungnir.median_filter.example)
add_executable(${TARGET_NAME})

target_sources(
	${TARGET_NAME}
	PRIVATE
	median_filter2d.cpp
)

if(GPU_BACKEND STREQUAL "HIP")
	set_source_files_properties(median_filter2d.cpp PROPERTIES LANGUAGE HIP)
	setup_hip(
		${TARGET_NAME}
		STANDARD ${CMAKE_CXX_STANDARD}
		ARCHITECTURES "gfx942"
	)
else()
	set_source_files_properties(median_filter2d.cpp PROPERTIES LANGUAGE CUDA)
	setup_cuda(
		${TARGET_NAME}
		STATIC_RUNTIME
		STANDARD ${CMAKE_CXX_STANDARD}
		ARCHITECTURES "87"
		PER_THREAD_STREAMS
	)
endif()

CPMAddPackage("gh:aretecarpe/tiffxx#master")
target_link_libraries(
	${TARGET_NAME}
	PRIVATE
		gungnir::median_filter
		gungnir::tiffxx
)

set_project_warnings(
	${TARGET_NAME}
	WARNINGS_AS_ERRORS
)
