# Usage of this module as follows:
#
#     find_package(RxCpp)
#
# Variables used by this module, they can change the default behaviour and need
# to be set before calling find_package:
#
#  RXCPP_ROOT_DIR Set this variable to the root installation of
#                    rxcpp if the module has problems finding
#                    the proper installation path.
#
# Variables defined by this module:
#
#  RXCPP_FOUND             System has rxcpp libs/headers
#  RXCPP_INCLUDE_DIR       The location of rxcpp headers

find_path(RXCPP_ROOT_DIR
    NAMES rxcpp/rx.hpp
)

find_path(RXCPP_INCLUDE_DIR
    NAMES rx.hpp
    HINTS ${RXCPP_ROOT_DIR}/rxcpp
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(RxCpp DEFAULT_MSG
    RXCPP_ROOT_DIR
    RXCPP_INCLUDE_DIR
)

mark_as_advanced(
    RXCPP_ROOT_DIR
    RXCPP_INCLUDE_DIR
)
