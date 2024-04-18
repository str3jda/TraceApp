vcpkg_minimum_required(VERSION 2022-10-12)

vcpkg_check_linkage(ONLY_STATIC_LIBRARY)

vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO  str3jda/TraceApp 
    REF "${VERSION}"
    SHA512 0
    HEAD_REF master
)
#a550b147045b930d3c6a5b5c83c764bdd2621178
vcpkg_cmake_configure( SOURCE_PATH "${SOURCE_PATH}" PREFER_NINJA )

vcpkg_cmake_install()
vcpkg_fixup_pkgconfig()

vcpkg_cmake_config_fixup(PACKAGE_NAME "trace")