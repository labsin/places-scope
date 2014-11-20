include(UseXGettext)

add_translations_directory(${GETTEXT_PACKAGE})

add_translations_catalog(
  GETTEXT_PACKAGE ${GETTEXT_PACKAGE}
  SOURCE_DIRECTORIES "${CMAKE_SOURCE_DIR}/src" "${CMAKE_SOURCE_DIR}/data"
)

# Generating and later removing header from ini file for gettext
add_custom_command(
        OUTPUT ${CMAKE_SOURCE_DIR}/data/com.ubuntu.developer.labsin.places_places.ini.in.h com.ubuntu.developer.labsin.places_places-settings.ini.in
        COMMAND intltool-extract --type=gettext/ini --update com.ubuntu.developer.labsin.places_places.ini.in
        COMMAND intltool-extract --type=gettext/ini --update com.ubuntu.developer.labsin.places_places-settings.ini.in
        DEPENDS ${CMAKE_SOURCE_DIR}/data/com.ubuntu.developer.labsin.places_places.ini.in
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/data)

add_custom_target("ini_header" ALL DEPENDS ${CMAKE_SOURCE_DIR}/data/com.ubuntu.developer.labsin.places_places.ini.in.h ${CMAKE_SOURCE_DIR}/data/com.ubuntu.developer.labsin.places_places-settings.ini.in.h)

add_dependencies("pot" "ini_header")

add_custom_command(
        COMMENT "Removing generated header"
        TARGET pot
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E remove ${CMAKE_SOURCE_DIR}/data/com.ubuntu.developer.labsin.places_places.ini.in.h ${CMAKE_SOURCE_DIR}/data/com.ubuntu.developer.labsin.places_places-settings.ini.in.h)
