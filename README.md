# File Tinder

A swipe-style file organization tool with an intuitive "Tinder-like" interface for quickly sorting and organizing files.

![File Tinder](https://img.shields.io/badge/Qt-6.x-green) ![C++17](https://img.shields.io/badge/C%2B%2B-17-blue) ![License](https://img.shields.io/badge/License-MIT-yellow)

## Features

### 🎯 Basic Mode
- **Swipe-style sorting**: Use arrow keys or buttons to quickly categorize files
  - → **Keep**: Keep file in original location
  - ← **Delete**: Mark file for deletion
  - ↓ **Skip**: Skip/ignore file
  - ↑ **Back**: Go back to previous file
- **Move to Folder**: Select destination folders for files
- **Progress tracking**: Visual progress bar and statistics
- **Session persistence**: Resume sorting sessions across application restarts

### 🌳 Advanced Mode
- **Visual Mind Map View**: Large, clickable folder nodes displayed as a mind map
- **Interactive folder tree**: Click any folder node to instantly move the current file there
- **Dynamic folder creation**: Create new folders on-the-fly during sorting
- **Quick access bar**: Pin frequently used folders for one-click access
- **Folder connections**: Group related folders visually
- **Pan and zoom**: Navigate large folder structures with ease

### 📋 Review & Execute
- **Batch operations**: Review all decisions before execution
- **Summary view**: See file counts and sizes per destination
- **Safe deletion**: Files are moved to trash by default
- **Error handling**: Detailed error reporting for any issues

## Screenshots

### Main Window
```
┌─────────────────────────────────────────────────────────────┐
│                      📁 FILE TINDER                         │
│                                                             │
│              Select a folder to organize:                   │
│    ┌─────────────────────────────────┐ ┌─────────────┐     │
│    │ /home/user/Downloads           │ │  Browse...  │     │
│    └─────────────────────────────────┘ └─────────────┘     │
│                                                             │
│    ┌─────────────────┐  ┌─────────────────┐                │
│    │  🎯 Basic Mode  │  │ 🌳 Advanced Mode │                │
│    │ Simple swipe    │  │ Visual folder   │                │
│    │   sorting       │  │     tree        │                │
│    └─────────────────┘  └─────────────────┘                │
└─────────────────────────────────────────────────────────────┘
```

## Requirements

- **CMake** 3.16 or higher
- **Qt 6.x** with the following modules:
  - Qt6::Core
  - Qt6::Widgets
  - Qt6::Gui
  - Qt6::Sql
- **C++17** compatible compiler

## Building

### Quick Build

```bash
# Clone the repository
git clone https://github.com/trabalhefabricio/file-tinder.git
cd file-tinder

# Create build directory
mkdir build && cd build

# Configure and build
cmake ..
cmake --build . --parallel
```

### Detailed Build Instructions

#### Linux (Ubuntu/Debian)

```bash
# Install dependencies
sudo apt-get update
sudo apt-get install -y cmake qt6-base-dev qt6-tools-dev

# Build
mkdir build && cd build
cmake ..
make -j$(nproc)

# Run
./FileTinder
```

#### Linux (Fedora)

```bash
# Install dependencies
sudo dnf install cmake qt6-qtbase-devel qt6-qttools-devel

# Build
mkdir build && cd build
cmake ..
make -j$(nproc)
```

#### macOS

```bash
# Install dependencies via Homebrew
brew install cmake qt@6

# Add Qt to PATH
export PATH="/opt/homebrew/opt/qt@6/bin:$PATH"

# Build
mkdir build && cd build
cmake ..
make -j$(sysctl -n hw.ncpu)

# Run
./FileTinder
```

#### Windows (Visual Studio)

```batch
REM Install Qt 6 from https://www.qt.io/download
REM Set Qt6_DIR environment variable

mkdir build
cd build
cmake -G "Visual Studio 17 2022" -A x64 ..
cmake --build . --config Release
```

### CMake Options

| Option | Default | Description |
|--------|---------|-------------|
| `CMAKE_BUILD_TYPE` | Release | Build type (Debug/Release) |
| `Qt6_DIR` | Auto-detected | Path to Qt6 CMake config |

## Usage

### Keyboard Shortcuts

#### Basic Mode
| Key | Action |
|-----|--------|
| → (Right Arrow) | Keep file |
| ← (Left Arrow) | Delete file |
| ↓ (Down Arrow) | Skip file |
| ↑ (Up Arrow) | Go back |
| M | Move to folder |
| Enter | Finish review |

#### Advanced Mode
| Key | Action |
|-----|--------|
| 1-9 | Quick access to folders 1-9 |
| D | Delete file |
| S | Skip file |
| N | Add new folder |
| F | Toggle folder panel |
| Ctrl+Scroll | Zoom in/out |
| Middle-click drag | Pan view |

### Workflow

1. **Select Folder**: Choose the folder containing files to organize
2. **Choose Mode**: Basic for simple sorting, Advanced for complex folder structures
3. **Sort Files**: Use swipes/clicks to assign each file to a destination
4. **Review**: Check the summary of all pending operations
5. **Execute**: Confirm and execute all moves/deletions

## Project Structure

```
file-tinder/
├── CMakeLists.txt              # Build configuration
├── README.md                   # This file
├── app/
│   ├── include/                # Header files
│   │   ├── ui_constants.hpp
│   │   ├── DatabaseManager.hpp
│   │   ├── FolderTreeModel.hpp
│   │   ├── FolderNodeWidget.hpp
│   │   ├── MindMapView.hpp
│   │   ├── FileTinderExecutor.hpp
│   │   ├── StandaloneFileTinderDialog.hpp
│   │   └── AdvancedFileTinderDialog.hpp
│   ├── lib/                    # Source files
│   │   ├── main.cpp
│   │   ├── DatabaseManager.cpp
│   │   ├── FolderTreeModel.cpp
│   │   ├── FolderNodeWidget.cpp
│   │   ├── MindMapView.cpp
│   │   ├── FileTinderExecutor.cpp
│   │   ├── StandaloneFileTinderDialog.cpp
│   │   └── AdvancedFileTinderDialog.cpp
│   └── resources/              # Resources
│       ├── resources.qrc
│       └── icons/
│           ├── folder.svg
│           ├── folder-new.svg
│           └── folder-linked.svg
└── proposal                    # Original specification
```

## Database

File Tinder uses SQLite for session persistence. The database is stored in:
- **Linux**: `~/.local/share/FileTinder/file_tinder.db`
- **macOS**: `~/Library/Application Support/FileTinder/file_tinder.db`
- **Windows**: `%APPDATA%/FileTinder/file_tinder.db`

### Schema

```sql
-- File decisions
CREATE TABLE file_tinder_state (
    folder_path TEXT NOT NULL,
    file_path TEXT NOT NULL,
    decision TEXT NOT NULL,
    destination_folder TEXT,
    timestamp DATETIME
);

-- Folder tree configuration
CREATE TABLE tinder_folder_tree (
    session_folder TEXT NOT NULL,
    folder_path TEXT NOT NULL,
    display_name TEXT,
    is_virtual INTEGER,
    is_pinned INTEGER
);

-- Recent folders
CREATE TABLE recent_folders (
    folder_path TEXT NOT NULL,
    timestamp DATETIME
);
```

## Contributing

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Acknowledgments

- Inspired by the Tinder swipe interface
- Built with [Qt 6](https://www.qt.io/)
- Icons from [Material Design Icons](https://materialdesignicons.com/)
