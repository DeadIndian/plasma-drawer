.pragma library

var builtInPresets = [
  {
    id: "micro-tidy",
    name: "Micro Tidy",
    description: "Minimal compact layout — just a few smartly named folders for the most common tasks. Ideal for small screens or users who prefer a clean, distraction-free launcher.",
    icon: "folder-documents",
    builtIn: true,
    layout: {
      version: 1,
      folders: [
        { id: "folder:1", name: "Browse", icon: "internet-web-browser", apps: ["firefox.desktop","org.kde.neochat.desktop","org.kde.kdeconnect.app.desktop"] },
        { id: "folder:2", name: "Office", icon: "x-office-document", apps: ["libreoffice-writer.desktop","libreoffice-calc.desktop","libreoffice-impress.desktop"] },
        { id: "folder:3", name: "Media", icon: "multimedia-player", apps: ["vlc.desktop","org.kde.elisa.desktop","org.kde.gwenview.desktop","org.kde.spectacle.desktop"] },
        { id: "folder:4", name: "System", icon: "preferences-system", apps: ["org.kde.dolphin.desktop","org.kde.konsole.desktop","org.kde.discover.desktop","systemsettings.desktop"] }
      ],
      rootOrder: ["folder:1","folder:2","folder:3","folder:4","org.kde.kate.desktop","org.kde.kcalc.desktop"],
      hidden: [], renames: {}
    }
  },
  {
    id: "broad-classification",
    name: "Broad Classification",
    description: "Organised into broad KDE-style categories. Closest to the traditional application menu layout.",
    icon: "applications-other",
    builtIn: true,
    layout: {
      version: 1,
      folders: [
        { id: "folder:1", name: "Internet", icon: "internet-web-browser", apps: ["firefox.desktop","org.kde.neochat.desktop","org.kde.kdeconnect.app.desktop","org.kde.krdc.desktop","org.kde.krfb.desktop","org.kde.knetattach.desktop"] },
        { id: "folder:2", name: "Office", icon: "x-office-document", apps: ["libreoffice-writer.desktop","libreoffice-calc.desktop","libreoffice-impress.desktop","libreoffice-draw.desktop","libreoffice-math.desktop","org.kde.kontact.desktop","org.kde.kaddressbook.desktop","org.kde.kmail2.desktop","org.kde.korganizer.desktop"] },
        { id: "folder:3", name: "Graphics", icon: "applications-graphics", apps: ["org.kde.gwenview.desktop","org.kde.kolourpaint.desktop","org.kde.skanpage.desktop","org.kde.spectacle.desktop","org.kde.krita.desktop","gimp.desktop","inkscape.desktop"] },
        { id: "folder:4", name: "Multimedia", icon: "applications-multimedia", apps: ["vlc.desktop","org.kde.elisa.desktop","org.kde.dragonplayer.desktop","org.kde.kdenlive.desktop","org.kde.kamoso.desktop","com.obsproject.Studio.desktop"] },
        { id: "folder:5", name: "Games", icon: "applications-games", apps: ["steam.desktop","org.kde.kmines.desktop","org.kde.kmahjongg.desktop","org.kde.kpat.desktop"] },
        { id: "folder:6", name: "Utilities", icon: "applications-utilities", apps: ["org.kde.kate.desktop","org.kde.kwrite.desktop","org.kde.kcalc.desktop","org.kde.kcharselect.desktop","org.kde.filelight.desktop","org.kde.kleopatra.desktop","org.kde.kwalletmanager.desktop","org.kde.ark.desktop","org.kde.kfind.desktop"] },
        { id: "folder:7", name: "System", icon: "preferences-system", apps: ["org.kde.dolphin.desktop","org.kde.konsole.desktop","org.kde.discover.desktop","systemsettings.desktop","org.kde.partitionmanager.desktop","org.kde.kinfocenter.desktop","org.kde.plasma-systemmonitor.desktop","org.kde.kmenuedit.desktop","org.kde.khelpcenter.desktop"] }
      ],
      rootOrder: ["folder:1","folder:2","folder:3","folder:4","folder:5","folder:6","folder:7"],
      hidden: [], renames: {}
    }
  },
  {
    id: "power-dev",
    name: "Power Dev",
    description: "Built for developers — dev tools, terminals, containers, and docs take centre stage.",
    icon: "applications-development",
    builtIn: true,
    layout: {
      version: 1,
      folders: [
        { id: "folder:1", name: "Dev Tools", icon: "applications-development", apps: ["code.desktop","org.kde.kate.desktop","vim.desktop","org.kde.kdevelop.desktop","org.kde.kdialog.desktop"] },
        { id: "folder:2", name: "Terminal & Shell", icon: "utilities-terminal", apps: ["org.kde.konsole.desktop","org.kde.kfind.desktop","org.kde.kdebugsettings.desktop"] },
        { id: "folder:3", name: "Databases & Containers", icon: "drive-harddisk", apps: ["org.kde.krfb.desktop","org.kde.krdc.desktop","org.kde.kdeconnect.app.desktop"] },
        { id: "folder:4", name: "Docs & Reference", icon: "help-contents", apps: ["org.kde.okular.desktop","org.kde.khelpcenter.desktop","org.kde.kcharselect.desktop"] },
        { id: "folder:5", name: "Graphics & Design", icon: "applications-graphics", apps: ["gimp.desktop","inkscape.desktop","org.kde.krita.desktop","org.kde.gwenview.desktop","org.kde.kolourpaint.desktop","org.kde.spectacle.desktop"] },
        { id: "folder:6", name: "Utilities", icon: "applications-utilities", apps: ["org.kde.kcalc.desktop","org.kde.filelight.desktop","org.kde.ark.desktop","org.kde.kleopatra.desktop","org.kde.kwalletmanager.desktop"] },
        { id: "folder:7", name: "System", icon: "preferences-system", apps: ["org.kde.dolphin.desktop","systemsettings.desktop","org.kde.discover.desktop","org.kde.plasma-systemmonitor.desktop","org.kde.kinfocenter.desktop","org.kde.partitionmanager.desktop","org.kde.kmenuedit.desktop"] }
      ],
      rootOrder: ["folder:1","folder:2","folder:3","folder:4","folder:5","folder:6","folder:7","firefox.desktop","org.kde.neochat.desktop","vlc.desktop"],
      hidden: [],
      renames: { "code.desktop": "VS Code", "org.kde.kate.desktop": "Kate Editor" }
    }
  },
  {
    id: "essential-flat",
    name: "Essential Flat",
    description: "No folders — just a curated flat list of essential apps. Every app is one click away.",
    icon: "view-list-details",
    builtIn: true,
    layout: {
      version: 1,
      folders: [],
      rootOrder: ["firefox.desktop","org.kde.dolphin.desktop","org.kde.konsole.desktop","org.kde.kate.desktop","org.kde.discover.desktop","systemsettings.desktop","libreoffice-writer.desktop","libreoffice-calc.desktop","org.kde.gwenview.desktop","org.kde.spectacle.desktop","vlc.desktop","org.kde.elisa.desktop","org.kde.kcalc.desktop","org.kde.neochat.desktop","org.kde.kdeconnect.app.desktop","org.kde.filelight.desktop"],
      hidden: [], renames: {}
    }
  },
  {
    id: "gaming-rig",
    name: "Gaming Rig",
    description: "Gaming-focused — launchers, genre folders, streaming tools, and comms front and centre.",
    icon: "applications-games",
    builtIn: true,
    layout: {
      version: 1,
      folders: [
        { id: "folder:1", name: "Launchers & Stores", icon: "steam", apps: ["steam.desktop","lutris.desktop","com.heroicgameslauncher.hgl.desktop","net.lutris.Lutris.desktop"] },
        { id: "folder:2", name: "Casual", icon: "applications-games", apps: ["org.kde.kmines.desktop","org.kde.kmahjongg.desktop","org.kde.kpat.desktop","org.kde.katomic.desktop","org.kde.kblocks.desktop","org.kde.kolf.desktop"] },
        { id: "folder:3", name: "Streaming & Comms", icon: "microphone", apps: ["com.obsproject.Studio.desktop","discord.desktop","org.kde.neochat.desktop","org.kde.kdeconnect.app.desktop"] },
        { id: "folder:4", name: "Tools", icon: "preferences-other", apps: ["org.kde.spectacle.desktop","org.kde.klipper.desktop","org.kde.filelight.desktop","org.kde.kcalc.desktop"] }
      ],
      rootOrder: ["folder:1","folder:2","folder:3","folder:4","firefox.desktop","org.kde.dolphin.desktop","org.kde.discover.desktop","systemsettings.desktop"],
      hidden: [], renames: {}
    }
  },
  {
    id: "work-life",
    name: "Work & Life",
    description: "A balanced split between productivity/work apps and personal/life tools.",
    icon: "user-home",
    builtIn: true,
    layout: {
      version: 1,
      folders: [
        { id: "folder:1", name: "Work", icon: "office-workstation", apps: ["libreoffice-writer.desktop","libreoffice-calc.desktop","libreoffice-impress.desktop","org.kde.kontact.desktop","org.kde.kmail2.desktop","org.kde.korganizer.desktop","org.kde.kaddressbook.desktop","org.kde.okular.desktop"] },
        { id: "folder:2", name: "Dev", icon: "applications-development", apps: ["code.desktop","org.kde.kate.desktop","org.kde.konsole.desktop","org.kde.kwrite.desktop","org.kde.khelpcenter.desktop"] },
        { id: "folder:3", name: "Personal", icon: "user-home", apps: ["firefox.desktop","org.kde.neochat.desktop","org.kde.kdeconnect.app.desktop","org.kde.elisa.desktop","org.kde.gwenview.desktop","vlc.desktop"] },
        { id: "folder:4", name: "Utilities", icon: "applications-utilities", apps: ["org.kde.kcalc.desktop","org.kde.filelight.desktop","org.kde.kleopatra.desktop","org.kde.kwalletmanager.desktop","org.kde.ark.desktop","org.kde.spectacle.desktop","org.kde.kcharselect.desktop"] },
        { id: "folder:5", name: "System", icon: "preferences-system", apps: ["org.kde.dolphin.desktop","systemsettings.desktop","org.kde.discover.desktop","org.kde.plasma-systemmonitor.desktop","org.kde.kinfocenter.desktop","org.kde.partitionmanager.desktop"] }
      ],
      rootOrder: ["folder:1","folder:2","folder:3","folder:4","folder:5"],
      hidden: ["org.kde.kmenuedit.desktop","org.kde.kdebugsettings.desktop","org.kde.drkonqi.desktop"],
      renames: { "code.desktop": "VS Code", "firefox.desktop": "Web" }
    }
  },
  {
    id: "kde-classic",
    name: "KDE Classic",
    description: "Mirrors the traditional KDE Application Menu layout with familiar category names.",
    icon: "start-here-kde",
    builtIn: true,
    layout: {
      version: 1,
      folders: [
        { id: "folder:1", name: "Favorites", icon: "emblem-favorite", apps: ["org.kde.dolphin.desktop","org.kde.konsole.desktop","systemsettings.desktop","org.kde.discover.desktop"] },
        { id: "folder:2", name: "Applications", icon: "applications-other", apps: ["firefox.desktop","libreoffice-writer.desktop","org.kde.gwenview.desktop","org.kde.spectacle.desktop","org.kde.kate.desktop","org.kde.kcalc.desktop"] },
        { id: "folder:3", name: "Graphics", icon: "applications-graphics", apps: ["org.kde.gwenview.desktop","org.kde.kolourpaint.desktop","org.kde.skanpage.desktop","org.kde.spectacle.desktop","gimp.desktop","inkscape.desktop","org.kde.krita.desktop"] },
        { id: "folder:4", name: "Internet", icon: "applications-internet", apps: ["firefox.desktop","org.kde.neochat.desktop","org.kde.kdeconnect.app.desktop","org.kde.krdc.desktop","org.kde.krfb.desktop","org.kde.knetattach.desktop"] },
        { id: "folder:5", name: "Multimedia", icon: "applications-multimedia", apps: ["vlc.desktop","org.kde.elisa.desktop","org.kde.dragonplayer.desktop","org.kde.kdenlive.desktop","org.kde.kamoso.desktop"] },
        { id: "folder:6", name: "Office", icon: "applications-office", apps: ["libreoffice-writer.desktop","libreoffice-calc.desktop","libreoffice-impress.desktop","org.kde.kontact.desktop","org.kde.kmail2.desktop","org.kde.korganizer.desktop","org.kde.kaddressbook.desktop","org.kde.okular.desktop"] },
        { id: "folder:7", name: "Games", icon: "applications-games", apps: ["steam.desktop","org.kde.kmines.desktop","org.kde.kmahjongg.desktop","org.kde.kpat.desktop","org.kde.katomic.desktop","org.kde.kblocks.desktop"] },
        { id: "folder:8", name: "Utilities", icon: "applications-utilities", apps: ["org.kde.kcalc.desktop","org.kde.kcharselect.desktop","org.kde.filelight.desktop","org.kde.ark.desktop","org.kde.kfind.desktop","org.kde.kleopatra.desktop","org.kde.kwalletmanager.desktop","org.kde.klipper.desktop"] },
        { id: "folder:9", name: "System", icon: "preferences-system", apps: ["org.kde.dolphin.desktop","org.kde.konsole.desktop","systemsettings.desktop","org.kde.discover.desktop","org.kde.plasma-systemmonitor.desktop","org.kde.kinfocenter.desktop","org.kde.partitionmanager.desktop","org.kde.kmenuedit.desktop","org.kde.khelpcenter.desktop"] }
      ],
      rootOrder: ["folder:1","folder:2","folder:3","folder:4","folder:5","folder:6","folder:7","folder:8","folder:9"],
      hidden: [], renames: {}
    }
  },
  {
    id: "media-studio",
    name: "Media Studio",
    description: "For content creators — video, audio, design, photography, and publishing tools.",
    icon: "applications-multimedia",
    builtIn: true,
    layout: {
      version: 1,
      folders: [
        { id: "folder:1", name: "Video Production", icon: "video-editor", apps: ["org.kde.kdenlive.desktop","com.obsproject.Studio.desktop","vlc.desktop"] },
        { id: "folder:2", name: "Audio", icon: "audio-player", apps: ["org.kde.elisa.desktop","audacity.desktop","org.kde.kamoso.desktop"] },
        { id: "folder:3", name: "Design", icon: "applications-graphics", apps: ["gimp.desktop","inkscape.desktop","org.kde.krita.desktop","org.kde.kolourpaint.desktop"] },
        { id: "folder:4", name: "Photography", icon: "camera-photo", apps: ["org.kde.gwenview.desktop","org.kde.gwenview_importer.desktop","org.kde.spectacle.desktop","org.kde.skanpage.desktop"] },
        { id: "folder:5", name: "Publishing", icon: "x-office-document", apps: ["libreoffice-writer.desktop","libreoffice-impress.desktop","libreoffice-draw.desktop","org.kde.okular.desktop"] },
        { id: "folder:6", name: "Utilities", icon: "applications-utilities", apps: ["org.kde.dolphin.desktop","org.kde.filelight.desktop","org.kde.kcalc.desktop","org.kde.konsole.desktop","org.kde.ark.desktop"] }
      ],
      rootOrder: ["folder:1","folder:2","folder:3","folder:4","folder:5","folder:6","systemsettings.desktop","org.kde.discover.desktop"],
      hidden: [], renames: {}
    }
  },
  {
    id: "minimalist",
    name: "Minimalist",
    description: "Extreme minimalism — only 3 folders and 2 loose apps. Dozens of apps hidden for a clean slate.",
    icon: "edit-clear",
    builtIn: true,
    layout: {
      version: 1,
      folders: [
        { id: "folder:1", name: "Web", icon: "internet-web-browser", apps: ["firefox.desktop","org.kde.neochat.desktop"] },
        { id: "folder:2", name: "Work", icon: "x-office-document", apps: ["libreoffice-writer.desktop","libreoffice-calc.desktop","org.kde.kontact.desktop"] },
        { id: "folder:3", name: "System", icon: "preferences-system", apps: ["org.kde.dolphin.desktop","systemsettings.desktop","org.kde.discover.desktop","org.kde.konsole.desktop"] }
      ],
      rootOrder: ["folder:1","folder:2","folder:3","org.kde.kcalc.desktop","org.kde.spectacle.desktop"],
      hidden: ["org.kde.kmenuedit.desktop","org.kde.kdebugsettings.desktop","org.kde.drkonqi.desktop","org.kde.khelpcenter.desktop","org.kde.klipper.desktop","org.kde.kwalletmanager.desktop","org.kde.kleopatra.desktop","org.kde.kfind.desktop","org.kde.kcharselect.desktop","org.kde.kamoso.desktop","org.kde.krdc.desktop","org.kde.krfb.desktop","org.kde.knetattach.desktop","org.kde.filelight.desktop","org.kde.kinfocenter.desktop","org.kde.kdeconnect.app.desktop","org.kde.partitionmanager.desktop","org.kde.skanpage.desktop","org.kde.kolourpaint.desktop","org.kde.okular.desktop","org.kde.kaddressbook.desktop","org.kde.kmail2.desktop","org.kde.korganizer.desktop","org.kde.kwrite.desktop"],
      renames: {}
    }
  },
  {
    id: "educator",
    name: "Educator",
    description: "Organised for teaching — office suite, reference, communication, creative tools, and educational games.",
    icon: "applications-education",
    builtIn: true,
    layout: {
      version: 1,
      folders: [
        { id: "folder:1", name: "Teaching Tools", icon: "applications-education", apps: ["libreoffice-impress.desktop","libreoffice-writer.desktop","libreoffice-calc.desktop","libreoffice-draw.desktop","libreoffice-math.desktop"] },
        { id: "folder:2", name: "Reference", icon: "help-contents", apps: ["org.kde.okular.desktop","org.kde.khelpcenter.desktop","org.kde.kcharselect.desktop","org.kde.kfind.desktop"] },
        { id: "folder:3", name: "Communication", icon: "internet-mail", apps: ["firefox.desktop","org.kde.kmail2.desktop","org.kde.neochat.desktop","org.kde.kdeconnect.app.desktop","org.kde.kontact.desktop"] },
        { id: "folder:4", name: "Creative", icon: "applications-graphics", apps: ["org.kde.gwenview.desktop","org.kde.kolourpaint.desktop","org.kde.spectacle.desktop","org.kde.krita.desktop","org.kde.kamoso.desktop"] },
        { id: "folder:5", name: "Utilities", icon: "applications-utilities", apps: ["org.kde.kcalc.desktop","org.kde.filelight.desktop","org.kde.kleopatra.desktop","org.kde.kwalletmanager.desktop","org.kde.ark.desktop"] },
        { id: "folder:6", name: "System", icon: "preferences-system", apps: ["org.kde.dolphin.desktop","org.kde.konsole.desktop","systemsettings.desktop","org.kde.discover.desktop","org.kde.plasma-systemmonitor.desktop"] }
      ],
      rootOrder: ["folder:1","folder:2","folder:3","folder:4","folder:5","folder:6","org.kde.kmines.desktop","org.kde.kpat.desktop","org.kde.kmahjongg.desktop"],
      hidden: [], renames: {}
    }
  }
];

