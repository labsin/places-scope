# Macro to include arbitrary files in the QtCreator tree
#  when importing cmake
macro( qtcreator_add_project_resources resources )
    set(projectName "_QT_RESOURCES_UNIQUE_COUNTER_${PROJECT_NAME}")
    get_property(currentCounter GLOBAL PROPERTY "${projectName}")
    if(NOT currentCounter)
       set(currentCounter 1)
    endif()
    add_custom_target( _QT_RESOURCES_${PROJECT_NAME}_${currentCounter} ALL SOURCES ${ARGN} )
    math(EXPR currentCounter "${currentCounter} + 1")
    set_property(GLOBAL PROPERTY ${projectName} ${currentCounter} )
endmacro()
