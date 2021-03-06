cmake_minimum_required(VERSION 2.8)
set(BRIDGE_NPBACKEND true CACHE BOOL "BRIDGE-NPBACKEND: Build the NPBACKEND-bridge.")
if(NOT BRIDGE_NPBACKEND)
    return()
endif()

find_package(NumPy)
set_package_properties(NumPy PROPERTIES DESCRIPTION "Numerical Python" URL "www.numpy.org")
set_package_properties(NumPy PROPERTIES TYPE RECOMMENDED PURPOSE "Enables the Python/NumPy bridge (together with Python and SWIG).")

find_package(SWIG)
set_package_properties(SWIG PROPERTIES DESCRIPTION "Simplified Wrapper and Interface Generator" URL "www.swig.org")
set_package_properties(SWIG PROPERTIES TYPE RECOMMENDED PURPOSE "Enables the Python/NumPy bridge (together with Python and NumPy).")

find_package(Cython)
set_package_properties(Cython PROPERTIES DESCRIPTION "C-Extensions for Python" URL "cython.org")
set_package_properties(Cython PROPERTIES TYPE RECOMMENDED PURPOSE "Enables the Python/NumPy bridge (together with Python and SWIG).")

if(PYTHONINTERP_FOUND AND NUMPY_FOUND AND SWIG_FOUND AND CYTHON_FOUND AND NUMPY_INCLUDE_DIRS)

    # Generate distutils package and install it
    add_custom_target(
        bhpy ALL
        COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/setup.py build_ext --include-dirs=${NUMPY_INCLUDE_DIRS} buildpath=${CMAKE_CURRENT_BINARY_DIR} && ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/setup.py install --prefix=${CMAKE_CURRENT_BINARY_DIR}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        DEPENDS bhc
    )
    install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${LIBDIR} DESTINATION . COMPONENT bohrium)
else()
    if (NOT NUMPY_INCLUDE_DIRS)
        message(STATUS "The Python/NumPy bridge cannot be built, because the numpy headers are missing")
    else()
        message(STATUS "The Python/NumPy bridge cannot be built")
    endif()
endif()

set(NO_PYTHON3 false CACHE BOOL "Never try to do a Python3 build alongside a Python2 build.")
if(NO_PYTHON3)
    return()
endif()

# If the default python version is version 2 we will try a version 3 build as well
if(${PYTHON_VERSION_MAJOR} EQUAL "2")
    find_program(PYTHON3 python3)
    if(PYTHON3)
        message(STATUS ${PYTHON3})
    endif()

    find_program(CYTHON3 cython3)
    if(CYTHON3)
        message(STATUS ${CYTHON3})
    endif()

    execute_process(COMMAND "${PYTHON3}" "-c" "import numpy as n;"
                    RESULT_VARIABLE NUMPY3)
    if(${NUMPY3} EQUAL "0" AND PYTHON3 AND CYTHON3)
        message(STATUS "Found Python3, Cython3, and NumPy3")

        # Generate distutils package and install it
        add_custom_target(
                bhpy3 ALL
                COMMAND ${PYTHON3} ${CMAKE_CURRENT_SOURCE_DIR}/setup.py build_ext --include-dirs=${NUMPY_INCLUDE_DIRS} buildpath=${CMAKE_CURRENT_BINARY_DIR} && ${PYTHON3} ${CMAKE_CURRENT_SOURCE_DIR}/setup.py install --prefix=${CMAKE_CURRENT_BINARY_DIR}
                WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
                DEPENDS bhc
        )
        install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${LIBDIR}
                DESTINATION .
                COMPONENT bohrium3
                PATTERN "python2.*" EXCLUDE) # we don't want python2 installed again
    endif()
endif()

