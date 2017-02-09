# Debian packaging
set(DEB_PACKAGE_COMPONENTS tools devel runtime)

set(DEB_PACKAGE_tools_NAME "${CMAKE_PROJECT_NAME}-tools")
set(DEB_PACKAGE_tools_DEPENDS "lib${CMAKE_PROJECT_NAME}")
set(DEB_PACKAGE_tools_DESRCIPTION "Iterlib tools package")

set(DEB_PACKAGE_runtime_NAME "lib${CMAKE_PROJECT_NAME}")
set(DEB_PACKAGE_runtime_DESRCIPTION "Iterlib package")

set(DEB_PACKAGE_devel_NAME "lib${CMAKE_PROJECT_NAME}-dev")
set(DEB_PACKAGE_devel_DESRCIPTION "Iterlib dev package")

include(deb_packaging)

