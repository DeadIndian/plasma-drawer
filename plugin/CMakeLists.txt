add_library(plasma_drawer_plugin SHARED
    menueditorbackend.cpp
    allappsmodel.cpp
    plugin.cpp
)

target_link_libraries(plasma_drawer_plugin
    Qt::Core
    Qt::Qml
    Qt::Gui
    Qt::Xml
    KF6::Service
    KF6::ConfigCore
)

install(TARGETS plasma_drawer_plugin DESTINATION ${KDE_INSTALL_QMLDIR}/org/kde/plasma/private/plasma_drawer)
install(FILES qmldir DESTINATION ${KDE_INSTALL_QMLDIR}/org/kde/plasma/private/plasma_drawer)
add_executable(allapps_test allapps_test.cpp)
target_link_libraries(allapps_test Qt::Core Qt::Gui KF6::Service KF6::ConfigCore)
