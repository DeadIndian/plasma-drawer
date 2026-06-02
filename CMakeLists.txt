cmake_minimum_required(VERSION 3.16)
project(plasma-drawer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(ECM 6.0.0 REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})
include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)

find_package(Qt6 REQUIRED COMPONENTS Core Qml Gui Xml)
find_package(KF6 REQUIRED COMPONENTS Service CoreAddons I18n Config)

set(CMAKE_AUTOMOC ON)

add_subdirectory(plugin)

install(FILES metadata.json DESTINATION ${KDE_INSTALL_DATADIR}/plasma/plasmoids/p-connor.plasma-drawer)
install(DIRECTORY contents DESTINATION ${KDE_INSTALL_DATADIR}/plasma/plasmoids/p-connor.plasma-drawer)

# CPack configuration for GitHub Actions
set(CPACK_PACKAGE_NAME "plasma-drawer")
set(CPACK_PACKAGE_VERSION "1.0.0")
set(CPACK_GENERATOR "DEB;RPM")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Plasma Drawer Team")
include(CPack)
