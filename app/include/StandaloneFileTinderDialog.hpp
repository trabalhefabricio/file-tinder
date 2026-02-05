#ifndef STANDALONE_FILE_TINDER_DIALOG_HPP
#define STANDALONE_FILE_TINDER_DIALOG_HPP

#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QProgressBar>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QListWidget>
#include <QComboBox>
#include <QString>
#include <QStringList>
#include <vector>
#include <memory>

class DatabaseManager;
class QPropertyAnimation;
class QGraphicsOpacityEffect;

struct FileToProcess {
    QString path;
    QString name;
    QString extension;
    qint64 size;
    QString modified_date;
    QString decision;           // "pending", "keep", "delete", "skip", "move"
    QString destination_folder; // For move operations
    QString mime_type;          // MIME type for filtering
};

// File filter types
enum class FileFilterType {
    All,
    Images,
    Videos,
    Audio,
    Documents,
    Archives,
    Other
};

class StandaloneFileTinderDialog : public QDialog {
    Q_OBJECT

public:
    explicit StandaloneFileTinderDialog(const QString& source_folder,
                                         DatabaseManager& db,
                                         QWidget* parent = nullptr);
    ~StandaloneFileTinderDialog() override;
    
    // Initialize the dialog - must be called after construction
    // This allows derived classes to properly initialize before UI setup
    virtual void initialize();
    
protected:
    // File management
    std::vector<FileToProcess> files_;
    std::vector<int> filtered_indices_;  // Indices into files_ after filtering
    int current_filtered_index_;         // Current position in filtered list
    QString source_folder_;
    DatabaseManager& db_;
    FileFilterType current_filter_;
    
    // Statistics
    int keep_count_;
    int delete_count_;
    int skip_count_;
    int move_count_;
    
    // UI Components
    QLabel* preview_label_;
    QLabel* file_info_label_;
    QLabel* progress_label_;
    QLabel* stats_label_;
    QProgressBar* progress_bar_;
    QComboBox* filter_combo_;
    QLabel* shortcuts_label_;
    
    QPushButton* back_btn_;
    QPushButton* delete_btn_;
    QPushButton* skip_btn_;
    QPushButton* keep_btn_;
    QPushButton* move_btn_;
    QPushButton* finish_btn_;
    QPushButton* switch_mode_btn_;
    QPushButton* help_btn_;
    
    // Animation
    QPropertyAnimation* swipe_animation_;
    
    // Initialization
    virtual void setup_ui();
    void scan_files();
    void load_session_state();
    void save_session_state();
    
    // Filtering
    void apply_filter(FileFilterType filter);
    void rebuild_filtered_indices();
    bool file_matches_filter(const FileToProcess& file) const;
    
    // File display
    void show_current_file();
    void update_preview(const QString& file_path);
    void update_file_info(const FileToProcess& file);
    void update_progress();
    void update_stats();
    
    // Animation
    void animate_swipe(bool forward);
    
    // Actions
    virtual void on_keep();
    virtual void on_delete();
    virtual void on_skip();
    virtual void on_back();
    virtual void on_move_to_folder();
    virtual void on_finish();
    void advance_to_next();
    void go_to_previous();
    
    // Helper to update decision counts (deduplication)
    void update_decision_count(const QString& old_decision, int delta);
    int get_current_file_index() const;  // Get actual file index from filtered index
    
    // Folder picker
    QString show_folder_picker();
    
    // Review screen
    void show_review_summary();
    void execute_decisions();
    
    // Help
    void show_shortcuts_help();
    
    // Keyboard shortcuts
    void keyPressEvent(QKeyEvent* event) override;
    void closeEvent(QCloseEvent* event) override;
    
signals:
    void session_completed();
    void switch_to_advanced_mode();
    
protected slots:
    void on_switch_mode_clicked();
    void on_filter_changed(int index);
};

#endif // STANDALONE_FILE_TINDER_DIALOG_HPP
