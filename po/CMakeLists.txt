include(UseXGettext)

add_translations_directory(${GETTEXT_PACKAGE})

add_translations_catalog(
  GETTEXT_PACKAGE ${GETTEXT_PACKAGE}
  SOURCE_DIRECTORIES "${CMAKE_SOURCE_DIR}/src" "${CMAKE_SOURCE_DIR}/data"
)

# Generating and later removing header from ini file for gettext
add_custom_command(
        OUTPUT ${CMAKE_SOURCE_DIR}/data/com.ubuntu.developer.labsin.places_places.ini.h
        COMMAND intltool-extract --type=gettext/ini --update data/com.ubuntu.developer.labsin.places_places.ini
        DEPENDS ${CMAKE_SOURCE_DIR}/data/com.ubuntu.developer.labsin.places_places.ini
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

add_custom_target("init_header" ALL DEPENDS ${CMAKE_SOURCE_DIR}/data/com.ubuntu.developer.labsin.places_places.ini.h)

add_dependencies("pot" "init_header")

add_custom_command(
        TARGET "pot"
        COMMAND ${CMAKE_COMMAND} -E remove ${CMAKE_SOURCE_DIR}/data/com.ubuntu.developer.labsin.places_places.ini.h
        DEPENDS ${CMAKE_SOURCE_DIR}/data/com.ubuntu.developer.labsin.places_places.ini.h)
