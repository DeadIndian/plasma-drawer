#!/usr/bin/env node
/*
 * Self-check for the nested-folder layout logic (run: node tests/layout.nesting.test.js).
 * layout.js is a QML `.pragma library` module; node can't import it directly, so we
 * read it, drop the pragma line, and eval it into a sandbox exposing its functions.
 */
"use strict";
const fs = require("fs");
const path = require("path");

const src = fs.readFileSync(path.join(__dirname, "../contents/code/layout.js"), "utf8")
    .replace(/^\s*\.pragma\s+library\s*$/m, "");
// Expose the top-level `function foo(){}` declarations after eval.
const L = {};
new Function(src + "\n;Object.assign(this, {createFolder, resolveRoot, resolveFolder, moveAppToFolder, deleteFolder, isFolderId, defaultDoc});").call(L);

let failures = 0;
function check(name, cond) {
    if (!cond) { failures++; console.error("FAIL:", name); }
    else console.log("ok  :", name);
}

const apps = [
    { storageId: "a.desktop", name: "A", icon: "a" },
    { storageId: "b.desktop", name: "B", icon: "b" },
    { storageId: "c.desktop", name: "C", icon: "c" },
];

// 1. Create a subfolder inside a folder.
let d = L.defaultDoc();
let outer = L.createFolder(d, "Outer", ["a.desktop", "b.desktop"]);
d = outer.doc;
let inner = L.createFolder(d, "Inner", ["b.desktop"], outer.folderId);
d = inner.doc;

check("subfolder id not at root", d.rootOrder.indexOf(inner.folderId) === -1);
check("subfolder id inside parent apps", L.resolveFolder(d, outer.folderId, apps)
    .some(e => e.type === "folder" && e.id === inner.folderId));
check("moved app left outer, now only in inner",
    L.resolveFolder(d, outer.folderId, apps).filter(e => e.type === "app").length === 1
    && L.resolveFolder(d, inner.folderId, apps).some(e => e.storageId === "b.desktop"));

// 2. Cycle guard: cannot move a folder into itself or a descendant.
let cyc = L.moveAppToFolder(d, outer.folderId, inner.folderId); // outer -> its own child
check("cycle move rejected (doc unchanged)",
    L.resolveFolder(cyc, inner.folderId, apps).every(e => e.id !== outer.folderId));
let self = L.moveAppToFolder(d, outer.folderId, outer.folderId);
check("self move rejected",
    L.resolveFolder(self, outer.folderId, apps).every(e => e.id !== outer.folderId));

// 3. Delete a subfolder: its contents re-parent to the PARENT folder, not root.
let delInner = L.deleteFolder(d, inner.folderId);
check("deleted subfolder gone from parent",
    L.resolveFolder(delInner, outer.folderId, apps).every(e => e.id !== inner.folderId));
check("orphaned app re-parented to outer, not root",
    L.resolveFolder(delInner, outer.folderId, apps).some(e => e.storageId === "b.desktop")
    && L.resolveRoot(delInner, apps).every(e => e.type !== "app" || e.storageId !== "b.desktop"));

// 4. Delete a root-level folder: contents fall to root.
let delOuter = L.deleteFolder(d, outer.folderId);
check("deleted root folder's apps land at root",
    L.resolveRoot(delOuter, apps).some(e => e.storageId === "a.desktop"));

process.exit(failures ? 1 : 0);
