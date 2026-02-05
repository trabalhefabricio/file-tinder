#ifndef ADVANCED_FILE_TINDER_DIALOG_HPP
#define ADVANCED_FILE_TINDER_DIALOG_HPP

#include "StandaloneFileTinderDialog.hpp"
#include <QSplitter>
#include <QTreeView>
#include <QListWidget>

class FolderTreeModel;
class MindMapView;

class AdvancedFileTinderDialog : public StandaloneFileTinderDialog {
    Q_OBJECT

public:
    explicit AdvancedFileTinderDialog(const QString& source_folder,
                                       DatabaseManager& db,
                                       QWidget* parent = nullptr);
    ~AdvancedFileTinderDialog() override;
    
private:
    // Advanced mode components
    QSplitter* main_splitter_;
    QWidget* folder_panel_;
    MindMapView* mind_map_view_;
    QListWidget* quick_access_list_;
    
    // Folder tree model
    FolderTreeModel* folder_model_;
    
    // UI setup
    void setup_ui() override;
    void setup_folder_panel();
    void setup_quick_access_bar();
    void setup_advanced_buttons();
    
    // Folder operations
    void on_folder_clicked(const QString& folder_path);
    void on_folder_double_clicked(const QString& folder_path);
    void on_folder_context_menu(const QString& folder_path, const QPoint& pos);
    void on_add_folder();
    void on_add_subfolder();
    void on_remove_folder();
    void on_pin_folder();
    void on_connect_folders();
    
    // Quick access
    void update_quick_access();
    void on_quick_access_clicked(QListWidgetItem* item);
    
    // Override actions
    void on_move_to_folder() override;
    void on_finish() override;
    
    // Keyboard shortcuts
    void keyPressEvent(QKeyEvent* event) override;
    
    // Load/save
    void load_folder_tree();
    void save_folder_tree();
    
signals:
    void switch_to_basic_mode();
    
private slots:
    void on_toggle_folder_panel();
    void on_zoom_in();
    void on_zoom_out();
    void on_zoom_fit();
};

#endif // ADVANCED_FILE_TINDER_DIALOG_HPP
