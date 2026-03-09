idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS "."
                    EMBED_TXTFILES ${project_dir}/ca_cert.pem)

idf_build_get_property(python_exe PYTHON)

add_custom_target(update_version ALL
    COMMAND ${python_exe} ${CMAKE_SOURCE_DIR}/../versioning.py
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Running versioning.py to bump build number and generate version.h..."
)

add_dependencies(${COMPONENT_LIB} update_version)