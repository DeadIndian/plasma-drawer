/*
 * Plasma Drawer — layout document engine.
 *
 * Pure functions over the "drawer layout" JSON document. No QML, no side effects:
 * every mutator takes a document and returns a NEW document, so the QML layer can
 * decide when to persist and rebuild. This is the single source of truth for the
 * user's custom organization (folders, ordering, renames, hidden apps).
 *
 * The system app list is passed in as `allApps` — an array of
 * { storageId, name, icon } — and is treated as read-only. We never mutate it.
 *
 * Document shape (see SPEC.md):
 *   {
 *     "version": 1,
 *     "folders":  [ { "id": "folder:1", "name": "Games", "icon": "folder-games",
 *                     "apps": ["steam.desktop", ...] } ],
 *     "rootOrder": ["firefox.desktop", "folder:1", ...],
 *     "hidden":   ["some-app.desktop"],
 *     "renames":  { "firefox.desktop": "Web Browser" }
 *   }
 */

.pragma library

const VERSION = 1;
const FOLDER_PREFIX = "folder:";

function isFolderId(id) {
    return typeof id === "string" && id.indexOf(FOLDER_PREFIX) === 0;
}

function defaultDoc() {
    return {
        version: VERSION,
        folders: [],
        rootOrder: [],
        hidden: [],
        renames: {}
    };
}

// True when the document carries no user organization at all — i.e. it is the
// default doc (fresh install / after Reset). Used to gate the one-time auto
// organize: we only sweep apps into folders when the user hasn't arranged
// anything yet, so we never disturb a layout they've touched.
function isEmpty(doc) {
    if (!doc) return true;
    return (!doc.folders || doc.folders.length === 0)
        && (!doc.rootOrder || doc.rootOrder.length === 0)
        && (!doc.hidden || doc.hidden.length === 0)
        && (!doc.renames || Object.keys(doc.renames).length === 0);
}

// Parse a stored string into a well-formed document, tolerating empties/garbage.
function parse(str) {
    if (!str || str === "{}") {
        return defaultDoc();
    }
    var doc;
    try {
        doc = JSON.parse(str);
    } catch (e) {
        return defaultDoc();
    }
    if (!doc || typeof doc !== "object") {
        return defaultDoc();
    }
    // Normalize missing fields so callers never have to null-check.
    var base = defaultDoc();
    if (Array.isArray(doc.folders)) base.folders = doc.folders;
    if (Array.isArray(doc.rootOrder)) base.rootOrder = doc.rootOrder;
    if (Array.isArray(doc.hidden)) base.hidden = doc.hidden;
    if (doc.renames && typeof doc.renames === "object") base.renames = doc.renames;
    return base;
}

function serialize(doc) {
    return JSON.stringify(doc);
}

// --- import/export ----------------------------------------------------------

// Pretty-printed form used when saving the layout to a file the user can share.
function serializeForExport(doc) {
    return JSON.stringify(doc, null, 2) + "\n";
}

// Strictly validate an imported layout JSON string. Unlike parse(), which is
// deliberately forgiving (a corrupt stored layout must never break the
// launcher), an import should fail loudly with a human-readable reason so the
// user gets feedback instead of a silently wiped layout. Returns either
// { ok: true, doc } with a normalized document ready to persist, or
// { ok: false, error: <message> }.
function validateImport(str) {
    if (typeof str !== "string" || str.trim() === "") {
        return { ok: false, error: "The document is empty." };
    }
    var doc;
    try {
        doc = JSON.parse(str);
    } catch (e) {
        return { ok: false, error: "Not valid JSON: " + e.message };
    }
    if (!doc || typeof doc !== "object" || Array.isArray(doc)) {
        return { ok: false, error: "Expected a JSON object at the top level." };
    }
    if (doc.version !== undefined
        && (typeof doc.version !== "number" || doc.version < 1 || doc.version > VERSION)) {
        return { ok: false, error: "Unsupported layout version (expected " + VERSION + ")." };
    }
    if (doc.folders !== undefined) {
        if (!Array.isArray(doc.folders)) {
            return { ok: false, error: "\"folders\" must be an array." };
        }
        for (var i = 0; i < doc.folders.length; ++i) {
            var f = doc.folders[i];
            if (!f || typeof f !== "object" || Array.isArray(f)) {
                return { ok: false, error: "Folder #" + (i + 1) + " is not an object." };
            }
            if (!isFolderId(f.id)) {
                return { ok: false, error: "Folder #" + (i + 1) + " has an invalid id (expected \"folder:N\")." };
            }
            if (f.name !== undefined && typeof f.name !== "string") {
                return { ok: false, error: "Folder " + f.id + " has a non-string name." };
            }
            if (f.icon !== undefined && typeof f.icon !== "string") {
                return { ok: false, error: "Folder " + f.id + " has a non-string icon." };
            }
            if (f.apps !== undefined && !_isStringArray(f.apps)) {
                return { ok: false, error: "Folder " + f.id + " has an invalid \"apps\" list." };
            }
        }
    }
    if (doc.rootOrder !== undefined && !_isStringArray(doc.rootOrder)) {
        return { ok: false, error: "\"rootOrder\" must be an array of strings." };
    }
    if (doc.hidden !== undefined && !_isStringArray(doc.hidden)) {
        return { ok: false, error: "\"hidden\" must be an array of strings." };
    }
    if (doc.renames !== undefined) {
        if (!doc.renames || typeof doc.renames !== "object" || Array.isArray(doc.renames)) {
            return { ok: false, error: "\"renames\" must be an object." };
        }
        for (var key in doc.renames) {
            if (typeof doc.renames[key] !== "string") {
                return { ok: false, error: "\"renames\" must map app ids to strings." };
            }
        }
    }
    // Structurally sound — hand back a normalized document with defaults filled.
    return { ok: true, doc: parse(str) };
}

