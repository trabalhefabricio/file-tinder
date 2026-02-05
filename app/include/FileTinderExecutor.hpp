#ifndef FILE_TINDER_EXECUTOR_HPP
#define FILE_TINDER_EXECUTOR_HPP

#include <QString>
#include <QStringList>
#include <vector>
#include <functional>

struct ExecutionPlan {
    std::vector<QString> files_to_delete;
    std::vector<std::pair<QString, QString>> files_to_move;  // source, dest
    std::vector<QString> folders_to_create;
};

struct ExecutionResult {
    int files_deleted = 0;
    int files_moved = 0;
    int folders_created = 0;
    int errors = 0;
    QStringList error_messages;
    bool success = true;
};

class FileTinderExecutor {
public:
    using ProgressCallback = std::function<void(int current, int total, const QString& message)>;
    
    FileTinderExecutor();
    ~FileTinderExecutor();
    
    ExecutionResult execute(const ExecutionPlan& plan, ProgressCallback progress_callback = nullptr);
    
    // Configuration
    void set_move_to_trash(bool use_trash) { use_trash_ = use_trash; }
    void set_overwrite_existing(bool overwrite) { overwrite_existing_ = overwrite; }
    
private:
    bool use_trash_;
    bool overwrite_existing_;
    
    bool create_folders(const std::vector<QString>& folders, ExecutionResult& result,
                       ProgressCallback callback, int& progress, int total);
    bool move_files(const std::vector<std::pair<QString, QString>>& moves, 
                   ExecutionResult& result, ProgressCallback callback, int& progress, int total);
    bool delete_files(const std::vector<QString>& files, ExecutionResult& result,
                     ProgressCallback callback, int& progress, int total);
    bool move_to_trash(const QString& file_path);
};

#endif // FILE_TINDER_EXECUTOR_HPP
