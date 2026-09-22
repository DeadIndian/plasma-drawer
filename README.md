<div align="center">

# Deads Plasma Drawer

### A fullscreen, customizable application launcher for KDE Plasma 6

Organize your apps into nestable folders, drag and drop to rearrange, and search apps, files, and system actions with a KRunner-like bar — all in a fullscreen drawer.

[![Build](https://img.shields.io/github/actions/workflow/status/DeadIndian/plasma-drawer/build.yml?style=flat-square)](https://github.com/DeadIndian/plasma-drawer/actions)
[![License](https://img.shields.io/github/license/DeadIndian/plasma-drawer?style=flat-square)](LICENSE)
[![Release](https://img.shields.io/github/v/release/DeadIndian/plasma-drawer?style=flat-square)](https://github.com/DeadIndian/plasma-drawer/releases)
[![Plasma](https://img.shields.io/badge/Plasma-6-1d99f3?style=flat-square&logo=kde)](https://kde.org/plasma-desktop/)
[![Stars](https://img.shields.io/github/stars/DeadIndian/plasma-drawer?style=flat-square)](https://github.com/DeadIndian/plasma-drawer/stargazers)
[![PRs Welcome](https://img.shields.io/badge/PRs-welcome-brightgreen.svg?style=flat-square)](CONTRIBUTING.md)

[Installation](#-installation) ·
[Usage](#-usage) ·
[Report Bug](https://github.com/DeadIndian/plasma-drawer/issues) ·
[Request Feature](https://github.com/DeadIndian/plasma-drawer/issues)

<img src="screenshots/custom.png" alt="Plasma Drawer screenshot" width="80%" />

</div>

---

## 📖 Table of Contents

- [About](#-about)
- [Features](#-features)
- [Screenshots](#-screenshots)
- [Installation](#-installation)
- [Usage](#-usage)
- [Internationalization](#-internationalization)
- [Contributing](#-contributing)
- [License](#-license)
- [Maintainers](#-maintainers)

---

## 🎯 About

Deads Plasma Drawer is a fullscreen application launcher for the KDE Plasma 6 desktop. It replaces the default menu with a spacious grid you fully control: group apps into folders of arbitrary depth, rename or hide entries, and reorder everything by dragging.

It is a **pure QML/JS plasmoid** — no compiled C++ backend. Your entire layout (folders, ordering, renames, hidden apps) lives as a single JSON document in the widget's own config, so nothing touches global system menu files. That means no build step, no root, and no `kbuildsycoca` runs. See [SPEC.md](SPEC.md) for the architecture.

## ✨ Features

- **Fullscreen launcher** — a full-screen grid of your applications
- **Nestable folders** — organize apps into folders of arbitrary depth
- **Drag and drop** — rearrange apps and folders, drop onto a folder to move an app in, drag out to remove it
- **Folder management** — create, rename, and delete folders directly from the interface
- **KRunner-like search** — find apps, files, and more, with configurable and reorderable search plugins
- **System actions** — shutdown, restart, log out and friends, individually hideable and reorderable
- **Self-contained layout** — all customization stored as JSON in the widget config, never in system files
- **Translations** — ships with German, Polish, Romanian, Russian, and Ukrainian

## 📸 Screenshots

| Custom layout | KRunner-like search |
| :---: | :---: |
| <img src="screenshots/custom.png" width="100%" /> | <img src="screenshots/manjaro-search.png" width="100%" /> |

## 🚀 Installation

> Prerequisites: KDE Plasma 6 (`X-Plasma-API-Minimum-Version` is 6.0).

### From a release (recommended)

Download the latest `.plasmoid` from the [releases page](https://github.com/DeadIndian/plasma-drawer/releases/latest), then install it:

```bash
kpackagetool6 -t Plasma/Applet -i plasma-drawer-VERSION.plasmoid
```

Upgrade an existing install:

```bash
kpackagetool6 -t Plasma/Applet -u plasma-drawer-VERSION.plasmoid
```

Uninstall:

```bash
kpackagetool6 -t Plasma/Applet -r io.github.deadindian.plasma-drawer
```

### From source

```bash
git clone https://github.com/DeadIndian/plasma-drawer.git
cd plasma-drawer
make install      # builds the .plasmoid and installs it with kpackagetool6
```

The `Makefile` also provides `make package`, `make upgrade`, `make uninstall`, and `make test` (opens the widget in `plasmoidviewer`).

## 💻 Usage

Add the widget to your panel or desktop, then click its icon to open the drawer. Apps in the grid are arranged left to right, top to bottom.

**Customizing apps and folders**

- Right click the widget icon and select **Edit Applications** to rearrange the grid.
- Drag an application onto a folder to move it in, or drop it on empty space to take it out.
- Drag or copy an app out of any folder to add it to the root page.
- Delete and rename folders directly from the interface.

**Customizing search**

Right click the icon, select **Configure Plasma Drawer**, then open the **Search Plugins** tab. Enable and reorder plugins to taste — matches from plugins higher in the list are prioritized.

**Customizing system actions**

Right click an action and select **Remove action** to hide it, or disable actions entirely in the widget configuration. Long press and drag to reorder them.

## 🌍 Internationalization

Want Plasma Drawer in your language? Follow the [Translations README](translate/README.md) to add a new locale. Existing translations: German, Polish, Romanian, Russian, Ukrainian.

## 🤝 Contributing

Contributions are welcome. Please read [CONTRIBUTING.md](CONTRIBUTING.md) and the [Code of Conduct](CODE_OF_CONDUCT.md) before opening a PR.

1. Fork the repo
2. Create your feature branch (`git checkout -b feature/amazing`)
3. Commit your changes
4. Push to the branch (`git push origin feature/amazing`)
5. Open a Pull Request

## 📄 License

Distributed under the **GNU General Public License v3.0**. See [LICENSE](LICENSE) for details.

## 👥 Maintainers

- **DeadIndian** — current maintainer ([@DeadIndian](https://github.com/DeadIndian))

Originally created by **Connor Popp** ([@p-connor](https://github.com/p-connor)). This is a maintained fork that continues the project.

---

<div align="center">
<sub>Built for KDE Plasma 6</sub>
</div>
