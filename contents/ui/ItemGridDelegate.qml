/***************************************************************************
 *   Copyright (C) 2015 by Eike Hein <hein@kde.org>                        *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA .        *
 ***************************************************************************/

import QtQuick

import org.kde.plasma.plasmoid
import org.kde.plasma.components 3.0 as PC3
import org.kde.kirigami as Kirigami
import org.kde.kquickcontrolsaddons
import Qt5Compat.GraphicalEffects

import "../code/tools.js" as Tools

Item {
    id: item

    width: isDummyApp ? 0 : GridView.view.cellWidth
    height: isDummyApp ? 0 : GridView.view.cellHeight

    property bool showLabel: true
    property var iconColorOverride: undefined
    property bool forceSymbolicIcons: false

    readonly property bool isDummyApp: model.url ? (model.url.toString().indexOf("plasma-drawer-dummy.desktop") !== -1) : false
    visible: !isDummyApp


    readonly property bool isDirectory: model.hasChildren ? true : false
    readonly property var directoryModel: isDirectory ? GridView.view.model.modelForRow(itemIndex) : undefined

    // For this widget, the only favoritable actions should be system actions
    readonly property bool isSystemAction: (model.favoriteId 
                                            && GridView.view.model.favoritesModel 
                                            && GridView.view.model.favoritesModel.enabled) ? true : false

    readonly property int itemIndex: model.index
    readonly property url url: model.url != undefined ? model.url : ""
    readonly property var icon: model.decoration
    property bool pressed: false
    
    readonly property bool isDragged: typeof kicker !== "undefined" && kicker.dragSource === item

    property bool isDropTarget: false

    scale: pressed || isDragged || isDropTarget ? 1.05 : 1.0
    opacity: isDragged ? 0.6 : 1.0
    Behavior on scale { NumberAnimation { duration: Kirigami.Units.shortDuration; easing.type: Easing.OutQuad } }
    Behavior on opacity { NumberAnimation { duration: Kirigami.Units.shortDuration; easing.type: Easing.OutQuad } }

    Rectangle {
        anchors.fill: parent
        anchors.margins: Kirigami.Units.smallSpacing
        radius: Kirigami.Units.smallSpacing
        color: Kirigami.Theme.highlightColor
        opacity: isDropTarget ? 0.3 : 0.0
        Behavior on opacity { NumberAnimation { duration: Kirigami.Units.shortDuration; easing.type: Easing.OutQuad } }
        z: -1
    }

    Accessible.role: Accessible.MenuItem
    Accessible.name: model.display

    MouseArea {
        id: itemMouseArea
        anchors.fill: parent
        hoverEnabled: isSystemAction
        acceptedButtons: Qt.NoButton
    }



    readonly property bool hasActionList: ((("hasActionList" in model) && (model.hasActionList == true))
                                           || isSystemAction || isDirectory)
    
    function getActionList() {
        var actions = isSystemAction ? Tools.createSystemActionActions(i18n, GridView.view.model.favoritesModel, model.favoriteId) : model.actionList;
        if (isDirectory && !isSystemAction) {
            var newActions = [];
            if (actions && actions.length > 0) {
                for (var i = 0; i < actions.length; ++i) {
                    newActions.push(actions[i]);
                }
                newActions.push({ type: "separator" });
            }
            newActions.push({
                text: i18n("Rename Folder..."),
                icon: "edit-rename",
                actionId: "_plasmaDrawer_rename_folder",
                actionArgument: model.url ? model.url.toString() : (model.favoriteId || model.name || model.display)
            });
            newActions.push({
                text: i18n("Delete Folder"),
                icon: "edit-delete",
                actionId: "_plasmaDrawer_delete_folder",
                actionArgument: model.url ? model.url.toString() : (model.favoriteId || model.name || model.display)
            });

            return newActions;
        } else if (!isDirectory && !isSystemAction) {
            var newAppActions = [];
            if (actions && actions.length > 0) {
                for (var j = 0; j < actions.length; ++j) {
                    newAppActions.push(actions[j]);
                }
                newAppActions.push({ type: "separator" });
            }
            
            var isOnlyApp = false;
            if (typeof menuEditorBackend !== "undefined") {
                isOnlyApp = !menuEditorBackend.hasMoreThanOneInstance(model.url ? model.url.toString() : model.favoriteId);
            }
            
            newAppActions.push({
                text: i18n("Duplicate"),
                icon: "edit-copy",
                actionId: "_plasmaDrawer_duplicate_app",
                actionArgument: { url: model.url ? model.url.toString() : model.favoriteId, menuEditorBackend: typeof menuEditorBackend !== "undefined" ? menuEditorBackend : null, folderId: GridView.view.folderId || "" }
            });
            newAppActions.push({
                text: i18n("Delete"),
                icon: "edit-delete",
                actionId: "_plasmaDrawer_delete_app",
                actionArgument: { url: model.url ? model.url.toString() : model.favoriteId, menuEditorBackend: typeof menuEditorBackend !== "undefined" ? menuEditorBackend : null, folderId: GridView.view.folderId || "" },
                enabled: !isOnlyApp
            });
            return newAppActions;
        }
        return actions;
    }







    Component {
        id: iconComponent

        Item {
            anchors.centerIn: parent
            Kirigami.Icon {
                id: icon
                anchors.centerIn: parent
                width: parent.width
                height: parent.height
                animated: false
                source: model.decoration + (forceSymbolicIcons ? "-symbolic" : "")
                roundToIconSize: width > Kirigami.Units.iconSizes.huge ? false : true
                isMask: iconColorOverride !== undefined
                color: iconColorOverride ?? "transparent"
            }
        }
    }

    Component {
        id: directoryViewComponent

        Rectangle {
            id: directoryBackgroundBox

            // anchors.fill: parent
            radius: width / 4
            //border.color: Kirigami.Theme.textColor
            //border.width: 2
            color: "#33000005"

            GridView {
                id: directoryGridView
                
                anchors.fill: parent
                anchors.margins: parent.width / 10
                cellWidth: (width / 2) * 0.9 > Kirigami.Units.iconSizes.small ? width / 2 : width
                cellHeight: cellWidth
                
                z: 1 // Make in front of background
                interactive: false
                clip: true

                model: directoryModel
                delegate: Item {
                    readonly property bool isDummyApp: model.url ? (model.url.toString().indexOf("plasma-drawer-dummy.desktop") !== -1) : false
                    visible: !isDummyApp

                    width: isDummyApp ? 0 : directoryGridView.cellWidth
                    height: isDummyApp ? 0 : directoryGridView.cellHeight

                    Kirigami.Icon {
                        id: directoryIconItem

                        width: parent.width * 0.9
                        height: parent.height * 0.9
                        anchors.centerIn: parent

                        animated: false
                        source: model.decoration + (forceSymbolicIcons ? "-symbolic" : "")
                        roundToIconSize: width > Kirigami.Units.iconSizes.medium ? false : true
                    }
                }
            }
        }
    }

    Rectangle {
        id: displayBox
        width: iconSize
        height: width
        y: (item.height / 2) - (height / 2) - (showLabel ? label.height / 2 : 0)
        anchors.horizontalCenter: parent.horizontalCenter
        // anchors.verticalCenter: parent.verticalCenter - (showLabel ? label.height / 2 : 0)
        color:"transparent"

        // Load either icon or directory view
        Loader {
            id: displayLoader
            anchors.fill: parent
            sourceComponent: isDirectory && !plasmoid.configuration.useDirectoryIcons ? directoryViewComponent : iconComponent
        }        
    }



    PC3.Label {
        id: label

        visible: showLabel

        anchors {
            top: displayBox.bottom
            topMargin: Kirigami.Units.largeSpacing * 1.5
            left: parent.left
            leftMargin: highlightItemSvg.margins.left
            right: parent.right
            rightMargin: highlightItemSvg.margins.right
        }

        horizontalAlignment: Text.AlignHCenter

        elide: Text.ElideRight
        wrapMode: Text.NoWrap

        text: model.display
        color: drawerTheme.textColor
    }

}
