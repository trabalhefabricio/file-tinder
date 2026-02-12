#include "FolderTreeModel.hpp"
#include "DatabaseManager.hpp"
#include <QDir>
#include <QFileInfo>
#include <QIcon>

FolderTreeModel::FolderTreeModel(QObject* parent)
    : QAbstractItemModel(parent)
    , root_(std::make_unique<FolderNode>())
    , next_connection_group_id_(1) {
}

FolderTreeModel::~FolderTreeModel() = default;

QModelIndex FolderTreeModel::index(int row, int column, const QModelIndex& parent) const {
    if (!hasIndex(row, column, parent)) {
        return QModelIndex();
    }
    
    FolderNode* parent_node = parent.isValid() ? 
        static_cast<FolderNode*>(parent.internalPointer()) : root_.get();
    
    if (row >= 0 && row < static_cast<int>(parent_node->children.size())) {
        return createIndex(row, column, parent_node->children[row].get());
    }
    
    return QModelIndex();
}

QModelIndex FolderTreeModel::parent(const QModelIndex& index) const {
    if (!index.isValid()) {
        return QModelIndex();
    }
    
    FolderNode* node = static_cast<FolderNode*>(index.internalPointer());
    FolderNode* parent_node = node->parent;
    
    if (parent_node == root_.get() || parent_node == nullptr) {
        return QModelIndex();
    }
    
    // Find row of parent in grandparent
    FolderNode* grandparent = parent_node->parent;
    if (!grandparent) {
        grandparent = root_.get();
    }
    
    for (size_t i = 0; i < grandparent->children.size(); ++i) {
        if (grandparent->children[i].get() == parent_node) {
            return createIndex(static_cast<int>(i), 0, parent_node);
        }
    }
    
    return QModelIndex();
}

int FolderTreeModel::rowCount(const QModelIndex& parent) const {
    FolderNode* parent_node = parent.isValid() ? 
        static_cast<FolderNode*>(parent.internalPointer()) : root_.get();
    return static_cast<int>(parent_node->children.size());
}

int FolderTreeModel::columnCount(const QModelIndex&) const {
    return 1;
}

QVariant FolderTreeModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid()) {
        return QVariant();
    }
    
    FolderNode* node = static_cast<FolderNode*>(index.internalPointer());
    
    switch (role) {
        case Qt::DisplayRole:
        case DisplayNameRole:
            return node->display_name;
        case PathRole:
            return node->path;
        case ExistsRole:
            return node->exists;
        case IsPinnedRole:
            return node->is_pinned;
        case IsConnectedRole:
            return node->is_connected;
        case IsExternalRole:
            return node->is_external;
        case ConnectionGroupRole:
            return node->connection_group_id;
        case FileCountRole:
            return node->assigned_file_count;
        case Qt::DecorationRole:
            if (!node->exists) {
                return QIcon::fromTheme("folder-new");
            } else if (node->is_connected) {
                return QIcon::fromTheme("folder-remote");
            }
            return QIcon::fromTheme("folder");
        case Qt::ToolTipRole:
            return node->path;
    }
    
    return QVariant();
}

Qt::ItemFlags FolderTreeModel::flags(const QModelIndex& index) const {
    if (!index.isValid()) {
        return Qt::NoItemFlags;
    }
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

QHash<int, QByteArray> FolderTreeModel::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[PathRole] = "path";
    roles[DisplayNameRole] = "displayName";
    roles[ExistsRole] = "exists";
    roles[IsPinnedRole] = "isPinned";
    roles[IsConnectedRole] = "isConnected";
    roles[IsExternalRole] = "isExternal";
    roles[ConnectionGroupRole] = "connectionGroup";
    roles[FileCountRole] = "fileCount";
    return roles;
}

void FolderTreeModel::set_root_folder(const QString& path) {
    beginResetModel();
    
    root_ = std::make_unique<FolderNode>();
    root_->path = path;
    root_->display_name = QFileInfo(path).fileName();
    if (root_->display_name.isEmpty()) {
        root_->display_name = path;
    }
    root_->exists = QDir(path).exists();
    
    // Start with root only — destination folders are added manually via
    // the "+" button (Create New Folder or Add Existing Folder).
    
    endResetModel();
    emit folder_structure_changed();
}

