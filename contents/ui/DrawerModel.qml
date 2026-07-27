/*
 * Plasma Drawer — model controller.
 *
 * Replaces the old C++ AllAppsModel + MenuEditorBackend. This is a pure-QML/JS
 * object that:
 *   1. reads the flat installed-app list from kicker's AppsModel (read-only source
 *      of truth for "what apps exist", and the launch engine via trigger()),
 *   2. owns the user's custom organization as a JSON document persisted in
 *      plasmoid.configuration.drawerLayout (see layout.js + SPEC.md),
 *   3. exposes a duck-typed model interface matching what the grid views expect
 *      from a kicker model: a ListModel carrying trigger()/modelForRow()/moveRow()
 *      and a drawerRebuilt() signal (modelReset is reserved by QAbstractItemModel).
 *
 * The object assigned to a view's `model` is a ListModel. `rootModel` is the root
 * grid; folders are lazily-built child ListModels returned by modelForRow().
 */

import QtQuick
import org.kde.plasma.private.kicker as Kicker

import "../code/layout.js" as Layout

QtObject {
    id: drawerModel

    // The applet root (kicker) — needed by RootModel for launching context.
    property var appletInterface: null
    // RootModel only populates once it has a valid appletInterface. The parent
    // assigns it after this object's children complete, so refresh here when it
    // actually arrives rather than in the source's Component.onCompleted.
    onAppletInterfaceChanged: {
        if (appletInterface) {
            rootSource.refresh();
            _rebuildIndex();
        }
    }
    // Display-name format, forwarded to the kicker source.
    property int appNameFormat: 0
    // When true, bypass the layout entirely and show the raw flat system list
    // (the "show all apps" toggle). No folders, no renames, no hiding.
    property bool flatMode: false

    // Kicker role ints. The Kicker::* enum is not exposed to QML, so we use the
    // literal values from kicker's actionlist.h (verified against the installed
    // plugin, and that data()/index() are JS-callable). Qt.UserRole (256) + offset.
    readonly property int _favoriteIdRole: 259  // Kicker::FavoriteIdRole
    readonly property int _urlRole: 266         // Kicker::UrlRole

    // Bound to the persisted layout JSON by the caller (main.qml).
    property string layoutJson: "{}"

    // --- the interface the views consume -----------------------------------
    // rootModel IS the model handed to the root AppsGridView. It carries the
    // controller functions/properties directly (see probes: this is valid QML).
    readonly property alias rootModel: rootModelImpl

    // Kicker RootModel: the master list + launcher. A bare AppsModel does NOT
    // yield a flat app list — with showTopLevelItems:false its root is only the
    // KServiceGroup categories (folders), and `flat` merely propagates down into
    // those child group models. The actual flat/dedup/sort of every installed app
    // is done in C++ by RootModel when showAllApps is set: it recurses every
    // category, collects the ApplicationType entries into one AppsModel tagged
    // "KICKER_ALL_MODEL", and nests it as a child (rootmodel.cpp). So we drive a
    // RootModel and treat that one child as our app source (see _resolveAppSource).
    property Kicker.RootModel rootSource: Kicker.RootModel {
        autoPopulate: true
        flat: true
        showAllApps: true
        // We only want the flat app list — suppress every other root section so
        // the all-apps child is the only meaningful entry we have to find.
        showAllAppsCategorized: false
        showRecentApps: false
        showRecentDocs: false
        showRecentFolders: false
        showPowerSession: false
        showFavoritesPlaceholder: false
        showTopLevelItems: false
        paginate: false
        appletInterface: drawerModel.appletInterface
        appNameFormat: drawerModel.appNameFormat

        onCountChanged: drawerModel._rebuildIndex()
        // appletInterface is assigned by the parent AFTER this child completes, so
        // refresh() in onCompleted runs too early (interface still null → no data).
        // Refresh whenever the interface actually arrives.
        onAppletInterfaceChanged: {
            if (appletInterface) {
                refresh();
                drawerModel._rebuildIndex();
            }
        }
        Component.onCompleted: {
            if (appletInterface) {
                refresh();
                drawerModel._rebuildIndex();
            }
        }
    }

    // --- first-run auto organize -------------------------------------------
    // On a fresh install (empty layout) we sweep the loose apps into category
    // folders, Android-launcher style, so the user isn't faced with hundreds of
    // ungrouped icons. Categories come from kicker itself: a SECOND, transient
    // RootModel in categorized mode, whose root rows are the (translated) menu
    // categories, each carrying its apps as a child model. It's created on demand,
    // read once, then destroyed — a persistent second RootModel would double the
    // (heavy) app-scan cost for a one-time operation.

    // Set once per session the moment we kick off (or decline) the sweep, so the
    // repeated _rebuildIndex() calls from rootSource.onCountChanged don't retrigger
    // it. Persistence across sessions is handled by isEmpty(): once folders exist
    // the layout is no longer empty, so the sweep never runs again (until Reset).
    property bool _autoOrganizeAttempted: false
    property var _categorizedSource: null

    property Component _categorizedSourceComponent: Component {
        Kicker.RootModel {
            autoPopulate: true
            flat: true
            showAllApps: true
            showAllAppsCategorized: true   // root rows become the menu categories
            showRecentApps: false
            showRecentDocs: false
            showRecentFolders: false
            showPowerSession: false
            showFavoritesPlaceholder: false
            showTopLevelItems: false
            paginate: false
            appletInterface: drawerModel.appletInterface
            appNameFormat: drawerModel.appNameFormat
            // Population is async and arrives category-by-category (and each
            // category's apps fill in after that), so debounce rather than read
            // a half-populated tree on the first count change.
            onCountChanged: drawerModel._categorizedSettleTimer.restart()
        }
    }

    property Timer _categorizedSettleTimer: Timer {
        interval: 250
        onTriggered: drawerModel._runAutoOrganize()
    }

    // Kick off the sweep if this is a genuine first run. Called from
    // _rebuildIndex once the flat app list is known.
    function _maybeAutoOrganize() {
        if (_autoOrganizeAttempted) return;
        if (flatMode) return;
        if (_allApps.length === 0) return;      // app list not populated yet
        if (!Layout.isEmpty(_doc())) return;    // user already has a layout
        _autoOrganizeAttempted = true;
        _categorizedSource = _categorizedSourceComponent.createObject(drawerModel);
    }

    // Read the transient categorized source into the [{category, apps}] shape
    // layout.js expects. The per-category child model carries the translated
    // category caption in its `description` (same role the flat source uses for
    // its KICKER_ALL_MODEL tag), and its rows are real apps with favoriteId(259).
    function _readCategorizedApps() {
        var src = _categorizedSource;
        if (!src) return [];
        var out = [];
        for (var i = 0; i < src.count; ++i) {
            var child = src.modelForRow(i);
            if (!child) continue;
            // The category caption is the parent row's display text (the section
            // header kicker renders); fall back to the child model's description.
            var name = "" + (src.data(src.index(i, 0), Qt.DisplayRole) || "");
            if (!name && child.description) name = "" + child.description;
            if (!name || name === "KICKER_ALL_MODEL") continue;
            var apps = [];
            for (var j = 0; j < child.count; ++j) {
                var sid = child.data(child.index(j, 0), _favoriteIdRole);
                if (sid) apps.push("" + sid);
            }
            if (apps.length > 0) out.push({ category: name, apps: apps });
        }
        return out;
    }

    function _runAutoOrganize() {
        if (_categorizedSource) {
            var cats = _readCategorizedApps();
            if (cats.length > 0) {
                _commit(Layout.autoOrganize(_doc(), cats));
            }
            _categorizedSource.destroy();
            _categorizedSource = null;
        }
    }

    // The flat all-apps AppsModel nested inside rootSource. RootModel tags it with
    // the non-i18n description "KICKER_ALL_MODEL" specifically so callers can match
    // it; its rows are real ApplicationType entries, so favoriteId(259)/url(266)
    // resolve. Returns null until RootModel has populated.
    function _resolveAppSource() {
        for (var i = 0; i < rootSource.count; ++i) {
            var m = rootSource.modelForRow(i);
            if (m && m.description === "KICKER_ALL_MODEL") return m;
        }
        return null;
    }

    // The resolved flat all-apps model (rootSource's KICKER_ALL_MODEL child).
    // Cached here so _launch can trigger() on the same model the rows come from.
    property var appSource: null
    // storageId -> flat source row, rebuilt whenever the source list changes.
    property var _rowFor: ({})
    // Cached [{ storageId, name, icon }] snapshot of the flat source.
    property var _allApps: []
    // Live child folder models, keyed by folderId, so entering a folder twice
    // returns a stable object and reorders persist.
    property var _folderModels: ({})

    // Guards the layoutJson->rebuild reaction. Our own writes set this true so a
    // single explicit rebuild() (or none, for live reorder) runs instead of the
    // reaction firing a second, redundant rebuild. External changes (reset, config
    // restore) leave it false so they still rebuild.
    property bool _suppressRebuild: false

    function _rebuildIndex() {
        var src = _resolveAppSource();
        appSource = src;
        var map = {};
        var apps = [];
        if (!src) {
            _rowFor = map;
            _allApps = apps;
            rebuild();
            return;
        }
        var n = src.count;
        for (var i = 0; i < n; ++i) {
            var idx = src.index(i, 0);
            var url = src.data(idx, _urlRole);
            var urlStr = url ? ("" + url) : "";
            var sid = src.data(idx, _favoriteIdRole);
            if (!sid) {
                // Fall back to the url as identity if favoriteId is empty.
                sid = urlStr;
            }
            if (!sid) continue;
            map[sid] = i;
            apps.push({
                storageId: sid,
                name: src.data(idx, Qt.DisplayRole),
                icon: src.data(idx, Qt.DecorationRole),
                url: urlStr
            });
        }
        _rowFor = map;
        _allApps = apps;
        rebuild();
        // Now that the flat app list is known, do a one-time first-run sweep into
        // category folders if the user hasn't organized anything yet.
        _maybeAutoOrganize();
    }

    // --- persistence --------------------------------------------------------

    function _doc() {
        return Layout.parse(layoutJson);
    }

    // Persist a mutated document and rebuild the visible models. Writing the
    // config property is what makes it survive across sessions (KConfig).
    function _commit(doc) {
        // Writing layoutJson fires onLayoutJsonChanged; suppress its rebuild and do
        // exactly one here so a commit never rebuilds twice.
        _suppressRebuild = true;
        layoutJson = Layout.serialize(doc);
        _suppressRebuild = false;
        rebuild();
    }

    // Persist a reordered document WITHOUT rebuilding the models. Live drag-reorder
    // already moved the ListModel rows itself (ListModel.move, which animates the
    // delegates and keeps their model.index in sync); re-filling here would set()
    // data under the delegates and desync the drag source (the "going ballistic"
    // bug). Reordering touches neither the app index nor folder membership, so no
    // rebuild is needed — we only need the new order to persist.
    function _commitOrderOnly(doc) {
        _suppressRebuild = true;
        layoutJson = Layout.serialize(doc);
        _suppressRebuild = false;
    }

    // Whenever the persisted JSON changes (e.g. reset, or external edit), rebuild —
    // unless one of our own mutators is driving the write and handles it explicitly.
    onLayoutJsonChanged: {
        if (_suppressRebuild) return;
        rebuild();
    }

    // --- (re)building the ListModels ---------------------------------------

    function rebuild() {
        _rebuildRoot();
        _rebuildOpenFolders();
        rootModelImpl.drawerRebuilt();
    }

    function _rebuildRoot() {
        var entries = flatMode
            ? _allApps.map(function (a) { return { type: "app", storageId: a.storageId, name: a.name, icon: a.icon }; })
            : Layout.resolveRoot(_doc(), _allApps);
        _fill(rootModelImpl, entries);
    }

    function _rebuildOpenFolders() {
        var doc = _doc();
        for (var fid in _folderModels) {
            var fm = _folderModels[fid];
            if (!fm) continue;
            _fill(fm, Layout.resolveFolder(doc, fid, _allApps));
        }
    }

    // Fill a ListModel from resolved entries. Roles the views/delegate read:
    //   display     - visible label
    //   decoration  - icon name
    //   url         - app: real .desktop file path (external drag + dummy check);
    //                 folder: "" (folders have no file)
    //   favoriteId  - app: storageId (stable identity used for all layout ops);
    //                 folder: ""
    //   folderId    - folder: its id (identity used for all layout ops); app: ""
    //   hasChildren - true for folders (view enters them via modelForRow)
    //   hasActionList, name
    function _fill(model, entries) {
        // Sync in place rather than clear()+append(). clear() empties the model,
        // which collapses the GridView's contentHeight and snaps contentY back to
        // 0 (the "jumps to top on reload" bug), and also re-triggers the per-item
        // add animation on every rebuild. Updating rows in place keeps the scroll
        // position and only animates rows that actually changed.
        for (var i = 0; i < entries.length; ++i) {
            var e = entries[i];
            var row = (e.type === "folder")
                ? {
                    display: e.name,
                    decoration: e.icon,
                    url: "",
                    favoriteId: "",
                    folderId: e.id,
                    hasChildren: true,
                    hasActionList: true,
                    name: e.name
                }
                : {
                    display: e.name,
                    decoration: e.icon,
                    url: e.url,
                    favoriteId: e.storageId,
                    folderId: "",
                    hasChildren: false,
                    hasActionList: true,
                    name: e.name
                };
            if (i < model.count) {
                model.set(i, row);
            } else {
                model.append(row);
            }
        }
        // Drop any rows beyond the new length.
        if (model.count > entries.length) {
            model.remove(entries.length, model.count - entries.length);
        }
    }

    // --- launch + folder navigation ----------------------------------------

    // Launch the app with the given storageId through the kicker source, which
    // runs a KIO::ApplicationLauncherJob (proper startup notification/activation).
    // Returns true if the drawer should close.
    function _launch(storageId, actionId, argument) {
        if (!appSource) return false;
        var row = _rowFor[storageId];
        if (row === undefined) return false;
        return appSource.trigger(row, actionId || "", argument === undefined ? null : argument);
    }

    // Get (creating if needed) the child ListModel for a folder id.
    function _folderModel(folderId) {
        if (_folderModels[folderId]) return _folderModels[folderId];
        var fm = folderModelComponent.createObject(drawerModel, { folderId: folderId });
        _folderModels[folderId] = fm;
        _fill(fm, Layout.resolveFolder(_doc(), folderId, _allApps));
        return fm;
    }

    // Resolve a folder id to its current row in the root model, or -1.
    function getFolderIndex(folderId) {
        for (var i = 0; i < rootModelImpl.count; ++i) {
            if (rootModelImpl.get(i).folderId === folderId) return i;
        }
        return -1;
    }

    // --- mutators (public API mirrors the old MenuEditorBackend) ------------

    function moveAppToFolder(storageId, folderId /*, oldFolderId */) {
        if (Layout.isFolderId(storageId)) return; // never nest folders
        _commit(Layout.moveAppToFolder(_doc(), storageId, folderId));
    }

    function removeAppFromFolder(storageId /*, oldFolderId */) {
        _commit(Layout.removeAppFromFolder(_doc(), storageId));
    }

    // Pull an app out of its folder and drop it at a SPECIFIC visible position in
    // the root grid (index as seen in the grid). Lets a drag out of a folder land
    // exactly where the pointer is, instead of always appending to the end.
    function removeAppFromFolderAt(storageId, index) {
        _commit(Layout.removeAppFromFolderAt(_doc(), _allApps, storageId, index));
    }

    function createFolder(folderName /*, parentFolderId */, seedApps) {
        var res = Layout.createFolder(_doc(), folderName, seedApps);
        _commit(res.doc);
        return res.folderId;
    }

    function renameFolder(folderId, newName) {
        _commit(Layout.renameFolder(_doc(), folderId, newName));
    }

    function deleteFolder(folderId) {
        if (_folderModels[folderId]) {
            _folderModels[folderId].destroy();
            delete _folderModels[folderId];
        }
        _commit(Layout.deleteFolder(_doc(), folderId));
    }

    function renameApp(storageId, newName) {
        _commit(Layout.renameApp(_doc(), storageId, newName));
    }

    // "Delete" == hide from the drawer; the real app is untouched.
    function deleteApp(storageId /*, folderId */) {
        _commit(Layout.hideApp(_doc(), storageId));
    }

    function resetToDefault() {
        // Drop every folder submodel; the document goes back to empty.
        for (var fid in _folderModels) {
            if (_folderModels[fid]) _folderModels[fid].destroy();
        }
        _folderModels = ({});
        // Re-arm the first-run sweep: the now-empty layout should re-organize.
        _autoOrganizeAttempted = false;
        layoutJson = "{}";   // triggers onLayoutJsonChanged -> rebuild()
        _maybeAutoOrganize();
    }

    // Public: (re)run the category sweep on demand. Only sweeps apps that are
    // currently loose at root; existing folders/renames/hidden stay intact.
    // Wire a header button to this if you want a manual "organize" action.
    function autoOrganize() {
        if (_categorizedSource) return;   // a sweep is already in flight
        _categorizedSource = _categorizedSourceComponent.createObject(drawerModel);
    }

    // --- the root model object ---------------------------------------------

    property ListModel rootModelImpl: ListModel {
        id: rootModelImpl

        // Duck-typed to match kicker models the views expect.
        property var favoritesModel: null   // apps aren't favoritable here
        property string folderId: ""         // root has no folder id
        signal drawerRebuilt()

        function trigger(index, actionId, argument) {
            if (index < 0 || index >= count) return false;
            var item = get(index);
            if (item.hasChildren) return false;   // folders are entered, not launched
            return drawerModel._launch(item.favoriteId, actionId, argument);
        }

        // Return the child model for a folder row, or null for an app row.
        function modelForRow(index) {
            if (index < 0 || index >= count) return null;
            var item = get(index);
            if (!item.hasChildren) return null;
            return drawerModel._folderModel(item.folderId);
        }

        // Drag-reorder within the root grid. Move the live ListModel row first so
        // the delegate (and thus kicker.dragSource.itemIndex) animates and stays in
        // sync with the pointer, THEN persist the new order without a rebuild. Doing
        // a rebuild/set() here instead would leave the drag source pinned to its old
        // slot displaying shifted data, retriggering moves every frame ("ballistic").
        function moveRow(from, to) {
            if (from === to) return;
            if (from < 0 || from >= count || to < 0 || to >= count) return;
            move(from, to, 1);
            drawerModel._commitOrderOnly(Layout.reorderRoot(drawerModel._doc(), drawerModel._allApps, from, to));
        }
    }

    // Template for lazily-created per-folder models.
    property Component folderModelComponent: Component {
        ListModel {
            property var favoritesModel: null
            property string folderId: ""
            property bool hasChildren: true   // it IS a folder (used by tryEnterDirectory)
            signal drawerRebuilt()

            function trigger(index, actionId, argument) {
                if (index < 0 || index >= count) return false;
                var item = get(index);
                if (item.hasChildren) return false;
                return drawerModel._launch(item.favoriteId, actionId, argument);
            }

            function modelForRow(index) {
                return null; // folders are one level deep
            }

            // Same live-move-then-persist approach as the root grid (see rootModelImpl.moveRow).
            function moveRow(from, to) {
                if (from === to) return;
                if (from < 0 || from >= count || to < 0 || to >= count) return;
                move(from, to, 1);
                drawerModel._commitOrderOnly(Layout.reorderFolder(drawerModel._doc(), folderId, from, to));
            }
        }
    }
}
