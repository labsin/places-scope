include(UseXGettext)

file(GLOB PO_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/*.po
)

qtcreator_add_project_resources(${PO_FILES})

add_translations_directory(${GETTEXT_PACKAGE})

add_translations_catalog(
  GETTEXT_PACKAGE ${GETTEXT_PACKAGE}
  SOURCE_DIRECTORIES "${CMAKE_SOURCE_DIR}/src" "${CMAKE_SOURCE_DIR}/data"
  EXTRA_SOURCES ${CMAKE_SOURCE_DIR}/data/com.ubuntu.developer.labsin.places_places.ini.in.h ${CMAKE_SOURCE_DIR}/data/com.ubuntu.developer.labsin.places_places-settings.ini.in.h
)

# Generating and later removing header from ini file for gettext
add_custom_command(
        OUTPUT ${CMAKE_SOURCE_DIR}/data/com.ubuntu.developer.labsin.places_places.ini.in.h ${CMAKE_SOURCE_DIR}/data/com.ubuntu.developer.labsin.places_places-settings.ini.in.h
        COMMAND intltool-extract --type=gettext/ini --update com.ubuntu.developer.labsin.places_places.ini.in
        COMMAND intltool-extract --type=gettext/ini --update com.ubuntu.developer.labsin.places_places-settings.ini.in
        DEPENDS ${CMAKE_SOURCE_DIR}/data/com.ubuntu.developer.labsin.places_places.ini.in ${CMAKE_SOURCE_DIR}/data/com.ubuntu.developer.labsin.places_places-settings.ini.in
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/data)

add_custom_target("ini_header" DEPENDS ${CMAKE_SOURCE_DIR}/data/com.ubuntu.developer.labsin.places_places.ini.in.h ${CMAKE_SOURCE_DIR}/data/com.ubuntu.developer.labsin.places_places-settings.ini.in.h)

add_dependencies("pot" "ini_header")

add_custom_command(
        COMMENT "Removing generated header"
        TARGET pot
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E remove ${CMAKE_SOURCE_DIR}/data/com.ubuntu.developer.labsin.places_places.ini.in.h ${CMAKE_SOURCE_DIR}/data/com.ubuntu.developer.labsin.places_places-settings.ini.in.h)
