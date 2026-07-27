# Plasma Drawer — JSON + QML Rewrite Spec

## Goal

Replace the fragile C++ backend (`menueditorbackend.cpp`, `allappsmodel.cpp`) that
edits real system menu files (`applications-kmenuedit.menu`, `.directory`/`.desktop`
files) and shells out to `kbuildsycoca6`, with a **pure QML/JS** launcher whose
custom organization (folders, ordering, renames, hidden apps) lives in a single
JSON blob in the plasmoid's own config.

Result: **no compiled plugin**. The widget becomes a plain Plasma package that
installs directly from the KDE Store via "Get New Widgets" — no distro packaging,
no build step, no root, no `kbuildsycoca`.

## Core principle

Two clearly separated sources of truth:

1. **Installed apps — read-only, owned by the system.** Enumerated and launched
   through `org.kde.plasma.private.kicker`'s `AppsModel { flat: true }`, which this
   project *already imports* for search, system actions, drag, and process running.
   We use it only as (a) the master flat app list and (b) the launch engine
   (`trigger()` runs a `KIO::ApplicationLauncherJob` in C++, so proper startup
   notification/activation is preserved). We never use its category tree.

2. **User organization — read-write, owned by us.** A JSON document in
   `plasmoid.configuration.drawerLayout`. All folder/order/rename/hide operations
   are plain mutations of this document. Persisted through KConfig like the existing
   `favoriteSystemActions` / `hiddenApplications` lists.

The old approach mutated **global system state** for what is really **per-widget
presentation**. That inversion was the source of the fragility. This spec fixes it.

## Layout JSON schema

Stored as a JSON string in `plasmoid.configuration.drawerLayout`.

```json
{
  "version": 1,
  "folders": [
    { "id": "folder:1", "name": "Games", "icon": "folder-games", "apps": ["steam.desktop", "lutris.desktop"] }
  ],
  "rootOrder": ["org.mozilla.firefox.desktop", "folder:1", "org.kde.dolphin.desktop"],
  "hidden": ["some-app.desktop"],
  "renames": { "org.mozilla.firefox.desktop": "Web Browser" }
}
```

- `rootOrder` — ordered entries shown at the root grid. Each entry is either an app
  `storageId` (ends in `.desktop`) or a folder id (prefixed `folder:`).
- `folders[].apps` — ordered `storageId`s inside that folder.
- Apps present on the system but absent from `rootOrder` and every folder are
  **auto-appended** to root (newly installed apps appear automatically).
- `hidden` — `storageId`s omitted from the drawer. The app stays installed.
- `renames` — `storageId` → custom display label (drawer-only override).
- Folders are **one level deep** (apps live in folders; folders live at root). This
  matches the actual Android-launcher-style UX; nested folders were never surfaced.

## Behavior decisions (confirmed)

- **Delete app** → adds `storageId` to `hidden`. Reversible via Reset. Real app
  untouched. (No package uninstall.)
- **Duplicate app** → removed. Meaningless in a display-only model.
- **Rename app** → display-only override in `renames`. Real `.desktop` untouched.
- **Rename/Delete/Create folder** → pure JSON edits (folders are our construct).
- **Reset to default** → clears `drawerLayout` back to `{}`; everything reverts to
  the flat system list. Re-arms the first-run auto-organize (below).
- **First-run auto-organize** → on a genuinely empty layout (`isEmpty(doc)`, i.e.
  fresh install or post-Reset), loose apps are swept into category folders once,
  Android-launcher style. Categories come from a transient categorized
  `Kicker.RootModel` (created, read once, destroyed). Only unplaced, non-hidden
  apps are moved; existing folders/renames/hidden stay intact. Categories with
  fewer than `MIN_FOLDER_SIZE` (3) eligible apps are left loose. Overlapping
  categories: first wins. Not automatic once any layout exists.

## Component design

### `contents/code/layout.js` (new, `.pragma library`)
Pure functions over the layout document — no QML, unit-testable:
- `parse(str)` / `serialize(doc)` / `defaultDoc()`
- `resolveRoot(doc, allApps)` → ordered array of resolved entries
  (`{type:'app', storageId, name, icon}` or `{type:'folder', id, name, icon, apps:[...]}`),
  auto-appending unplaced apps, skipping hidden ones.
- `resolveFolder(doc, folderId, allApps)` → ordered resolved app entries.
- Mutators returning a **new doc**: `moveAppToFolder`, `removeAppFromFolder`,
  `createFolder`, `renameFolder`, `deleteFolder`, `renameApp`, `hideApp`,
  `reorderRoot`, `reorderFolder`, `autoOrganize`.
