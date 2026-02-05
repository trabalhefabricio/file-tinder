# File Tinder Standalone - Feature Specification

## Table of Contents
1. [Executive Summary](#executive-summary)
2. [Core Concept](#core-concept)
3. [Basic Mode: Standalone File Tinder](#basic-mode-standalone-file-tinder)
4. [Advanced Mode: Folder Tree System](#advanced-mode-folder-tree-system)
5. [User Interface Design](#user-interface-design)
6. [Technical Implementation](#technical-implementation)
7. [Database Schema](#database-schema)
8. [Implementation Roadmap](#implementation-roadmap)

---

## Executive Summary

This document specifies a **standalone version of File Tinder** - a swipe-style file organization tool that combines the intuitive "Tinder-like" interface with an advanced folder destination system. The tool allows users to quickly sort files by swiping through them while assigning each file to a destination folder, with support for both simple sorting and complex folder hierarchies.

### Key Features
- **Standalone application** or integrated module
- **Basic Mode**: Simple swipe-to-sort with Keep/Delete/Skip actions
- **Advanced Mode**: Clickable folder tree with visual node connections
- **Folder creation**: Create new folders on-the-fly during sorting
- **Session persistence**: Resume sorting sessions across application restarts
- **Batch operations**: Review and confirm all decisions before execution

---

## Core Concept

### The "Tinder" Metaphor
Just like the dating app Tinder, users swipe through items (files) one at a time:
- **Swipe Right (→)** = Positive action (Keep/Move to folder)
- **Swipe Left (←)** = Negative action (Delete)
- **Swipe Down (↓)** = Neutral action (Skip/Ignore)
- **Swipe Up (↑)** = Go back/Undo

### Extended Metaphor for File Organization
In the standalone version, we extend this:
- **Right swipe** → Opens folder selection (Basic) or shows folder tree (Advanced)
- **Multiple folders** → Files can be routed to different destinations
- **Folder branching** → Hierarchical folder structures with visual connections

---

## Basic Mode: Standalone File Tinder

### Overview
Basic Mode provides a simple, streamlined interface for quick file cleanup and organization.

### User Flow

```
┌─────────────────────────────────────────────────────────────┐
│                    FILE TINDER - BASIC MODE                 │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│   ┌─────────────────────────────────────────────────────┐   │
│   │                                                     │   │
│   │              [FILE PREVIEW AREA]                    │   │
│   │                                                     │   │
│   │         (Image preview / Text content /             │   │
│   │          File metadata display)                     │   │
│   │                                                     │   │
│   └─────────────────────────────────────────────────────┘   │
│                                                             │
│   File: vacation_photo_2024.jpg                             │
│   Size: 3.2 MB | Type: JPEG | Modified: Jan 15, 2024        │
│                                                             │
│   ══════════════════════════════════════════════════════    │
│   Progress: [████████████░░░░░░░░] 45 / 120 files (38%)     │
│   ══════════════════════════════════════════════════════    │
│                                                             │
│   ✓ Keep: 20  |  ✗ Delete: 15  |  ↓ Skip: 10                │
│                                                             │
│   ┌────────┐    ┌────────┐    ┌────────┐    ┌────────┐      │
│   │ ↑ Back │    │← Delete│    │↓ Skip  │    │→ Keep  │      │
│   └────────┘    └────────┘    └────────┘    └────────┘      │
│                                                             │
│                    [📁 Move to Folder...]                    │
│                    [Finish Review →]                         │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

### Actions in Basic Mode

| Action | Keyboard | Button | Description |
|--------|----------|--------|-------------|
| Keep | → Right Arrow | → Keep | Keep file in original location |
| Delete | ← Left Arrow | ← Delete | Mark file for deletion |
| Skip | ↓ Down Arrow | ↓ Skip | Ignore file, move to next |
| Undo | ↑ Up Arrow | ↑ Back | Go back to previous file |
| Move | M | 📁 Move to Folder | Select destination folder |
| Finish | Enter | Finish Review | Show summary and execute |

### Move to Folder Feature
When user clicks "Move to Folder" or presses 'M':
1. Quick folder picker dialog opens
2. Shows recently used folders at top
3. Browse button for folder selection
4. "Create New Folder" option
5. File is marked with destination folder
6. Continue to next file

---

## Advanced Mode: Folder Tree System

### Overview
Advanced Mode introduces a **visual node-based folder tree** that allows users to:
- See all target folders as clickable nodes
- Create visual connections between folders
- Organize folders hierarchically
- Create new folders during the sorting process
- Assign files to any folder in the tree with one click

### The Node/Tree Concept

Advanced Mode combines the File Tinder swipe interface with a **visual folder tree** displayed alongside the file preview. This is similar to the Whitelist Tree Editor system but adapted for folder destinations.

```
┌──────────────────────────────────────────────────────────────────────────────┐
│                    FILE TINDER - ADVANCED MODE                               │
├──────────────────────────────────────────────────────────────────────────────┤
│                                                                              │
│  ┌────────────────────────────────┐  ┌─────────────────────────────────────┐ │
│  │    FILE PREVIEW               │  │     DESTINATION FOLDER TREE        │ │
│  │    ════════════════           │  │     ═════════════════════════      │ │
│  │                               │  │                                     │ │
│  │  ┌─────────────────────────┐  │  │  📁 ~/Sorted                        │ │
│  │  │                         │  │  │  ├── 📁 Images ◀━━━━━━━━━━━━━━┓    │ │
│  │  │  [vacation_photo.jpg]   │  │  │  │   ├── 📁 Vacation          ┃    │ │
│  │  │                         │  │  │  │   ├── 📁 Family            ┃    │ │
│  │  │      🏖️ Beach Scene      │  │  │  │   └── 📁 Screenshots       ┃    │ │
│  │  │                         │  │  │  ├── 📁 Documents               ┃    │ │
│  │  └─────────────────────────┘  │  │  │   ├── 📁 Work              ━━━┛   │ │
│  │                               │  │  │   └── 📁 Personal                 │ │
│  │  Size: 3.2 MB | JPEG          │  │  ├── 📁 Videos                       │ │
│  │  Modified: Jan 15, 2024       │  │  └── 📁 Other                        │ │
│  │                               │  │                                     │ │
│  │  ────────────────────         │  │  [+ Add Folder] [+ Add Subfolder]   │ │
│  │  45 / 120 files               │  │  [ ] Show connected folders only     │ │
│  │  ════════════════════         │  │                                     │ │
│  │                               │  │  ─────────────────────────────────  │ │
│  │  ┌──────┐ ┌──────┐ ┌──────┐  │  │  Quick Actions:                      │ │
│  │  │Delete│ │ Skip │ │ Back │  │  │  [Images/Vacation] ⭐                 │ │
│  │  └──────┘ └──────┘ └──────┘  │  │  [Documents/Work]                     │ │
│  │                               │  │  [Recently Used...]                  │ │
│  └────────────────────────────────┘  └─────────────────────────────────────┘ │
│                                                                              │
│  [Cancel Session]                                        [Execute All Moves] │
│                                                                              │
└──────────────────────────────────────────────────────────────────────────────┘
```

### Node Types

#### 1. Existing Folders (Solid Nodes)
- Displayed with folder icon 📁
- Can be clicked to assign current file
- Shows file count badge when files are assigned
- Double-click to expand/collapse

#### 2. Virtual/Planned Folders (Dashed Nodes)
- Displayed with dashed border or different icon 📂
- Folders that will be created by the software
- Created when user clicks "+ Add Folder"
- Become solid after execution

#### 3. Connected Folders (Linked Nodes)
- Visual lines/connections between related folders
- Useful for showing folder relationships
- Can be connected/disconnected by user
- Connected folders appear as quick-access buttons

### Advanced Mode Features

#### A. Clickable Folder Tree
```cpp
// Conceptual structure
struct FolderNode {
    QString path;                    // Full path to folder
    QString display_name;            // Short name for display
    bool exists;                     // true = existing folder, false = will be created
    bool is_connected;               // Part of a connection group
    std::vector<FolderNode*> children;
    FolderNode* parent;
    int assigned_file_count;         // Files assigned to this folder
};
```

**Interactions:**
- **Single click**: Assign current file to this folder and advance to next file
- **Double click**: Expand/collapse folder children
- **Right click**: Context menu (Rename, Delete, Disconnect, Set as default)
- **Drag & drop**: Reorder folders (optional)

#### B. Folder Connections (Branching System)
Connections allow users to group related folders:

```
Example: User is sorting photos

📁 Images
├── 📁 2024 ──────┐
│   ├── January   │
│   └── February  │ Connected Group
├── 📁 2023 ──────┘ (Show together)
│   └── ...
└── 📁 Favorites ◀── Standalone
```

**Connection Use Cases:**
- Group destination folders that should appear together
- Create "quick access" groups for frequently used folders
- Visual organization of the folder tree
- Filter view to show only connected folders

#### C. Dynamic Folder Creation

**During Session:**
```
User clicks [+ Add Folder]
    → Dialog: "New Folder Name: [          ]"
    → Select parent folder from tree
    → Option: "Create immediately" or "Create when executing"
    → New node appears in tree (dashed if not yet created)
```

**Folder Creation Rules:**
1. Created folders are marked as "virtual" until execution
2. Virtual folders can have files assigned to them
3. On execution, virtual folders are created first, then files are moved
4. If folder creation fails, affected file moves are skipped (with error report)

#### D. File Assignment Flow

```
┌─────────────────────────────────────────────────────────────────┐
│                                                                 │
│   1. User sees file preview                                     │
│              │                                                  │
│              ▼                                                  │
│   2. User clicks folder in tree OR uses keyboard shortcut       │
│              │                                                  │
│              ├──────────────────────────────────────────────┐   │
│              │                                              │   │
│              ▼                                              ▼   │
│   3a. File is marked with        3b. File is marked        │   │
│       destination folder              for delete/skip       │   │
│              │                              │                │   │
│              └──────────────┬───────────────┘                │   │
│                             │                                │   │
│                             ▼                                │   │
│   4. Auto-advance to next file                               │   │
│              │                                                │   │
│              ▼                                                │   │
│   5. Repeat until all files reviewed                         │   │
│              │                                                │   │
│              ▼                                                │   │
│   6. Show summary screen                                     │   │
│              │                                                │   │
│              ▼                                                │   │
│   7. User confirms → Execute all operations                  │   │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

### Keyboard Shortcuts in Advanced Mode

| Key | Action |
|-----|--------|
| 1-9 | Assign to folder position 1-9 in tree |
| D | Mark for deletion |
| S | Skip file |
| Backspace/↑ | Go back to previous file |
| N | Add new folder |
| F | Toggle folder tree panel |
| C | Connect selected folder |
| Enter | Execute all pending operations |
| Escape | Cancel/Close dialog |

---

## User Interface Design

### Main Window Layout Options

#### Option A: Side-by-Side Layout
```
┌─────────────────────────────────────────────────┐
│  [File Preview]  │  [Folder Tree]              │
│                  │                              │
│                  │                              │
├─────────────────────────────────────────────────┤
│  [Action Buttons]                               │
└─────────────────────────────────────────────────┘
```

#### Option B: Overlay Layout
```
┌─────────────────────────────────────────────────┐
│                                                 │
│              [File Preview]                     │
│                                                 │
│  ┌─────────────────────┐                       │
│  │ [Folder Tree Popup] │                       │
│  └─────────────────────┘                       │
├─────────────────────────────────────────────────┤
│  [Action Buttons]                               │
└─────────────────────────────────────────────────┘
```

#### Option C: Tab/Toggle Layout
```
┌─────────────────────────────────────────────────┐
│  [Preview Tab] | [Folder Tree Tab]              │
├─────────────────────────────────────────────────┤
│                                                 │
│  (Content based on selected tab)                │
│                                                 │
├─────────────────────────────────────────────────┤
│  [Action Buttons]                               │
└─────────────────────────────────────────────────┘
```

### Recommended: Split Panel with Toggle

```cpp
class AdvancedFileTinderDialog : public QDialog {
    // Main layout: QSplitter with resizable panels
    QSplitter* main_splitter_;
    
    // Left panel: File preview (existing)
    QWidget* preview_panel_;
    
    // Right panel: Folder tree (new)
    QWidget* folder_tree_panel_;
    QTreeWidget* folder_tree_widget_;
    
    // Toggle button to show/hide folder panel
    QPushButton* toggle_folder_panel_btn_;
    
    // Quick access bar at bottom
    QHBoxLayout* quick_access_bar_;
};
```

### Component Specifications

#### 1. File Preview Panel
- **Same as existing** FileTinderDialog
- Image preview with scaling
- Text file preview (first 2000 chars)
- Metadata display for other files
- File name, size, type, modified date

#### 2. Folder Tree Panel
- QTreeWidget-based folder tree
- Icons distinguish existing vs. virtual folders
- Checkboxes or selection highlight for assigned folders
- Context menu for folder operations
- Collapsible hierarchy
- Search/filter box at top

#### 3. Quick Access Bar
- Horizontal list of pinned/favorite folders
- Most recently used folders
- Connected folder groups
- Click to assign current file
- Drag to reorder

#### 4. Action Buttons
- Standard: Delete, Skip, Back
- Advanced-specific: "Move to Selected" (when folder selected in tree)
- Mode toggle: Switch between Basic/Advanced

#### 5. Summary/Review Screen
```
┌─────────────────────────────────────────────────────────────────┐
│                     REVIEW SUMMARY                              │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  Files to DELETE: 15                                            │
│  Files to KEEP: 25                                              │
│  Files to MOVE: 45                                              │
│  Files SKIPPED: 10                                              │
│                                                                 │
│  ═══════════════════════════════════════════════════════════   │
│                                                                 │
│  MOVE DESTINATIONS:                                             │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │ Folder                           │ Files │ Size         │   │
│  ├─────────────────────────────────────────────────────────┤   │
│  │ 📁 Images/Vacation               │   15  │ 125 MB       │   │
│  │ 📁 Documents/Work                │   12  │  45 MB       │   │
│  │ 📂 Images/2024/March (NEW)       │    8  │  32 MB       │   │
│  │ 📁 Videos                        │   10  │ 890 MB       │   │
│  └─────────────────────────────────────────────────────────┘   │
│                                                                 │
│  ⚠️ Folders to be created: 1                                    │
│                                                                 │
│  ┌──────────────┐  ┌──────────────┐  ┌────────────────────┐    │
│  │    Cancel    │  │ Edit Details │  │ Execute All ✓      │    │
│  └──────────────┘  └──────────────┘  └────────────────────┘    │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

---

## Technical Implementation

### New Classes to Create

#### 1. `StandaloneFileTinderDialog` (Basic Mode)
```cpp
// app/include/StandaloneFileTinderDialog.hpp

class StandaloneFileTinderDialog : public QDialog {
    Q_OBJECT

public:
    explicit StandaloneFileTinderDialog(const std::string& source_folder,
                                         DatabaseManager& db,
                                         QWidget* parent = nullptr);

private:
    // File management
    std::vector<FileToProcess> files_;
    size_t current_index_;
    
    // Destinations (Basic mode - simple list)
    std::map<std::string, std::string> file_destinations_;  // file_path -> destination_folder
    QStringList recent_folders_;
    
    // UI
    void setup_basic_ui();
    void on_move_to_folder();
    void show_folder_picker();
};
```

#### 2. `AdvancedFileTinderDialog` (Advanced Mode)
```cpp
// app/include/AdvancedFileTinderDialog.hpp

class AdvancedFileTinderDialog : public QDialog {
    Q_OBJECT

public:
    explicit AdvancedFileTinderDialog(const std::string& source_folder,
                                       DatabaseManager& db,
                                       QWidget* parent = nullptr);

private:
    // Folder tree management
    FolderTreeModel* folder_model_;
    QTreeView* folder_tree_view_;
    
    // Folder connections
    std::vector<FolderConnection> connections_;
    
    // UI panels
    QSplitter* main_splitter_;
    void setup_advanced_ui();
    void on_folder_clicked(const QModelIndex& index);
    void on_add_folder();
    void on_connect_folders();
};
```

#### 3. `FolderTreeModel` (Folder Tree Data Model)
```cpp
// app/include/FolderTreeModel.hpp

struct FolderNode {
    QString path;
    QString display_name;
    bool exists;                      // Existing folder vs. to-be-created
    bool is_pinned;                   // Pinned to quick access
    std::vector<QString> connected_folders;  // Connection group
    int assigned_file_count;
    FolderNode* parent;
    std::vector<std::unique_ptr<FolderNode>> children;
};

class FolderTreeModel : public QAbstractItemModel {
    Q_OBJECT

public:
    void set_root_folder(const QString& path);
    void add_folder(const QString& path, bool virtual_folder = false);
    void remove_folder(const QString& path);
    void connect_folders(const QStringList& paths);
    void disconnect_folder(const QString& path);
    
    FolderNode* node_at(const QModelIndex& index);
    QStringList get_virtual_folders() const;
    
signals:
    void folder_assigned(const QString& folder_path);
};
```

#### 4. `FileTinderExecutor` (Execution Engine)
```cpp
// app/include/FileTinderExecutor.hpp

struct ExecutionPlan {
    std::vector<std::string> files_to_delete;
    std::vector<std::pair<std::string, std::string>> files_to_move;  // source, dest
    std::vector<std::string> folders_to_create;
};

struct ExecutionResult {
    int files_deleted;
    int files_moved;
    int folders_created;
    int errors;
    std::vector<std::string> error_messages;
};

class FileTinderExecutor {
public:
    ExecutionResult execute(const ExecutionPlan& plan, 
                           std::function<void(int, int)> progress_callback = nullptr);
    
private:
    bool create_folders(const std::vector<std::string>& folders);
    bool move_files(const std::vector<std::pair<std::string, std::string>>& moves);
    bool delete_files(const std::vector<std::string>& files);
};
```

### Database Schema Extensions

```sql
-- Session state for File Tinder (already exists, extended)
CREATE TABLE IF NOT EXISTS file_tinder_state (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    folder_path TEXT NOT NULL,
    file_path TEXT NOT NULL,
    decision TEXT NOT NULL CHECK (decision IN ('pending', 'keep', 'delete', 'ignore', 'move')),
    destination_folder TEXT,  -- NEW: For move operations
    timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,
    UNIQUE(folder_path, file_path)
);

-- Folder tree configuration (NEW)
CREATE TABLE IF NOT EXISTS tinder_folder_tree (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    session_folder TEXT NOT NULL,  -- Source folder being sorted
    folder_path TEXT NOT NULL,
    display_name TEXT,
    is_virtual BOOLEAN DEFAULT 0,  -- 1 = will be created
    is_pinned BOOLEAN DEFAULT 0,   -- 1 = show in quick access
    parent_path TEXT,
    sort_order INTEGER DEFAULT 0,
    UNIQUE(session_folder, folder_path)
);

-- Folder connections (NEW)
CREATE TABLE IF NOT EXISTS tinder_folder_connections (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    session_folder TEXT NOT NULL,
    group_id INTEGER NOT NULL,
    folder_path TEXT NOT NULL,
    UNIQUE(session_folder, folder_path)
);
```

### Integration with Existing Code

The standalone File Tinder builds upon the existing implementation:

```cpp
// Inheritance/composition approach
class StandaloneFileTinderDialog : public FileTinderDialog {
    // Extends base class with:
    // - Move-to-folder functionality
    // - Folder destination tracking
    // - Enhanced review screen
};

class AdvancedFileTinderDialog : public StandaloneFileTinderDialog {
    // Extends standalone with:
    // - Visual folder tree
    // - Node connections
    // - Dynamic folder creation
};
```

### Files to Create/Modify

**New Files:**
1. `app/include/StandaloneFileTinderDialog.hpp`
2. `app/lib/StandaloneFileTinderDialog.cpp`
3. `app/include/AdvancedFileTinderDialog.hpp`
4. `app/lib/AdvancedFileTinderDialog.cpp`
5. `app/include/FolderTreeModel.hpp`
6. `app/lib/FolderTreeModel.cpp`
7. `app/include/FileTinderExecutor.hpp`
8. `app/lib/FileTinderExecutor.cpp`

**Modified Files:**
1. `app/include/DatabaseManager.hpp` - Add folder tree methods
2. `app/lib/DatabaseManager.cpp` - Implement folder tree methods
3. `app/include/ui_constants.hpp` - Add new UI constants
4. `CMakeLists.txt` - Add new source files

---

## Implementation Roadmap

### Phase 1: Standalone Basic Mode
**Duration:** 3-5 days
**Priority:** High

- [ ] Create `StandaloneFileTinderDialog` class
- [ ] Implement "Move to Folder" dialog
- [ ] Add recent folders tracking
- [ ] Extend database schema for destinations
- [ ] Implement file move execution
- [ ] Add "Create New Folder" in folder picker
- [ ] Update review screen with move destinations
- [ ] Add unit tests for basic functionality

### Phase 2: Folder Tree Infrastructure
**Duration:** 4-6 days
**Priority:** High

- [ ] Create `FolderTreeModel` class
- [ ] Implement folder tree widget with Qt
- [ ] Add folder CRUD operations
- [ ] Implement virtual folder support
- [ ] Add folder tree persistence to database
- [ ] Create folder tree context menu
- [ ] Add unit tests for folder tree model

### Phase 3: Advanced Mode UI
**Duration:** 5-7 days
**Priority:** Medium

- [ ] Create `AdvancedFileTinderDialog` class
- [ ] Implement split panel layout
- [ ] Add folder click-to-assign functionality
- [ ] Implement quick access bar
- [ ] Add keyboard shortcuts for folder positions
- [ ] Implement folder panel toggle
- [ ] Add folder search/filter
- [ ] Update review screen for advanced mode

### Phase 4: Folder Connections
**Duration:** 3-4 days
**Priority:** Medium

- [ ] Design connection visualization
- [ ] Implement connection data model
- [ ] Add connect/disconnect UI
- [ ] Implement "connected folders only" filter
- [ ] Add connection persistence
- [ ] Visual feedback for connected groups

### Phase 5: Execution Engine
**Duration:** 3-4 days
**Priority:** High

- [ ] Create `FileTinderExecutor` class
- [ ] Implement folder creation logic
- [ ] Implement file move with conflict handling
- [ ] Add progress reporting
- [ ] Implement rollback on errors
- [ ] Add detailed execution logging
- [ ] Unit tests for executor

### Phase 6: Polish & Integration
**Duration:** 3-4 days
**Priority:** Medium

- [ ] Add animations/transitions (optional)
- [ ] Implement undo for executed operations
- [ ] Add drag-drop reordering in folder tree
- [ ] Performance optimization for large file sets
- [ ] Integration with main application menu
- [ ] User documentation
- [ ] End-to-end testing

### Total Estimated Time: 21-30 days

---

## Appendix A: UI Constants to Add

```cpp
// app/include/ui_constants.hpp (additions)

namespace ui::dimensions {
    // Standalone File Tinder
    constexpr int kStandaloneFileTinderMinWidth = 900;
    constexpr int kStandaloneFileTinderMinHeight = 700;
    
    // Advanced File Tinder
    constexpr int kAdvancedFileTinderMinWidth = 1200;
    constexpr int kAdvancedFileTinderMinHeight = 800;
    constexpr int kFolderTreePanelMinWidth = 300;
    constexpr int kFolderTreePanelMaxWidth = 500;
    
    // Quick Access Bar
    constexpr int kQuickAccessBarHeight = 60;
    constexpr int kQuickAccessButtonWidth = 120;
}

namespace ui::icons {
    constexpr const char* kFolderExisting = "folder";
    constexpr const char* kFolderVirtual = "folder-new";
    constexpr const char* kFolderConnected = "folder-linked";
    constexpr const char* kConnectionLine = "link";
}
```

---

## Appendix B: Example Usage Scenarios

### Scenario 1: Photo Organization
1. User opens File Tinder on Downloads folder
2. Switches to Advanced Mode
3. Creates folder tree: Images → 2024 → Vacation, Family, Events
4. Swipes through photos
5. Clicks appropriate folder for each photo
6. Reviews summary showing 150 photos going to 8 folders
7. Executes - folders created, files moved

### Scenario 2: Document Cleanup
1. User opens File Tinder on Documents folder
2. Uses Basic Mode initially
3. Sees document, clicks "Move to Folder"
4. Creates "Archive/2023" folder on the fly
5. Continues sorting
6. Switches to Advanced Mode for visual overview
7. Connects "Work" and "Projects" folders
8. Completes sorting with connected folders as quick access

### Scenario 3: Download Cleanup with Mixed Content
1. User has 500 files in Downloads
2. Opens Advanced File Tinder
3. Sets up destination tree:
   - Keep (stay in Downloads)
   - Images/Screenshots
   - Documents/Receipts
   - Software/Installers
   - Delete (trash)
4. Uses keyboard shortcuts 1-5 for quick sorting
5. Reviews: 200 keep, 150 move, 150 delete
6. Executes all at once

---

## Appendix C: Error Handling

### File Operation Errors
- **File locked**: Skip and report
- **Permission denied**: Skip and report
- **Disk full**: Stop execution, report progress
- **Path too long**: Suggest rename or shorter destination
- **File already exists**: Prompt for action (overwrite/rename/skip)

### Folder Creation Errors
- **Invalid characters**: Prompt for valid name
- **Folder exists**: Use existing folder
- **Permission denied**: Report and skip files destined for this folder
- **Path too long**: Suggest shorter name

### Recovery
- All operations are staged before execution
- Failed operations are logged with reasons
- Partial success is allowed (completed operations persist)
- User can retry failed operations

---

## Appendix D: Configuration Options

```cpp
struct FileTinderConfig {
    // UI preferences
    bool start_in_advanced_mode = false;
    bool show_folder_tree_by_default = true;
    int folder_tree_panel_width = 350;
    
    // Behavior
    bool auto_advance_after_action = true;
    bool confirm_delete_individual = false;
    bool confirm_delete_batch = true;
    bool create_folders_immediately = false;  // vs. at execution time
    
    // File handling
    bool include_hidden_files = false;
    bool recursive_scan = false;
    QStringList excluded_extensions;
    
    // Persistence
    bool save_session_on_close = true;
    bool remember_folder_tree = true;
    int max_recent_folders = 10;
};
```

---

*Document Version: 1.0*  
*Last Updated: February 2026*  
*Author: AI File Sorter Development Team*