void FolderTreeModel::scan_directory(FolderNode* parent, const QString& path, int depth) {
    if (depth > 3) return; // Limit depth
    
    QDir dir(path);
    QStringList subdirs = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
    
    for (const QString& subdir : subdirs) {
        auto child = std::make_unique<FolderNode>();
        child->path = dir.absoluteFilePath(subdir);
        child->display_name = subdir;
        child->exists = true;
        child->parent = parent;
        
        // Recursively scan subdirectories
        scan_directory(child.get(), child->path, depth + 1);
        
        parent->children.push_back(std::move(child));
    }
}

void FolderTreeModel::add_folder(const QString& path, bool virtual_folder) {
    // Find parent folder
    QString parent_path = QFileInfo(path).absolutePath();
    FolderNode* parent_node = find_node(parent_path);
    
    if (!parent_node) {
        parent_node = root_.get();
    }
    
    // Check if folder already exists in model
    if (find_node(path)) {
        return;
    }
    
    int row = static_cast<int>(parent_node->children.size());
    QModelIndex parent_index = index_for_node(parent_node);
    
    beginInsertRows(parent_index, row, row);
    
    auto child = std::make_unique<FolderNode>();
    child->path = path;
    child->display_name = QFileInfo(path).fileName();
    child->exists = !virtual_folder && QDir(path).exists();
    child->is_external = !path.startsWith(root_->path);
    child->parent = parent_node;
    
    parent_node->children.push_back(std::move(child));
    
    endInsertRows();
    emit folder_structure_changed();
}

void FolderTreeModel::remove_folder(const QString& path) {
    FolderNode* node = find_node(path);
    if (!node || !node->parent) {
        return;
    }
    
    FolderNode* parent = node->parent;
    QModelIndex parent_index = index_for_node(parent);
    
    // Find row of node in parent
    int row = -1;
    for (size_t i = 0; i < parent->children.size(); ++i) {
        if (parent->children[i].get() == node) {
            row = static_cast<int>(i);
            break;
        }
    }
    
    if (row < 0) return;
    
    beginRemoveRows(parent_index, row, row);
    parent->children.erase(parent->children.begin() + row);
    endRemoveRows();
    
    emit folder_structure_changed();
}

void FolderTreeModel::set_folder_pinned(const QString& path, bool pinned) {
    FolderNode* node = find_node(path);
    if (node) {
        node->is_pinned = pinned;
        QModelIndex idx = index_for_node(node);
        emit dataChanged(idx, idx, {IsPinnedRole});
    }
}

void FolderTreeModel::connect_folders(const QStringList& paths, int group_id) {
    if (group_id < 0) {
        group_id = next_connection_group_id_++;
    }
    
    for (const QString& path : paths) {
        FolderNode* node = find_node(path);
        if (node) {
            node->is_connected = true;
            node->connection_group_id = group_id;
            QModelIndex idx = index_for_node(node);
            emit dataChanged(idx, idx, {IsConnectedRole, ConnectionGroupRole});
        }
    }
    
    emit folder_structure_changed();
}

void FolderTreeModel::disconnect_folder(const QString& path) {
    FolderNode* node = find_node(path);
    if (node) {
        node->is_connected = false;
        node->connection_group_id = -1;
        QModelIndex idx = index_for_node(node);
        emit dataChanged(idx, idx, {IsConnectedRole, ConnectionGroupRole});
        emit folder_structure_changed();
    }
}

void FolderTreeModel::assign_file_to_folder(const QString& folder_path) {
    FolderNode* node = find_node(folder_path);
    if (node) {
        node->assigned_file_count++;
        QModelIndex idx = index_for_node(node);
        emit dataChanged(idx, idx, {FileCountRole});
        emit folder_assigned(folder_path);
    }
}

void FolderTreeModel::unassign_file_from_folder(const QString& folder_path) {
    FolderNode* node = find_node(folder_path);
    if (node && node->assigned_file_count > 0) {
        node->assigned_file_count--;
        QModelIndex idx = index_for_node(node);
        emit dataChanged(idx, idx, {FileCountRole});
    }
}

void FolderTreeModel::clear_assignments() {
    std::function<void(FolderNode*)> clear_recursive = [&](FolderNode* node) {
        node->assigned_file_count = 0;
        for (auto& child : node->children) {
            clear_recursive(child.get());
        }
    };
    clear_recursive(root_.get());
    emit dataChanged(QModelIndex(), QModelIndex());
}