function _isStringArray(value) {
    if (!Array.isArray(value)) return false;
    for (var i = 0; i < value.length; ++i) {
        if (typeof value[i] !== "string") return false;
    }
    return true;
}

// --- internal helpers -----------------------------------------------------

function _clone(doc) {
    return JSON.parse(JSON.stringify(doc));
}

function _findFolder(doc, folderId) {
    for (var i = 0; i < doc.folders.length; ++i) {
        if (doc.folders[i].id === folderId) return doc.folders[i];
    }
    return null;
}

function _removeFromArray(arr, value) {
    var idx = arr.indexOf(value);
    if (idx !== -1) arr.splice(idx, 1);
    return idx !== -1;
}

// Remove an app storageId from wherever it currently lives (root or any folder).
function _detachApp(doc, storageId) {
    _removeFromArray(doc.rootOrder, storageId);
    for (var i = 0; i < doc.folders.length; ++i) {
        _removeFromArray(doc.folders[i].apps, storageId);
    }
}

// Build a lookup from storageId -> app record for quick resolution.
function _appMap(allApps) {
    var map = {};
    for (var i = 0; i < allApps.length; ++i) {
        map[allApps[i].storageId] = allApps[i];
    }
    return map;
}

// Every storageId the document explicitly places (root + folders).
function _placedApps(doc) {
    var placed = {};
    for (var i = 0; i < doc.rootOrder.length; ++i) {
        if (!isFolderId(doc.rootOrder[i])) placed[doc.rootOrder[i]] = true;
    }
    for (var f = 0; f < doc.folders.length; ++f) {
        var apps = doc.folders[f].apps;
        for (var a = 0; a < apps.length; ++a) placed[apps[a]] = true;
    }
    return placed;
}

function displayName(doc, storageId, fallback) {
    if (doc.renames && doc.renames[storageId]) return doc.renames[storageId];
    return fallback;
}

// --- resolution (document + system list -> view model) --------------------

// Resolve the root grid into an ordered array of entries:
//   { type: "app",    storageId, name, icon }
//   { type: "folder", id, name, icon, count }
// Apps installed on the system but not placed anywhere are auto-appended, so newly
// installed apps show up without any document edit. Hidden apps are skipped.
function resolveRoot(doc, allApps) {
    var map = _appMap(allApps);
    var hidden = {};
    for (var h = 0; h < doc.hidden.length; ++h) hidden[doc.hidden[h]] = true;

    var out = [];

    // 1. Entries in explicit root order.
    for (var i = 0; i < doc.rootOrder.length; ++i) {
        var entry = doc.rootOrder[i];
        if (isFolderId(entry)) {
            var folder = _findFolder(doc, entry);
            if (folder) {
                out.push({
                    type: "folder",
                    id: folder.id,
                    name: folder.name || "",
                    icon: folder.icon || "folder",
                    count: _visibleFolderCount(folder, map, hidden)
                });
            }
        } else if (!hidden[entry] && map[entry]) {
            out.push(_appEntry(doc, map[entry]));
        }
        // Silently drop stale entries (uninstalled apps / missing folders).
    }

    // 2. Auto-append installed apps that aren't placed and aren't hidden,
    //    preserving the system list's (alphabetical) order.
    var placed = _placedApps(doc);
    for (var j = 0; j < allApps.length; ++j) {
        var app = allApps[j];
        if (!placed[app.storageId] && !hidden[app.storageId]) {
            out.push(_appEntry(doc, app));
        }
    }

    return out;
}

function _appEntry(doc, app) {
    return {
        type: "app",
        storageId: app.storageId,
        name: displayName(doc, app.storageId, app.name),
        icon: app.icon,
        url: app.url || ""   // real .desktop file path, for external drag + dummy check
    };
}

