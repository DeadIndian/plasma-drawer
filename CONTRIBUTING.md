# Contributing to Plasma Drawer

First off, thanks for taking the time to contribute! 🎉

The following is a set of guidelines for contributing. These are mostly guidelines, not rules — use your best judgment, and feel free to propose changes to this document in a pull request.

## Code of Conduct

This project and everyone participating in it is governed by our [Code of Conduct](CODE_OF_CONDUCT.md). By participating, you are expected to uphold it. Please report unacceptable behavior to gollabharath2007@gmail.com.

## How Can I Contribute?

### Reporting Bugs

Open an [issue](https://github.com/DeadIndian/plasma-drawer/issues) with:
- A clear, descriptive title
- Steps to reproduce
- What you expected to happen vs. what actually happened
- Your environment (distro, Plasma version, Qt version)

### Suggesting Enhancements

Open an issue describing the enhancement, why it's useful, and any alternatives you considered.

### Translations

To add or update a translation for your language, follow the [Translations README](translate/README.md).

### Pull Requests

1. Fork the repo and create your branch from `main`.
2. Make your changes.
3. Test the widget locally: `make test` (opens it in `plasmoidviewer`).
4. If you touched layout/nesting logic, run the JS tests: `node --test tests/`.
5. Make sure your code follows the existing QML/JS style in `contents/`.
6. Write a clear commit message and open the PR.

## Development Setup

```bash
git clone https://github.com/DeadIndian/plasma-drawer.git
cd plasma-drawer

# Install into your user plasmoid dir
make install

# Or just preview without installing
make test
```

This is a pure QML/JS plasmoid — there is no compile step. Edit files under `contents/`, then run `make test` or reload the widget to see changes.

## Architecture

The layout model (folders, ordering, renames, hidden apps) is stored as a single JSON document in the widget config. Installed apps are read-only, enumerated through `org.kde.plasma.private.kicker`. See [SPEC.md](SPEC.md) for the full design.

## Questions?

Feel free to open an issue or reach out to a maintainer.
