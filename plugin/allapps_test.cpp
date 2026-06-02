#include <QGuiApplication>
#include <KServiceGroup>
#include <KService>
#include <iostream>

int main(int argc, char **argv) {
    QGuiApplication app(argc, argv);
    QGuiApplication::setApplicationName(QStringLiteral("allapps_test"));
    KServiceGroup::Ptr root = KServiceGroup::root();
    if (root) {
        for (auto entry : root->entries()) {
            if (entry->isType(KST_KService)) {
                KService::Ptr service(static_cast<KService*>(entry.data()));
                std::cout << "SERVICE: " << service->name().toStdString() << std::endl;
            } else if (entry->isType(KST_KServiceGroup)) {
                KServiceGroup::Ptr group(static_cast<KServiceGroup*>(entry.data()));
                std::cout << "GROUP: " << group->caption().toStdString() << std::endl;
            }
        }
    } else {
        std::cout << "ROOT IS NULL" << std::endl;
    }
    return 0;
}