function listBuiltIn() {
  return builtInPresets;
}

function listAll(presetsJson) {
  var userPresets = parseUserPresets(presetsJson);
  return builtInPresets.concat(userPresets);
}

function find(id, presetsJson) {
  for (var i = 0; i < builtInPresets.length; ++i) {
    if (builtInPresets[i].id === id) return builtInPresets[i];
  }
  var userPresets = parseUserPresets(presetsJson);
  for (var j = 0; j < userPresets.length; ++j) {
    if (userPresets[j].id === id) return userPresets[j];
  }
  return null;
}

function listUserPresets(presetsJson) {
  return parseUserPresets(presetsJson);
}

function parseUserPresets(jsonStr) {
  if (!jsonStr || jsonStr === "{}" || jsonStr === "[]") return [];
  try {
    var arr = JSON.parse(jsonStr);
    if (Array.isArray(arr)) return arr;
  } catch (e) {}
  return [];
}

function serializeUserPresets(presets) {
  return JSON.stringify(presets);
}

function saveUserPreset(presetsJson, name, description, layoutObj) {
  var presets = parseUserPresets(presetsJson);
  var maxId = 0;
  for (var i = 0; i < presets.length; ++i) {
    var suffix = parseInt(presets[i].id.substring("user:".length), 10);
    if (!isNaN(suffix) && suffix > maxId) maxId = suffix;
  }
  for (var b = 0; b < builtInPresets.length; ++b) {
    var bsuffix = parseInt(builtInPresets[b].id.substring("user:".length), 10);
    if (!isNaN(bsuffix) && bsuffix > maxId) maxId = bsuffix;
  }
  var newId = "user:" + (maxId + 1);
  presets.push({
    id: newId,
    name: name || "Unnamed Preset",
    description: description || "",
    icon: "folder",
    builtIn: false,
    layout: layoutObj
  });
  return { json: serializeUserPresets(presets), preset: presets[presets.length - 1] };
}

function deleteUserPreset(presetsJson, presetId) {
  var presets = parseUserPresets(presetsJson);
  var found = false;
  for (var i = presets.length - 1; i >= 0; --i) {
    if (presets[i].id === presetId) {
      presets.splice(i, 1);
      found = true;
      break;
    }
  }
  return found ? serializeUserPresets(presets) : presetsJson;
}
