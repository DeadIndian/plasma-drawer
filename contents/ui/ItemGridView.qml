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
import QtQuick.Controls

import org.kde.plasma.components 3.0 as PC3
import org.kde.plasma.extras as PlasmaExtras
import org.kde.kquickcontrolsaddons
import org.kde.draganddrop

import "../code/tools.js" as Tools

FocusScope {
    id: itemGrid

    signal keyNavLeft
    signal keyNavRight
    signal keyNavUp
    signal keyNavDown

    property bool dragEnabled: true
    property bool showLabels: true
    property bool setIconColorBasedOnTheme: false
    property bool forceSymbolicIcons: false

    property int iconSize: Kirigami.Units.iconSizes.large

    property int numberColumns: Math.floor(width / cellWidth)
    property int maxVisibleRows: -1
    readonly property int numberRows: Math.ceil(count / numberColumns)
    property alias cellWidth: gridView.cellWidth
    property alias cellHeight: gridView.cellHeight

    property alias model: gridView.model

    // Id of the folder this grid shows ("" at root). Read from the model, which
    // carries it (see DrawerModel); kicker models don't have it, hence the guard.
    readonly property string folderId: {
        try {
            return gridView.model.folderId || "";
        } catch (e) {
            return "";
        }
    }

    property alias currentIndex: gridView.currentIndex
    property alias currentItem: gridView.currentItem
    property alias contentItem: gridView.contentItem
    property alias contentY: gridView.contentY
    property alias count: gridView.count
    property alias flow: gridView.flow
    property alias snapMode: gridView.snapMode

    property alias hoverEnabled: mouseArea.hoverEnabled

    property alias populateTransition: gridView.populate

    // ScrollView needs additional space on the right for the scrollbar,
    // so we add additional padding on the left to center the gridview
    implicitWidth: scrollView.width + scrollView.ScrollBar.vertical.width
    implicitHeight: scrollView.height

    onFocusChanged: {
        if (!focus) {
            currentIndex = -1;
        }
    }

    function itemAtIndex(index) {
        return gridView.itemAtIndex(index);
    }

    function currentRow() {
        if (currentIndex == -1) {
            return -1;
        }

        return Math.floor(currentIndex / numberColumns);
    }

    function currentCol() {
        if (currentIndex == -1) {
            return -1;
        }

        return currentIndex - (currentRow() * numberColumns);
    }

    function lastRow() {
        return numberRows - 1;
    }

    function trySelect(row, col) {
        if (count) {
            // Constrains between 0 and numberRows - 1
            row = Math.min(Math.max(row, 0), numberRows - 1);
            col = Math.min(Math.max(col, 0), numberColumns - 1);
            currentIndex = Math.min(((row * numberColumns) + col), count - 1);

            gridView.forceActiveFocus();
        }
    }

    function trigger(index) {
        if (gridView.model.modelForRow(index) != null) {
            appsGrid.tryEnterDirectory(index);
        } else if ("trigger" in gridView.model) {
            gridView.model.trigger(index, "", null);
            root.toggle();
        }
    }

    function forceLayout() {
        gridView.forceLayout();
    }

    ActionMenu {
        id: actionMenu

        property int targetIndex: -1

        visualParent: gridView
        
        onActionClicked: function (actionId, actionArgument) {
            if (actionId === "_plasmaDrawer_rename") {
                var delegate = gridView.itemAtIndex(targetIndex);
                if (delegate && delegate.startRename) {
                    delegate.startRename();
                }
                return;
            }

            var closeRequested = Tools.triggerAction(plasmoid, model, targetIndex, actionId, actionArgument);
            if (closeRequested) {
                root.toggle();
            }
        }

        onClosed: {
            currentIndex = -1;
        }
    }

    function openActionMenu(x, y, actionList) {
        if (actionList && "length" in actionList && actionList.length > 0) {
            actionMenu.actionList = actionList;
            actionMenu.targetIndex = currentIndex;
            actionMenu.open(x, y);
        }
    }

    DropArea {
        id: dropArea

        width: numberColumns * cellWidth
        height: (maxVisibleRows == -1 ? numberRows : maxVisibleRows) * cellHeight
        anchors.centerIn: parent

        property var currentDropTarget: null

        function clearDropTarget() {
            if (currentDropTarget) {
                currentDropTarget.isDropTarget = false;
                currentDropTarget = null;
            }
        }

        onDragEnter: function(event) {
            event.action = Qt.CopyAction;
            event.accept(Qt.CopyAction);
        }

        onDragMove: function (event) {
            var cPos = mapToItem(gridView.contentItem, event.x, event.y);
            var item = gridView.itemAt(cPos.x, cPos.y);

            if (dropArea.currentDropTarget && dropArea.currentDropTarget !== item) {
                dropArea.clearDropTarget();
            }

            if (item && item.isDirectory && kicker.draggedAppData && !kicker.draggedAppData.isDirectory) {
                // Hovering an app over a folder: mark it as the drop target.
                event.action = Qt.CopyAction;
                event.accept(Qt.CopyAction);
                if (!dropArea.currentDropTarget) {
                    dropArea.currentDropTarget = item;
                    item.isDropTarget = true;
                }
            } else if (!item && kicker.draggedAppData && !kicker.draggedAppData.isDirectory && appsGrid.isAtRoot) {
                // Empty root space: dropping here pulls the app out of its folder.
                event.action = Qt.CopyAction;
                event.accept(Qt.CopyAction);
            } else if (item && item != kicker.dragSource && kicker.dragSource
                       && kicker.dragSource.parent == gridView.contentItem
                       && "moveRow" in gridView.model) {
                // Dragging within a single grid: live-reorder. The model's
                // moveRow moves the row and persists the new order.
                gridView.model.moveRow(kicker.dragSource.itemIndex, item.itemIndex);
                event.action = Qt.MoveAction;
                event.accept(Qt.MoveAction);
            } else {
                event.ignore();
            }
        }

        onDragLeave: function(event) {
            dropArea.clearDropTarget();
        }

        onDrop: function(event) {
            dropArea.clearDropTarget();

            var cPos = mapToItem(gridView.contentItem, event.x, event.y);
            var item = gridView.itemAt(cPos.x, cPos.y);

            if (item && item.isDirectory && kicker.draggedAppData && !kicker.draggedAppData.isDirectory) {
                // Drop an app onto a folder.
                drawerModel.moveAppToFolder(kicker.draggedAppData.storageId, item.folderId);
                event.accept(Qt.CopyAction);
            } else if (!item && kicker.draggedAppData && !kicker.draggedAppData.isDirectory && appsGrid.isAtRoot) {
                // Drop an app onto empty root space: pull it out of its folder.
                drawerModel.removeAppFromFolder(kicker.draggedAppData.storageId);
                event.accept(Qt.CopyAction);
            }
            // Same-grid reorders already happened live during onDragMove.

            kicker.resetDragSource();
        }

        PC3.ScrollView {
            id: scrollView
            width: (numberColumns * cellWidth) + ScrollBar.vertical.width
            height: parent.height
            anchors.left: parent.left
            anchors.verticalCenter: parent.verticalCenter

            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
            ScrollBar.vertical.interactive: true

            focus: true

            GridView {
                id: gridView
                width: numberColumns * cellWidth
                height: parent.height
                // anchors.left: parent.left
                // anchors.verticalCenter: parent.verticalCenter

                focus: true
                visible: model ? model.count > 0 : false
                currentIndex: -1
                // clip: true

                keyNavigationWraps: false
                boundsBehavior: Flickable.StopAtBounds
                snapMode: GridView.SnapToRow
                flickDeceleration: 4000

                highlightFollowsCurrentItem: true
                highlight: PlasmaExtras.Highlight {
                    visible: gridView.highlightFollowsCurrentItem
                }
                highlightMoveDuration: 0

                delegate: ItemGridDelegate {
                    showLabel: showLabels
                    iconColorOverride: setIconColorBasedOnTheme && drawerTheme.usingCustomTheme ? drawerTheme.iconColor : undefined
                    forceSymbolicIcons: itemGrid.forceSymbolicIcons
                }

                onModelChanged: {
                    currentIndex = -1;
                }

                Keys.onLeftPressed: function (event) {
                    if (currentIndex == -1) {
                        currentIndex = 0;
                        return;
                    }

                    if (!(event.modifiers & Qt.ControlModifier) && currentCol() != 0) {
                        event.accepted = true;
                        moveCurrentIndexLeft();
                    } else {
                        itemGrid.keyNavLeft();
                    }
                }

                Keys.onRightPressed: function (event) {
                    if (currentIndex == -1) {
                        currentIndex = 0;
                        return;
                    }

                    var columns = Math.floor(width / cellWidth);

                    if (!(event.modifiers & Qt.ControlModifier) && currentCol() != columns - 1 && currentIndex != count - 1) {
                        event.accepted = true;
                        moveCurrentIndexRight();
                    } else {
                        itemGrid.keyNavRight();
                    }
                }

                Keys.onUpPressed: function (event) {
                    if (currentIndex == -1) {
                        currentIndex = 0;
                        return;
                    }

                    if (currentRow() != 0) {
                        event.accepted = true;
                        moveCurrentIndexUp();
                        positionViewAtIndex(currentIndex, GridView.Contain);
                    } else {
                        itemGrid.keyNavUp();
                    }
                }

                Keys.onDownPressed: function (event) {
                    if (currentIndex == -1) {
                        currentIndex = 0;
                        return;
                    }

                    if (currentRow() < itemGrid.lastRow()) {
                        // Fix moveCurrentIndexDown()'s lack of proper spatial nav down
                        // into partial columns.
                        event.accepted = true;
                        var columns = Math.floor(width / cellWidth);
                        var newIndex = currentIndex + columns;
                        currentIndex = Math.min(newIndex, count - 1);
                        positionViewAtIndex(currentIndex, GridView.Contain);
                    } else {
                        itemGrid.keyNavDown();
                    }
                }

                Keys.onPressed: function (event) {
                    if (event.key == Qt.Key_Menu && currentItem && currentItem.hasActionList) {
                        event.accepted = true;
                        openActionMenu(currentItem.x, currentItem.y, currentItem.getActionList());
                        return;
                    } 
                    if ((event.key == Qt.Key_Enter || event.key == Qt.Key_Return && currentIndex != -1)) {
                        event.accepted = true;
                        itemGrid.trigger(currentIndex);
                        // root.toggle();
                    }

                    let rowsInPage = Math.floor(gridView.height / cellHeight);

                    if (event.key == Qt.Key_PageUp) {
                        if (currentIndex == -1) {
                            currentIndex = 0;
                            return;
                        }

                        if (currentRow() != 0) {
                            event.accepted = true;
                            trySelect(currentRow() - rowsInPage, currentCol());
                            positionViewAtIndex(currentIndex, GridView.Beginning);
                        } else {
                            itemGrid.keyNavUp();
                        }
                        return;
                    }
                    
                    if (event.key == Qt.Key_PageDown) {
                        if (currentIndex == -1) {
                            currentIndex = 0;
                            return;
                        }

                        if (currentRow() != numberRows - 1) {
                            event.accepted = true;
                            trySelect(currentRow() + rowsInPage, currentCol());
                            positionViewAtIndex(currentIndex, GridView.Beginning);
                        } else {
                            itemGrid.keyNavDown();
                        }
                        return;
                    }
                }

                MouseArea {
                    id: mouseArea
                    anchors.fill: parent
                    anchors.bottomMargin: 2; // Prevents autoscrolling down when mouse at bottom of grid

                    property int pressX: -1
                    property int pressY: -1

                    acceptedButtons: Qt.LeftButton | Qt.RightButton

                    enabled: itemGrid.enabled
                    hoverEnabled: enabled

                    function updatePositionProperties(x, y) {
                        var cPos = mapToItem(gridView.contentItem, x, y);
                        var index = gridView.indexAt(cPos.x, cPos.y);
                        gridView.currentIndex = index;
                        itemGrid.focus = true;

                        return index;
                    }

                    onPressed: function (mouse) {
                        mouse.accepted = true;
                        updatePositionProperties(mouse.x, mouse.y);
                        pressX = mouse.x;
                        pressY = mouse.y;

                        if (gridView.currentItem && gridView.currentItem.hasOwnProperty("pressed")) {
                            gridView.currentItem.pressed = true;
                        }

                        if (mouse.button == Qt.RightButton) {
                            if (gridView.currentItem && gridView.currentItem.hasActionList) {
                                openActionMenu(mouse.x, mouse.y, gridView.currentItem.getActionList());
                            }
                        }
                    }

                    onReleased: function (mouse) {
                        mouse.accepted = true;
                        if (gridView.currentItem) {
                            itemGrid.trigger(gridView.currentIndex);
                        } else if (!dragHelper.dragging) {
                            // TODO - pass mouse events down to root instead
                            if (mouse.button == Qt.RightButton) {
                                var cpos = mapToItem(root.mainItem, mouse.x, mouse.y);
                                root.openActionMenu(cpos.x, cpos.y);
                            } else {
                                root.leave();
                            }
                        }

                        if (gridView.currentItem && gridView.currentItem.hasOwnProperty("pressed")) {
                            gridView.currentItem.pressed = false;
                        }

                        pressX = -1;
                        pressY = -1;
                    }

                    onPressAndHold: function (mouse) {
                        if (!dragEnabled) {
                            pressX = -1;
                            pressY = -1;
                            return;
                        }

                        var cPos = mapToItem(gridView.contentItem, mouse.x, mouse.y);
                        var item = gridView.itemAt(cPos.x, cPos.y);

                        if (!item) {
                            return;
                        }

                        if (!dragHelper.isDrag(pressX, pressY, mouse.x, mouse.y)) {
                            kicker.dragSource = item;
                            kicker.draggedAppData = {
                                storageId: item.favoriteId,
                                url: item.url.toString(),
                                isDirectory: item.isDirectory,
                                itemIndex: item.itemIndex,
                                oldFolderId: itemGrid.folderId
                            };

                            if (item.m && "pluginName" in item.m) {
                                dragHelper.startDrag(kicker, item.url, item.icon,
                                "text/x-plasmoidservicename", item.m.pluginName);
                            } else {
                                dragHelper.startDrag(kicker, item.url, item.icon);
                            }
                        }

                        pressX = -1;
                        pressY = -1;
                    }

                    onPositionChanged: function (mouse) {
                        updatePositionProperties(mouse.x, mouse.y);

                        if (gridView.currentIndex != -1 && currentItem && currentItem.m != null) {
                            if (dragEnabled && !dragHelper.dragging && dragHelper.isDrag(pressX, pressY, mouse.x, mouse.y)) {
                                kicker.dragSource = currentItem;
                                kicker.draggedAppData = {
                                    storageId: currentItem.favoriteId,
                                    url: currentItem.url.toString(),
                                    isDirectory: currentItem.isDirectory,
                                    itemIndex: currentItem.itemIndex,
                                    oldFolderId: itemGrid.folderId
                                };
                                if (currentItem.m && "pluginName" in currentItem.m) {
                                    dragHelper.startDrag(kicker, currentItem.url, currentItem.icon,
                                    "text/x-plasmoidservicename", currentItem.m.pluginName);
                                } else {
                                    dragHelper.startDrag(kicker, currentItem.url, currentItem.icon);
                                }

                                pressX = -1;
                                pressY = -1;
                            }
                        }
                    }

                    onContainsMouseChanged: {
                        if (!containsMouse) {
                            if (!actionMenu.opened) {
                                if (gridView.currentItem && gridView.currentItem.hasOwnProperty("pressed")) {
                                    gridView.currentItem.pressed = false;
                                }
                                gridView.currentIndex = -1;
                            }

                            pressX = -1;
                            pressY = -1;
                            //hoverEnabled = false;
                        }
                    }
                }
            }
        }
    }
}
