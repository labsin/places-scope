# Put the ini file in the build directory next to the scope
# .so file so test tools can find both easily.
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/com.ubuntu.developer.labsin.places_places.ini"
    "${CMAKE_BINARY_DIR}/src/com.ubuntu.developer.labsin.places_places.ini"
    @ONLY
)
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/com.ubuntu.developer.labsin.places_places-settings.ini"
    "${CMAKE_BINARY_DIR}/src/com.ubuntu.developer.labsin.places_places-settings.ini"
    @ONLY
)

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/logo.png"
    "${CMAKE_BINARY_DIR}/src/logo.png"
    @ONLY
    COPYONLY
)

# Install the scope ini file
install(
  FILES "com.ubuntu.developer.labsin.places_places.ini"
        "com.ubuntu.developer.labsin.places_places-settings.ini"
  DESTINATION ${SCOPE_INSTALL_DIR}
)

# Install the scope images
install(
  FILES
    "icon.png"
    "logo.png"
    "places.jpg"
    "screenshot.png"
  DESTINATION
    "${SCOPE_INSTALL_DIR}"
)