function _visibleFolderCount(folder, map, hidden) {
    var n = 0;
    for (var i = 0; i < folder.apps.length; ++i) {
        var sid = folder.apps[i];
        if (map[sid] && !hidden[sid]) n++;
    }
    return n;
}

// Resolve one folder into ordered app entries (skips hidden / uninstalled).
function resolveFolder(doc, folderId, allApps) {
    var folder = _findFolder(doc, folderId);
    if (!folder) return [];
    var map = _appMap(allApps);
    var hidden = {};
    for (var h = 0; h < doc.hidden.length; ++h) hidden[doc.hidden[h]] = true;

    var out = [];
    for (var i = 0; i < folder.apps.length; ++i) {
        var sid = folder.apps[i];
        if (map[sid] && !hidden[sid]) out.push(_appEntry(doc, map[sid]));
    }
    return out;
}

// --- mutators (return a NEW document) -------------------------------------

function moveAppToFolder(doc, storageId, folderId) {
    var d = _clone(doc);
    var folder = _findFolder(d, folderId);
    if (!folder) return d;
    _detachApp(d, storageId);
    if (folder.apps.indexOf(storageId) === -1) folder.apps.push(storageId);
    return d;
}

// Pull an app out of any folder and place it at root (appended to the end).
function removeAppFromFolder(doc, storageId) {
    var d = _clone(doc);
    var wasInFolder = false;
    for (var i = 0; i < d.folders.length; ++i) {
        if (_removeFromArray(d.folders[i].apps, storageId)) wasInFolder = true;
    }
    if (wasInFolder && d.rootOrder.indexOf(storageId) === -1) {
        d.rootOrder.push(storageId);
    }
    return d;
}

// Pull an app out of any folder and place it at a SPECIFIC visible position in
// the root grid. `index` is a resolved-root index (as the user sees it in the
// grid), so — like reorderRoot — we materialize the full resolved order into
// explicit ids first, then splice the app in at that spot. This is what lets a
// drag out of a folder land exactly where the pointer is dropped, instead of
// always being appended to the end.
function removeAppFromFolderAt(doc, allApps, storageId, index) {
    var d = _clone(doc);
    var wasInFolder = false;
    for (var i = 0; i < d.folders.length; ++i) {
        if (_removeFromArray(d.folders[i].apps, storageId)) wasInFolder = true;
    }
    if (!wasInFolder) return d;

    // With the app now detached from its folder it is "unplaced", so resolveRoot
    // auto-appends it at the end. Materialize that order, pull the app back out,
    // then re-insert it at the requested position. Removing it first guarantees
    // `ids` matches the current visible grid exactly (which does not yet contain
    // the app), so `index` — computed from that grid — lines up.
    var resolved = resolveRoot(d, allApps);
    var ids = resolved.map(function (e) {
        return e.type === "folder" ? e.id : e.storageId;
    });
    _removeFromArray(ids, storageId);
    if (typeof index !== "number" || index < 0 || index > ids.length) {
        index = ids.length;
    }
    ids.splice(index, 0, storageId);
    d.rootOrder = ids;
    return d;
}

// Create a folder at root. `seedApp` (optional) is dropped into it immediately —
// this is the drag-app-onto-app "make a folder" gesture. Returns { doc, folderId }.
function createFolder(doc, name, seedApps) {
    var d = _clone(doc);
    // Allocate a stable unique id. Scan existing ids for the max numeric suffix.
    var maxId = 0;
    for (var i = 0; i < d.folders.length; ++i) {
        var suffix = parseInt(d.folders[i].id.substring(FOLDER_PREFIX.length), 10);
        if (!isNaN(suffix) && suffix > maxId) maxId = suffix;
    }
    var newId = FOLDER_PREFIX + (maxId + 1);
    var apps = [];
    if (seedApps) {
        for (var s = 0; s < seedApps.length; ++s) {
            _detachApp(d, seedApps[s]);
            apps.push(seedApps[s]);
        }
    }
    d.folders.push({ id: newId, name: name || "New Folder", icon: "folder", apps: apps });
    d.rootOrder.push(newId);
    return { doc: d, folderId: newId };
}

// Minimum number of (unplaced) apps a category must contribute before it earns
// its own folder. Below this the apps stay loose at root — a folder holding one
// or two icons is more clutter than help.
const MIN_FOLDER_SIZE = 3;

