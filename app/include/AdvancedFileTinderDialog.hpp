#ifndef ADVANCED_FILE_TINDER_DIALOG_HPP
#define ADVANCED_FILE_TINDER_DIALOG_HPP

#include "StandaloneFileTinderDialog.hpp"
#include <QListWidget>
#include <QScrollArea>

class FolderTreeModel;
class MindMapView;
class FilterWidget;

class AdvancedFileTinderDialog : public StandaloneFileTinderDialog {
    Q_OBJECT

public:
    explicit AdvancedFileTinderDialog(const QString& source_folder,
                                       DatabaseManager& db,
                                       QWidget* parent = nullptr);
    ~AdvancedFileTinderDialog() override;
    
    // Override initialize to set up advanced mode properly
    void initialize() override;
    
private:
    // Advanced mode components (vertical layout - no splitter)
    QScrollArea* scroll_area_;
    QWidget* main_content_;
    MindMapView* mind_map_view_;
    QWidget* file_info_panel_;
    QLabel* adv_file_icon_label_;
    QLabel* file_name_label_;
    QLabel* file_details_label_;
    QWidget* quick_access_panel_;
    QListWidget* quick_access_list_;
    FilterWidget* filter_widget_;
    
    // Folder tree model
    FolderTreeModel* folder_model_;
    
    // Quick access management (manual, limited to 10)
    QStringList quick_access_folders_;
    static const int kMaxQuickAccess = 10;
    
    // UI setup
    void setup_ui() override;
    void setup_filter_bar();
    void setup_mind_map();
    void setup_file_info_panel();
    void setup_quick_access_panel();
    void setup_action_buttons();
    
    // Folder operations
    void on_folder_clicked(const QString& folder_path);
    void on_add_node_clicked();
    void prompt_add_folder();
    void on_folder_context_menu(const QString& folder_path, const QPoint& pos);
    
    // Quick access (manual management)
    void load_quick_access();
    void save_quick_access();
    void add_to_quick_access(const QString& folder_path);
    void remove_from_quick_access(int index);
    void update_quick_access_display();
    void on_quick_access_clicked(QListWidgetItem* item);
    void on_add_quick_access();
    void on_remove_quick_access();
    
    // File info display
    void update_file_info_display();
    QString get_file_type_icon(const QString& path);
    
    // Override actions
    void on_finish() override;
    void on_undo() override;
    void show_current_file() override;  // Override base class implementation
    void closeEvent(QCloseEvent* event) override;
    void reject() override;  // Save folder tree/quick access before closing
    bool eventFilter(QObject* obj, QEvent* event) override;  // Double-click to open file
    
    // Keyboard shortcuts
    void keyPressEvent(QKeyEvent* event) override;
    
    // Filter/sort handling
    void on_filter_changed();
    void on_sort_changed();
    
    // Load/save
    void load_folder_tree();
    void save_folder_tree();
    
signals:
    void switch_to_basic_mode();
    
private slots:
    void on_zoom_in();
    void on_zoom_out();
    void on_zoom_fit();
};

#endif // ADVANCED_FILE_TINDER_DIALOG_HPP
