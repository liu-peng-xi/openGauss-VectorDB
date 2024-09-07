set(EXTENSION "datavec")
set(EXTVERSION "0.7.2")

file(GLOB_RECURSE TGT_datavec_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
)

SET(TGT_datavec_INC
    ${PROJECT_SRC_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

set(datavec_DEF_OPTIONS ${MACRO_OPTIONS})
set(datavec_COMPILE_OPTIONS ${OPTIMIZE_OPTIONS} ${OS_OPTIONS} ${PROTECT_OPTIONS} ${WARNING_OPTIONS} ${LIB_SECURE_OPTIONS} ${CHECK_OPTIONS})
set(datavec_LINK_OPTIONS ${LIB_LINK_OPTIONS})

add_shared_libtarget(datavec TGT_datavec_SRC TGT_datavec_INC "${datavec_DEF_OPTIONS}" "${datavec_COMPILE_OPTIONS}" "${datavec_LINK_OPTIONS}")

set_target_properties(datavec PROPERTIES PREFIX "")

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/sql/${EXTENSION}--${EXTVERSION}.sql
    COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_CURRENT_SOURCE_DIR}/sql/${EXTENSION}.sql
            ${CMAKE_CURRENT_SOURCE_DIR}/sql/${EXTENSION}--${EXTVERSION}.sql
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/sql/${EXTENSION}.sql
    COMMENT "Copying ${CMAKE_CURRENT_SOURCE_DIR}/sql/${EXTENSION}.sql to ${CMAKE_CURRENT_SOURCE_DIR}/sql/${EXTENSION}--${EXTVERSION}.sql"
)

add_custom_target(
    CopyFile ALL
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/sql/${EXTENSION}--${EXTVERSION}.sql
)

set_directory_properties(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES \${SQL_FILE_PATH})

install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/${EXTENSION}.control
    DESTINATION share/postgresql/extension/
)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/sql/${EXTENSION}--${EXTVERSION}.sql
    DESTINATION share/postgresql/extension/
)

install(TARGETS datavec DESTINATION lib/postgresql)
