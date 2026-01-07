# Valkyrie

Lightweight Native Desktop Application Framework. Write once, run on Linux and Windows. Minimal binary size using system WebView.

## Overview 

A CLI tool for building native desktop applications with HTML, CSS, and JavaScript. Uses the system WebView for minimal binary size: webkit2gtk on Linux, Edge WebView2 on Windows.

### Features

- Vanilla JavaScript
- React
- Vue  
- Preact
- TypeScript
- CSS/SCSS imports
- Environment variables (.env)
- Hot reload

## Installation

```bash
git clone https://github.com/Kitsuri-Studios/Valkyrie
cd valkyrie
mkdir build && cd build
cmake ..
make -j$(nproc)
sudo make install
```

## Usage

```bash
valkyrie init my-app
cd my-app
npm install
valkyrie dev

valkyrie build
valkyrie build --target=windows 
valkyrie package
```

## Project Structure

```
my-app/
  index.html
  app.js
  package.json
  .env
```

## API

```javascript
valkyrie.dialog.showMessageBox({ title: 'Title', message: 'Content' });
valkyrie.dialog.showOpenDialog({});
valkyrie.dialog.showSaveDialog({});
valkyrie.notification.show('title', 'body');
valkyrie.clipboard.writeText('text');
valkyrie.clipboard.readText();
valkyrie.send({ command: 'custom', data: 'payload' });
```

### Callbacks

```javascript
window.onFileOpen = (path) => {};
window.onFolderOpen = (path) => {};
window.onFileSave = (path) => {};
window.onClipboardRead = (text) => {};
```

## Platform Support

| Feature | Linux | Windows |
|---------|-------|---------|
| Dialogs | Zenity | Win32 |
| Notifications | notify-send | MessageBox |
| Clipboard | GTK | Win32 |
| File Pickers | Zenity | Win32 |
| WebView | webkit2gtk | Edge WebView2 |

macOS support is experimental. Contributions welcome.

## Cross Compilation

```bash
sudo pacman -S mingw-w64-gcc
valkyrie build --target=windows
```

First build downloads and compiles QuickJS and libuv for Windows. Subsequent builds use cached dependencies.

## Binary Size Comparison

| Framework | Size |
|-----------|------|
| Electron | 150-300 MB |
| Tauri | 10-20 MB |
| Valkyrie | ~2 MB |

## Known Issues

- macOS cross-compilation not yet implemented
- Windows notifications use MessageBox instead of toast notifications
- Hot reload may miss changes on slow storage devices

## License

BSD 3-Clause. See [LICENSE](LICENSE) for details.

## Contributing

Pull requests are welcome. New features should support both Linux and Windows.