- `isEmpty(doc)` → true for the default doc (gates first-run auto-organize).
- `autoOrganize(doc, categorizedApps)` → sweeps unplaced apps into category
  folders; `categorizedApps` is `[{category, apps:[storageId,...]}]`.
- `serializeForExport(doc)` → pretty-printed JSON for saving the layout to a
  shareable file.
- `validateImport(str)` → strict structural check of an imported layout string;
  returns `{ok, doc}` (normalized) or `{ok:false, error}` so a bad file fails
  loudly instead of silently wiping the layout. The General config page exposes
  this as "Folder structure": export to a JSON file (via the executable data
  engine) and import from an http(s) URL (via XMLHttpRequest).

### `contents/ui/DrawerModel.qml` (new)
Replaces both `AllAppsModel` and `MenuEditorBackend`. A QtObject that owns:
- `Kicker.AppsModel { flat: true; id: appSource }` — master list + launcher.
- A `storageId → flatRow` map, rebuilt on `appSource.countChanged`.
- `property string layoutJson: plasmoid.configuration.drawerLayout` binding.
- A root `ListModel` (`rootModel`) and lazily-created per-folder `ListModel`s.

Public interface consumed by the views (mirrors current contract so call sites
change minimally):
- `count`, `trigger(index, actionId, arg)`, `modelForRow(index)`, `moveRow(from, to)`,
  `favoritesModel` (null for apps), signal `modelReset()`.
- Item roles: `display`, `decoration`, `url` (folderId for folders, "" for apps),
  `favoriteId` (storageId), `hasChildren`, `hasActionList`, `actionList`, `name`.
- Mutators (same names as old backend): `moveAppToFolder`, `removeAppFromFolder`,
  `renameFolder`, `deleteFolder`, `renameApp`, `createFolder`, `resetToDefault`,
  `deleteApp` (→ hide), `getFolderIndex(folderId)`.
- Each mutator: edit JS doc → write `plasmoid.configuration.drawerLayout` →
  rebuild affected `ListModel`(s) → emit `modelReset()`.

Launch: `trigger(index)` maps the entry's `storageId` to its flat-source row and
calls `appSource.trigger(row, actionId, arg)`; folders return false (the view enters
them via `modelForRow`). App jump-list actions are copied from the flat source.

### Views — rewiring
- `main.qml`: drop `PlasmaDrawer.AllAppsModel`, `PlasmaDrawer.MenuEditorBackend`,
  and the `PlasmaDrawer` import. Add one `DrawerModel { id: drawerModel }`.
  `appsModel` becomes `drawerModel`. `showAllApps` toggle becomes "show flat list"
  (bypass layout) vs "show organized" — implemented as a property on `DrawerModel`.
- `MenuRepresentation.qml`: `menuEditorBackend` references → `drawerModel`.
- `ItemGridView.qml` / `AppsGridView.qml`: `menuEditorBackend.*` → `drawerModel.*`;
  `getFolderIndex(model, id)` → `drawerModel.getFolderIndex(id)`.
- `ItemGridDelegate.qml`: remove `hasMoreThanOneInstance` / duplicate / delete-enabled
  logic; "Delete" always enabled (hides). Keep rename-folder / delete-folder actions.
- `tools.js`: drop `_duplicate_app`; `_delete_app` → `drawerModel.deleteApp`;
  `_create_folder` → `drawerModel.createFolder`. `menuEditorBackend` arg → `drawerModel`.

### Removed
- `plugin/` entire directory (`menueditorbackend.*`, `allappsmodel.*`, `plugin.cpp`,
  `qmldir`, `CMakeLists.txt`, tests, `update_menu.py`).
- Top-level `CMakeLists.txt`, `Makefile` — no longer a compiled project. (Keep a thin
  `Makefile` with `kpackagetool6 --install/--upgrade` targets for dev convenience.)
- `Qt::Xml`, `KF6::Service`, `KF6::Config` C++ deps.

## Config change
Add to `contents/config/main.xml`:
```xml
<entry name="drawerLayout" type="String"><default>{}</default></entry>
```

## Verification
- `qmllint-qt6` on every changed `.qml`.
- `kpackagetool6 --upgrade .` to install into the live session.
- `plasmoidviewer -a .` for interactive smoke test.
- Manual: launch app, create folder, drag app in/out, reorder, rename folder,
  rename app, delete (hide) app, reset, toggle flat view, search still works,
  new app auto-appears, layout survives Plasma restart.

## Out of scope / unchanged
- Search path (`RunnerModel` / KRunner) stays as-is — it's independent C++ from
  Plasma, not our plugin, and keeps its own `trigger()`.
- System actions bar (`SystemModel` + `KAStatsFavoritesModel`) unchanged.
- Config writes flush to disk on `plasmashell` exit (KConfig lifecycle), not
  instantly mid-session — acceptable, matches how favorites already persist.