// Sweep currently-unplaced apps into category folders, Android-launcher style.
// `categorizedApps` is [{ category: <translated name>, apps: [storageId, ...] }],
// as produced from kicker's categorized model. Only apps that aren't already
// placed (root or any folder) and aren't hidden are moved; existing folders,
// renames, hidden entries and ordering are left untouched. An app can appear in
// several categories (KDE menus overlap) — first category wins. Categories that
// contribute fewer than MIN_FOLDER_SIZE unplaced apps are skipped, leaving those
// apps at root. If a folder with the category's name already exists we append
// into it rather than making a duplicate. Returns a NEW document.
function autoOrganize(doc, categorizedApps) {
    var d = _clone(doc);
    if (!categorizedApps || categorizedApps.length === 0) return d;

    // Apps that already have an explicit home — never touch these.
    var placed = _placedApps(d);
    var hidden = {};
    for (var h = 0; h < d.hidden.length; ++h) hidden[d.hidden[h]] = true;

    // Track apps consumed in this pass so overlapping categories don't fight.
    var consumed = {};

    for (var c = 0; c < categorizedApps.length; ++c) {
        var cat = categorizedApps[c];
        if (!cat || !cat.category || !cat.apps) continue;

        // Collect the unplaced, not-yet-consumed, non-hidden apps for this category.
        var pending = [];
        for (var a = 0; a < cat.apps.length; ++a) {
            var sid = cat.apps[a];
            if (placed[sid] || consumed[sid] || hidden[sid]) continue;
            pending.push(sid);
        }
        if (pending.length < MIN_FOLDER_SIZE) continue;

        // Reuse an existing folder of the same name, else create one.
        var folder = _findFolderByName(d, cat.category);
        var folderId;
        if (folder) {
            folderId = folder.id;
        } else {
            var res = createFolder(d, cat.category, null);
            d = res.doc;
            folderId = res.folderId;
            folder = _findFolder(d, folderId);
        }

        for (var p = 0; p < pending.length; ++p) {
            var appId = pending[p];
            if (folder.apps.indexOf(appId) === -1) folder.apps.push(appId);
            consumed[appId] = true;
        }
    }

    return d;
}

function _findFolderByName(doc, name) {
    for (var i = 0; i < doc.folders.length; ++i) {
        if (doc.folders[i].name === name) return doc.folders[i];
    }
    return null;
}

function renameFolder(doc, folderId, newName) {
    var d = _clone(doc);
    var folder = _findFolder(d, folderId);
    if (folder) folder.name = newName;
    return d;
}

// Delete a folder; its apps fall back to root (they stay installed).
function deleteFolder(doc, folderId) {
    var d = _clone(doc);
    for (var i = 0; i < d.folders.length; ++i) {
        if (d.folders[i].id === folderId) {
            var orphans = d.folders[i].apps;
            for (var a = 0; a < orphans.length; ++a) {
                if (d.rootOrder.indexOf(orphans[a]) === -1) d.rootOrder.push(orphans[a]);
            }
            d.folders.splice(i, 1);
            break;
        }
    }
    _removeFromArray(d.rootOrder, folderId);
    return d;
}

// Display-only rename. Empty/blank name clears the override.
function renameApp(doc, storageId, newName) {
    var d = _clone(doc);
    if (!d.renames) d.renames = {};
    if (newName && newName.trim() !== "") {
        d.renames[storageId] = newName.trim();
    } else {
        delete d.renames[storageId];
    }
    return d;
}

// "Delete" an app == hide it from the drawer. Real .desktop is untouched.
function hideApp(doc, storageId) {
    var d = _clone(doc);
    _detachApp(d, storageId);
    if (d.hidden.indexOf(storageId) === -1) d.hidden.push(storageId);
    return d;
}

// Reorder within the root grid: move the entry at `fromIndex` (as seen in the
// resolved root) to `toIndex`. Because resolveRoot may auto-append unplaced apps,
// we first materialize the full resolved order into rootOrder, then splice.
function reorderRoot(doc, allApps, fromIndex, toIndex) {
    var d = _clone(doc);
    var resolved = resolveRoot(doc, allApps);
    if (fromIndex < 0 || fromIndex >= resolved.length ||
        toIndex < 0 || toIndex >= resolved.length || fromIndex === toIndex) {
        return d;
    }
    // Materialize current visible order as a list of ids (storageId or folderId).
    var ids = resolved.map(function (e) {
        return e.type === "folder" ? e.id : e.storageId;
    });
    var moved = ids.splice(fromIndex, 1)[0];
    ids.splice(toIndex, 0, moved);
    d.rootOrder = ids;
    return d;
}

// Reorder within a folder.
function reorderFolder(doc, folderId, fromIndex, toIndex) {
    var d = _clone(doc);
    var folder = _findFolder(d, folderId);
    if (!folder) return d;
    if (fromIndex < 0 || fromIndex >= folder.apps.length ||
        toIndex < 0 || toIndex >= folder.apps.length || fromIndex === toIndex) {
        return d;
    }
    var moved = folder.apps.splice(fromIndex, 1)[0];
    folder.apps.splice(toIndex, 0, moved);
    return d;
}