FolderNode* FolderTreeModel::node_at(const QModelIndex& index) const {
    if (!index.isValid()) {
        return root_.get();
    }
    return static_cast<FolderNode*>(index.internalPointer());
}

FolderNode* FolderTreeModel::find_node(const QString& path) const {
    return find_node_recursive(root_.get(), path);
}

FolderNode* FolderTreeModel::find_node_recursive(FolderNode* node, const QString& path) const {
    if (node->path == path) {
        return node;
    }
    
    for (const auto& child : node->children) {
        FolderNode* found = find_node_recursive(child.get(), path);
        if (found) {
            return found;
        }
    }
    
    return nullptr;
}

QModelIndex FolderTreeModel::index_for_path(const QString& path) const {
    FolderNode* node = find_node(path);
    return index_for_node(node);
}

QModelIndex FolderTreeModel::index_for_node(FolderNode* node) const {
    if (!node || node == root_.get()) {
        return QModelIndex();
    }
    
    FolderNode* parent = node->parent;
    if (!parent) {
        return QModelIndex();
    }
    
    for (size_t i = 0; i < parent->children.size(); ++i) {
        if (parent->children[i].get() == node) {
            return createIndex(static_cast<int>(i), 0, node);
        }
    }
    
    return QModelIndex();
}

QStringList FolderTreeModel::get_virtual_folders() const {
    QStringList result;
    collect_virtual_folders(root_.get(), result);
    return result;
}

void FolderTreeModel::collect_virtual_folders(FolderNode* node, QStringList& result) const {
    if (!node->exists) {
        result.append(node->path);
    }
    for (const auto& child : node->children) {
        collect_virtual_folders(child.get(), result);
    }
}

QStringList FolderTreeModel::get_all_folder_paths() const {
    QStringList result;
    std::function<void(FolderNode*)> collect = [&](FolderNode* node) {
        if (node != root_.get()) {
            result.append(node->path);
        }
        for (const auto& child : node->children) {
            collect(child.get());
        }
    };
    if (root_) collect(root_.get());
    return result;
}

QStringList FolderTreeModel::get_pinned_folders() const {
    QStringList result;
    std::function<void(FolderNode*)> collect = [&](FolderNode* node) {
        if (node->is_pinned) {
            result.append(node->path);
        }
        for (const auto& child : node->children) {
            collect(child.get());
        }
    };
    collect(root_.get());
    return result;
}

QStringList FolderTreeModel::get_connected_folders(int group_id) const {
    QStringList result;
    std::function<void(FolderNode*)> collect = [&](FolderNode* node) {
        if (node->is_connected && node->connection_group_id == group_id) {
            result.append(node->path);
        }
        for (const auto& child : node->children) {
            collect(child.get());
        }
    };
    collect(root_.get());
    return result;
}

void FolderTreeModel::load_from_database(DatabaseManager& db, const QString& session_folder) {
    auto entries = db.get_folder_tree(session_folder);
    auto connections = db.get_folder_connections(session_folder);
    
    for (const auto& entry : entries) {
        add_folder(entry.folder_path, entry.is_virtual);
        FolderNode* node = find_node(entry.folder_path);
        if (node) {
            node->display_name = entry.display_name;
            node->is_pinned = entry.is_pinned;
        }
    }
    
    for (const auto& conn : connections) {
        FolderNode* node = find_node(conn.folder_path);
        if (node) {
            node->is_connected = true;
            node->connection_group_id = conn.group_id;
            if (conn.group_id >= next_connection_group_id_) {
                next_connection_group_id_ = conn.group_id + 1;
            }
        }
    }
}

void FolderTreeModel::save_to_database(DatabaseManager& db, const QString& session_folder) {
    std::function<void(FolderNode*)> save_recursive = [&](FolderNode* node) {
        if (node != root_.get()) {
            FolderTreeEntry entry;
            entry.folder_path = node->path;
            entry.display_name = node->display_name;
            entry.is_virtual = !node->exists;
            entry.is_pinned = node->is_pinned;
            entry.parent_path = node->parent ? node->parent->path : "";
            db.save_folder_tree_entry(session_folder, entry);
            
            if (node->is_connected) {
                db.add_folder_connection(session_folder, node->connection_group_id, node->path);
            }
        }
        
        for (const auto& child : node->children) {
            save_recursive(child.get());
        }
    };
    save_recursive(root_.get());
}
