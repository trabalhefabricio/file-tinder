#include "AiFileTinderDialog.hpp"
#include "FolderTreeModel.hpp"
#include "MindMapView.hpp"
#include "DatabaseManager.hpp"
#include "AppLogger.hpp"
#include "ui_constants.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QGroupBox>
#include <QMessageBox>
#include <QProgressDialog>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QNetworkRequest>
#include <QFileInfo>
#include <QDir>
#include <QMimeDatabase>
#include <QApplication>
#include <QScreen>
#include <QTimer>
#include <QTextBrowser>
#include <QEventLoop>
#include <QElapsedTimer>
#include <QDateTime>
#include <QScrollBar>
#include <QMenu>
#include <QProgressBar>

// Maximum number of new folders the AI can create in one run
static const int kMaxAiGeneratedFolders = 30;
// Batch size for API calls
static const int kBatchSize = 50;
// Timeout for API requests (ms) — higher for local LLMs
static const int kCloudTimeoutMs = 60000;
static const int kLocalTimeoutMs = 120000;

// ============================================================
// AiSetupDialog
// ============================================================

AiSetupDialog::AiSetupDialog(const QStringList& existing_folders,
                             int file_count,
                             DatabaseManager& db,
                             const QString& session_folder,
                             QWidget* parent)
    : QDialog(parent)
    , db_(db)
    , session_folder_(session_folder)
    , existing_folders_(existing_folders)
    , file_count_(file_count) {
    setWindowTitle("AI Sorting Setup");
    setMinimumSize(ui::scaling::scaled(520), ui::scaling::scaled(580));
    build_ui();
    load_saved_provider();
}

void AiSetupDialog::build_ui() {
    auto* main_layout = new QVBoxLayout(this);
    main_layout->setSpacing(10);
    main_layout->setContentsMargins(18, 18, 18, 18);

    auto* header = new QLabel("\xF0\x9F\xA4\x96 AI Sorting Configuration");
    header->setStyleSheet("font-size: 18px; font-weight: bold; color: #3498db;");
    header->setAlignment(Qt::AlignCenter);
    main_layout->addWidget(header);

    // ── Provider Group ──
    auto* prov_group = new QGroupBox("AI Provider");
    prov_group->setStyleSheet("QGroupBox { font-weight: bold; }");
    auto* prov_layout = new QVBoxLayout(prov_group);

    auto* prov_row = new QHBoxLayout();
    prov_row->addWidget(new QLabel("Provider:"));
    provider_combo_ = new QComboBox();
    provider_combo_->setEditable(true);
    provider_combo_->addItems({"OpenAI", "Anthropic", "Google Gemini", "Mistral",
                               "Groq", "OpenRouter", "Ollama (Local)", "LM Studio (Local)",
                               "Custom"});
    QStringList saved = db_.get_ai_provider_names();
    for (const QString& name : saved) {
        if (provider_combo_->findText(name) < 0)
            provider_combo_->addItem(name);
    }
    prov_row->addWidget(provider_combo_, 1);
    prov_layout->addLayout(prov_row);

    auto* key_row = new QHBoxLayout();
    key_row->addWidget(new QLabel("API Key:"));
    api_key_edit_ = new QLineEdit();
    api_key_edit_->setEchoMode(QLineEdit::Password);
    api_key_edit_->setPlaceholderText("sk-... (leave empty for local LLM)");
    key_row->addWidget(api_key_edit_, 1);
    prov_layout->addLayout(key_row);

    auto* ep_row = new QHBoxLayout();
    ep_row->addWidget(new QLabel("Endpoint:"));
    endpoint_edit_ = new QLineEdit();
    endpoint_edit_->setPlaceholderText("https://api.openai.com/v1/chat/completions");
    ep_row->addWidget(endpoint_edit_, 1);
    prov_layout->addLayout(ep_row);

    auto* model_row = new QHBoxLayout();
    model_row->addWidget(new QLabel("Model:"));
    model_edit_ = new QLineEdit();
    model_edit_->setPlaceholderText("gpt-4o-mini");
    model_row->addWidget(model_edit_, 1);
    prov_layout->addLayout(model_row);

    auto* rate_row = new QHBoxLayout();
    rate_row->addWidget(new QLabel("Rate limit (req/min):"));
    rate_limit_spin_ = new QSpinBox();
    rate_limit_spin_->setRange(1, 10000);
    rate_limit_spin_->setValue(60);
    rate_limit_spin_->setToolTip("Max API requests per minute");
    rate_row->addWidget(rate_limit_spin_);
    rate_row->addStretch();
    prov_layout->addLayout(rate_row);

    // Provider preset auto-fill
    connect(provider_combo_, &QComboBox::currentTextChanged, this, [this](const QString& text) {
        QString api_key, endpoint, model;
        bool is_local = false;
        int rpm = 60;
        if (db_.get_ai_provider(text, api_key, endpoint, model, is_local, rpm)) {
            api_key_edit_->setText(api_key);
            endpoint_edit_->setText(endpoint);
            model_edit_->setText(model);
            rate_limit_spin_->setValue(rpm);
            update_cost_estimate();
            return;
        }
        if (text == "OpenAI") {
            endpoint_edit_->setText("https://api.openai.com/v1/chat/completions");
            model_edit_->setText("gpt-4o-mini");
            rate_limit_spin_->setValue(60);
        } else if (text == "Anthropic") {
            endpoint_edit_->setText("https://api.anthropic.com/v1/messages");
            model_edit_->setText("claude-3-haiku-20240307");
            rate_limit_spin_->setValue(50);
        } else if (text == "Google Gemini") {
            endpoint_edit_->setText("https://generativelanguage.googleapis.com/v1beta/models");
            model_edit_->setText("gemini-1.5-flash");
            rate_limit_spin_->setValue(60);
        } else if (text == "Mistral") {
            endpoint_edit_->setText("https://api.mistral.ai/v1/chat/completions");
            model_edit_->setText("mistral-small-latest");
            rate_limit_spin_->setValue(60);
        } else if (text == "Groq") {
            endpoint_edit_->setText("https://api.groq.com/openai/v1/chat/completions");
            model_edit_->setText("llama-3.1-8b-instant");
            rate_limit_spin_->setValue(30);
        } else if (text == "OpenRouter") {
            endpoint_edit_->setText("https://openrouter.ai/api/v1/chat/completions");
            model_edit_->setText("meta-llama/llama-3.1-8b-instruct:free");
            rate_limit_spin_->setValue(20);
        } else if (text.contains("Ollama")) {
            endpoint_edit_->setText("http://localhost:11434/v1/chat/completions");
            model_edit_->setText("llama3.1");
            rate_limit_spin_->setValue(10000);
            api_key_edit_->clear();
        } else if (text.contains("LM Studio")) {
            endpoint_edit_->setText("http://localhost:1234/v1/chat/completions");
            model_edit_->setText("local-model");
            rate_limit_spin_->setValue(10000);
            api_key_edit_->clear();
        }
        update_cost_estimate();
    });

    main_layout->addWidget(prov_group);

    // ── Mode Group ──
    auto* mode_group = new QGroupBox("Sorting Mode");
    mode_group->setStyleSheet("QGroupBox { font-weight: bold; }");
    auto* mode_layout = new QVBoxLayout(mode_group);

    auto_radio_ = new QRadioButton("Auto \xe2\x80\x94 AI sorts all files, then review");
    auto_radio_->setChecked(true);
    auto_radio_->setToolTip("The AI assigns every file to a folder. You review and adjust on the review screen.");
    mode_layout->addWidget(auto_radio_);

    auto* semi_row = new QHBoxLayout();
    semi_radio_ = new QRadioButton("Semi \xe2\x80\x94 AI suggests");
    semi_radio_->setToolTip("The AI highlights the top N matching folders per file in the grid.");
    semi_row->addWidget(semi_radio_);
    semi_count_spin_ = new QSpinBox();
    semi_count_spin_->setRange(2, 5);
    semi_count_spin_->setValue(3);
    semi_count_spin_->setEnabled(false);
    semi_row->addWidget(semi_count_spin_);
    semi_row->addWidget(new QLabel("folders per file"));
    semi_row->addStretch();
    mode_layout->addLayout(semi_row);

    connect(semi_radio_, &QRadioButton::toggled, semi_count_spin_, &QSpinBox::setEnabled);

    main_layout->addWidget(mode_group);

    // ── Categories Group ──
    auto* cat_group = new QGroupBox("Category Handling");
    cat_group->setStyleSheet("QGroupBox { font-weight: bold; }");
    auto* cat_layout = new QVBoxLayout(cat_group);

    category_combo_ = new QComboBox();
    category_combo_->addItem("Keep existing categories", static_cast<int>(AiCategoryMode::KeepExisting));
    category_combo_->addItem("Generate new categories", static_cast<int>(AiCategoryMode::GenerateNew));
    category_combo_->addItem("Synthesize new categories (existing + AI)", static_cast<int>(AiCategoryMode::SynthesizeNew));
    category_combo_->addItem("Keep + Generate new categories", static_cast<int>(AiCategoryMode::KeepPlusGenerate));
    cat_layout->addWidget(category_combo_);

    auto* cat_help = new QLabel(
        "<b>Keep existing:</b> AI sorts into your current folders<br>"
        "<b>Generate new:</b> AI creates entirely new category folders<br>"
        "<b>Synthesize:</b> AI merges your folder intent with new analysis<br>"
        "<b>Keep + Generate:</b> Keeps current folders and adds new ones");
    cat_help->setStyleSheet("color: #999; font-size: 10px;");
    cat_help->setWordWrap(true);
    cat_layout->addWidget(cat_help);

    main_layout->addWidget(cat_group);

    // ── Depth Group ──
    auto* depth_group = new QGroupBox("Category Depth");
    depth_group->setStyleSheet("QGroupBox { font-weight: bold; }");
    auto* depth_layout = new QHBoxLayout(depth_group);

    depth_layout->addWidget(new QLabel("Subcategory depth:"));
    depth_spin_ = new QSpinBox();
    depth_spin_->setRange(1, 3);
    depth_spin_->setValue(2);
    depth_spin_->setToolTip("1 = flat (Images/)\n2 = one sub-level (Images/Vacation/)\n3 = two sub-levels (Images/Vacation/Beach/)");
    depth_layout->addWidget(depth_spin_);
    depth_layout->addWidget(new QLabel("levels (1\xe2\x80\x93" "3)"));
    depth_layout->addStretch();

    main_layout->addWidget(depth_group);

    // ── Purpose (optional) ──
    auto* purpose_group = new QGroupBox("What is this folder for? (optional)");
    purpose_group->setStyleSheet("QGroupBox { font-weight: bold; }");
    auto* purpose_layout = new QVBoxLayout(purpose_group);

    purpose_edit_ = new QTextEdit();
    purpose_edit_->setMaximumHeight(55);
    purpose_edit_->setPlaceholderText("e.g. 'This is my Downloads folder, I want to organize by project and file type'");
    purpose_edit_->setStyleSheet("QTextEdit { background: #2d2d2d; color: #ecf0f1; border: 1px solid #4a6078; }");
    purpose_layout->addWidget(purpose_edit_);

    main_layout->addWidget(purpose_group);

    // ── Cost estimate ──
    cost_label_ = new QLabel("");
    cost_label_->setStyleSheet("color: #f39c12; font-size: 11px;");
    cost_label_->setWordWrap(true);
    main_layout->addWidget(cost_label_);
    update_cost_estimate();

    main_layout->addStretch();

    // ── Buttons ──
    auto* btn_layout = new QHBoxLayout();
    btn_layout->addStretch();

    auto* cancel_btn = new QPushButton("Cancel");
    cancel_btn->setStyleSheet("QPushButton { padding: 8px 20px; }");
    connect(cancel_btn, &QPushButton::clicked, this, &QDialog::reject);
    btn_layout->addWidget(cancel_btn);

    auto* ok_btn = new QPushButton("OK \xe2\x80\x94 Start Sorting");
    ok_btn->setStyleSheet(
        "QPushButton { padding: 8px 25px; background-color: #3498db; "
        "color: white; font-weight: bold; border-radius: 4px; }"
        "QPushButton:hover { background-color: #2980b9; }"
    );
    connect(ok_btn, &QPushButton::clicked, this, [this]() {
        QString provider = provider_combo_->currentText();
        bool is_local = provider.contains("Ollama") || provider.contains("LM Studio")
                        || provider.contains("Local");
        if (!is_local && api_key_edit_->text().trimmed().isEmpty()) {
            QMessageBox::warning(this, "API Key Required",
                "Please enter an API key for the selected provider.\n"
                "For local LLMs (Ollama, LM Studio), the key can be left empty.");
            return;
        }
        if (endpoint_edit_->text().trimmed().isEmpty()) {
            QMessageBox::warning(this, "Endpoint Required",
                "Please enter the API endpoint URL.");
            return;
        }
        save_provider_config();
        accept();
    });
    btn_layout->addWidget(ok_btn);

    main_layout->addLayout(btn_layout);
}

void AiSetupDialog::load_saved_provider() {
    QStringList names = db_.get_ai_provider_names();
    if (!names.isEmpty()) {
        QString name = names.first();
        int idx = provider_combo_->findText(name);
        if (idx >= 0) provider_combo_->setCurrentIndex(idx);
        else provider_combo_->setCurrentText(name);
        emit provider_combo_->currentTextChanged(name);
    }
}

void AiSetupDialog::save_provider_config() {
    QString provider = provider_combo_->currentText();
    bool is_local = provider.contains("Ollama") || provider.contains("LM Studio")
                    || provider.contains("Local");
    db_.save_ai_provider(provider, api_key_edit_->text().trimmed(),
                         endpoint_edit_->text().trimmed(),
                         model_edit_->text().trimmed(),
                         is_local, rate_limit_spin_->value());
}

void AiSetupDialog::update_cost_estimate() {
    QString provider = provider_combo_->currentText();
    bool is_local = provider.contains("Ollama") || provider.contains("LM Studio")
                    || provider.contains("Local");
    if (is_local || provider.contains("free") || provider.contains("Free")) {
        cost_label_->setText("");
        return;
    }
    // Rough cost estimate based on GPT-4o-mini pricing (~$0.15/1M input, ~$0.60/1M output)
    // Actual costs vary by provider and model; this is an approximate indicator
    int batches = (file_count_ + kBatchSize - 1) / kBatchSize;
    double input_tokens = file_count_ * 200.0 + batches * 500.0;
    double output_tokens = file_count_ * 80.0;
    double cost = (input_tokens * 0.15 + output_tokens * 0.60) / 1000000.0;
    cost_label_->setText(QString("\xe2\x9a\xa0\xef\xb8\x8f Cost estimate: ~$%1 for %2 files (%3)")
        .arg(cost, 0, 'f', 3).arg(file_count_).arg(model_edit_->text()));
}

AiSortMode AiSetupDialog::sort_mode() const {
    return auto_radio_->isChecked() ? AiSortMode::Auto : AiSortMode::Semi;
}

int AiSetupDialog::semi_mode_count() const {
    return semi_count_spin_->value();
}

AiCategoryMode AiSetupDialog::category_mode() const {
    return static_cast<AiCategoryMode>(category_combo_->currentData().toInt());
}

int AiSetupDialog::category_depth() const {
    return depth_spin_->value();
}

QString AiSetupDialog::folder_purpose() const {
    return purpose_edit_->toPlainText().trimmed();
}

AiProviderConfig AiSetupDialog::provider_config() const {
    AiProviderConfig cfg;
    cfg.provider_name = provider_combo_->currentText();
    cfg.api_key = api_key_edit_->text().trimmed();
    cfg.endpoint_url = endpoint_edit_->text().trimmed();
    cfg.model_name = model_edit_->text().trimmed();
    cfg.is_local = cfg.provider_name.contains("Ollama") || cfg.provider_name.contains("LM Studio")
                   || cfg.provider_name.contains("Local");
    cfg.rate_limit_rpm = rate_limit_spin_->value();
    return cfg;
}

// ============================================================
// AiFileTinderDialog
// ============================================================

AiFileTinderDialog::AiFileTinderDialog(const QString& source_folder,
                                       DatabaseManager& db,
                                       QWidget* parent)
    : AdvancedFileTinderDialog(source_folder, db, parent)
    , sort_mode_(AiSortMode::Auto)
    , category_mode_(AiCategoryMode::KeepExisting)
    , semi_count_(3)
    , category_depth_(2)
    , network_manager_(new QNetworkAccessManager(this))
    , rate_limit_timer_(new QTimer(this))
    , requests_this_minute_(0) {

    setWindowTitle(QString("File Tinder - AI Mode \xe2\x80\x94 %1").arg(QFileInfo(source_folder).fileName()));

    rate_limit_timer_->setInterval(60000);
    connect(rate_limit_timer_, &QTimer::timeout, this, &AiFileTinderDialog::reset_rate_limit);
}

AiFileTinderDialog::~AiFileTinderDialog() = default;

void AiFileTinderDialog::initialize() {
    AdvancedFileTinderDialog::initialize();

    // Replace the "Basic Mode" switch button with a mode menu for AI mode
    if (switch_mode_btn_) {
        switch_mode_btn_->disconnect();
        switch_mode_btn_->setText("Switch Mode");
        connect(switch_mode_btn_, &QPushButton::clicked, this, [this]() {
            QMenu menu(this);
            auto* basic_action = menu.addAction("Basic Mode");
            auto* adv_action = menu.addAction("Advanced Mode");
            QAction* selected = menu.exec(switch_mode_btn_->mapToGlobal(
                QPoint(0, switch_mode_btn_->height())));
            if (selected == basic_action) {
                emit switch_to_basic_mode();
            } else if (selected == adv_action) {
                emit switch_to_advanced_mode();
            }
        });
    }

    // Add "Re-run AI" button to the title bar
    if (auto* main_layout = qobject_cast<QVBoxLayout*>(layout())) {
        rerun_ai_btn_ = new QPushButton("\xF0\x9F\xA4\x96 Re-run AI");
        rerun_ai_btn_->setStyleSheet(
            "QPushButton { padding: 5px 12px; background-color: #3498db; "
            "border-radius: 4px; color: white; font-size: 11px; }"
            "QPushButton:hover { background-color: #2980b9; }"
        );
        rerun_ai_btn_->setToolTip("Re-run AI analysis on remaining unsorted files or all files");
        connect(rerun_ai_btn_, &QPushButton::clicked, this, [this]() {
            auto reply = QMessageBox::question(this, "Re-run AI",
                "Re-analyze which files?\n\n"
                "\xe2\x80\xa2 Yes \xe2\x80\x94 Remaining unsorted files only\n"
                "\xe2\x80\xa2 No \xe2\x80\x94 All files (overwrite existing decisions)",
                QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel,
                QMessageBox::Yes);
            if (reply == QMessageBox::Cancel) return;
            bool remaining_only = (reply == QMessageBox::Yes);
            run_ai_analysis(remaining_only);
        });

        if (main_layout->count() > 0) {
            auto* first_item = main_layout->itemAt(0);
            if (first_item && first_item->widget()) {
                auto* title_bar = first_item->widget();
                auto* title_layout = qobject_cast<QHBoxLayout*>(title_bar->layout());
                if (title_layout) {
                    title_layout->insertWidget(title_layout->count() - 1, rerun_ai_btn_);
                }
            }
        }
    }

    if (!show_ai_setup()) {
        QTimer::singleShot(0, this, [this]() { QDialog::reject(); });
        return;
    }

    run_ai_analysis(false);
}

bool AiFileTinderDialog::show_ai_setup() {
    QStringList existing_folders;
    if (folder_model_) {
        existing_folders = folder_model_->get_all_folder_paths();
    }

    AiSetupDialog setup(existing_folders, static_cast<int>(files_.size()),
                        db_, source_folder_, this);
    if (setup.exec() != QDialog::Accepted) {
        return false;
    }

    sort_mode_ = setup.sort_mode();
    semi_count_ = setup.semi_mode_count();
    category_mode_ = setup.category_mode();
    category_depth_ = setup.category_depth();
    folder_purpose_ = setup.folder_purpose();
    provider_config_ = setup.provider_config();

    rate_limit_timer_->start();
    return true;
}

void AiFileTinderDialog::run_ai_analysis(bool remaining_only) {
    if (files_.empty()) {
        QMessageBox::information(this, "No Files", "No files to analyze.");
        return;
    }

    // Determine which files to analyze
    std::vector<int> file_indices;
    for (int i = 0; i < static_cast<int>(files_.size()); ++i) {
        if (!remaining_only || files_[i].decision == "pending") {
            file_indices.push_back(i);
        }
    }
    if (file_indices.empty()) {
        QMessageBox::information(this, "No Files", "No unsorted files remaining.");
        return;
    }

    // Build file descriptions
    QStringList file_descriptions;
    for (int idx : file_indices) {
        const auto& f = files_[idx];
        file_descriptions.append(QString("%1|%2|%3|%4|%5")
            .arg(idx).arg(f.name).arg(f.extension).arg(f.size).arg(f.mime_type));
    }

    // Build available folders list
    QStringList available_folders;
    if (folder_model_) {
        available_folders = folder_model_->get_all_folder_paths();
    }
    if (!available_folders.contains(source_folder_)) {
        available_folders.prepend(source_folder_);
    }

    // Calculate batches
    int total_files = static_cast<int>(file_descriptions.size());
    int total_batches = (total_files + kBatchSize - 1) / kBatchSize;

    // Progress dialog with real-time log
    QDialog progress_dialog(this);
    progress_dialog.setWindowTitle("AI Analysis");
    progress_dialog.setMinimumSize(ui::scaling::scaled(550), ui::scaling::scaled(400));
    auto* prog_layout = new QVBoxLayout(&progress_dialog);

    auto* prog_header = new QLabel(QString("Analyzing %1 files...").arg(total_files));
    prog_header->setStyleSheet("font-size: 14px; font-weight: bold; color: #3498db;");
    prog_layout->addWidget(prog_header);

    auto* prog_bar = new QProgressBar();
    prog_bar->setRange(0, total_batches);
    prog_bar->setValue(0);
    prog_layout->addWidget(prog_bar);

    auto* log_browser = new QTextBrowser();
    log_browser->setStyleSheet("QTextBrowser { background: #1a1a2e; color: #e0e0e0; font-family: monospace; font-size: 11px; }");
    log_browser->setReadOnly(true);
    prog_layout->addWidget(log_browser, 1);

    auto* cancel_btn = new QPushButton("Cancel");
    cancel_btn->setStyleSheet("QPushButton { padding: 6px 16px; }");
    prog_layout->addWidget(cancel_btn, 0, Qt::AlignRight);

    bool cancelled = false;
    connect(cancel_btn, &QPushButton::clicked, this, [&cancelled]() { cancelled = true; });

    progress_dialog.show();
    QApplication::processEvents();

    auto log = [&](const QString& msg) {
        QString ts = QDateTime::currentDateTime().toString("hh:mm:ss");
        log_browser->append(QString("[%1] %2").arg(ts, msg));
        log_browser->verticalScrollBar()->setValue(log_browser->verticalScrollBar()->maximum());
        QApplication::processEvents();
    };

    log(QString("Starting AI analysis of %1 files...").arg(total_files));
    log(QString("Provider: %1 (%2) | Rate: %3 req/min")
        .arg(provider_config_.provider_name, provider_config_.model_name)
        .arg(provider_config_.rate_limit_rpm));

    QString cat_mode_name;
    switch (category_mode_) {
        case AiCategoryMode::KeepExisting: cat_mode_name = "Keep existing"; break;
        case AiCategoryMode::GenerateNew: cat_mode_name = "Generate new"; break;
        case AiCategoryMode::SynthesizeNew: cat_mode_name = "Synthesize new"; break;
        case AiCategoryMode::KeepPlusGenerate: cat_mode_name = "Keep + Generate"; break;
    }
    log(QString("Category mode: %1 | Depth: %2").arg(cat_mode_name).arg(category_depth_));

    // Clear previous suggestions for files we're re-analyzing
    if (remaining_only) {
        // Keep suggestions for already-sorted files
        std::vector<AiFileSuggestion> kept;
        for (const auto& s : suggestions_) {
            if (s.file_index >= 0 && s.file_index < static_cast<int>(files_.size())
                && files_[s.file_index].decision != "pending") {
                kept.push_back(s);
            }
        }
        suggestions_ = kept;
    } else {
        suggestions_.clear();
    }

    QElapsedTimer elapsed;
    elapsed.start();
    int files_classified = 0;

    for (int batch = 0; batch < total_batches && !cancelled; ++batch) {
        int start = batch * kBatchSize;
        int end = qMin(start + kBatchSize, total_files);
        int batch_size = end - start;

        log(QString("Batch %1/%2 \xe2\x80\x94 analyzing files %3-%4...")
            .arg(batch + 1).arg(total_batches).arg(start + 1).arg(end));

        // Build batch file list
        QStringList batch_files;
        QMap<int, int> batch_index_map; // maps original file index to position in batch
        for (int i = start; i < end; ++i) {
            batch_files.append(file_descriptions[i]);
            // Parse original file index from the description string
            int orig_idx = file_descriptions[i].split('|').first().toInt();
            batch_index_map[orig_idx] = i - start;
        }

        // Check rate limit
        if (!check_rate_limit()) {
            int wait_secs = 60 - (elapsed.elapsed() / 1000) % 60;
            if (wait_secs <= 0) wait_secs = 5;
            log(QString("\xe2\x9a\xa0\xef\xb8\x8f Rate limit: waiting %1s before next request...").arg(wait_secs));
            QEventLoop wait_loop;
            QTimer::singleShot(wait_secs * 1000, &wait_loop, &QEventLoop::quit);
            wait_loop.exec();
            reset_rate_limit();
        }

        // Build prompt for this batch
        QString prompt = build_analysis_prompt(batch_files, available_folders);

        // Send request
        QElapsedTimer batch_timer;
        batch_timer.start();

        QString error;
        QString response = send_api_request(prompt, error);

        if (!error.isEmpty()) {
            log(QString("\xe2\x9d\x8c Batch %1/%2 failed: %3").arg(batch + 1).arg(total_batches).arg(error));

            // Error recovery: let user choose
            progress_dialog.hide();
            auto reply = QMessageBox::question(this, "AI Analysis Interrupted",
                QString("AI analysis interrupted after %1/%2 files.\n\n"
                        "%3 files remain unclassified.\n\n"
                        "What would you like to do?")
                    .arg(files_classified).arg(total_files).arg(total_files - files_classified),
                QMessageBox::Abort | QMessageBox::Ignore,
                QMessageBox::Ignore);

            if (reply == QMessageBox::Abort) {
                suggestions_.clear();
                return;
            }
            // Continue to review with partial results
            break;
        }

        double batch_secs = batch_timer.elapsed() / 1000.0;

        // Parse response
        auto batch_suggestions = parse_ai_response(response, batch_index_map);
        int parsed_count = static_cast<int>(batch_suggestions.size());
        files_classified += parsed_count;

        // If partial parse failure, retry once
        if (parsed_count < batch_size) {
            int failed = batch_size - parsed_count;
            log(QString("\xe2\x9a\xa0\xef\xb8\x8f %1/%2 files parsed. Retrying %3 failed...")
                .arg(parsed_count).arg(batch_size).arg(failed));

            // Build a retry prompt with just the failed file indices
            QSet<int> parsed_indices;
            for (const auto& s : batch_suggestions) parsed_indices.insert(s.file_index);

            QStringList retry_files;
            QMap<int, int> retry_map;
            int retry_pos = 0;
            for (int i = start; i < end; ++i) {
                int orig_idx = file_descriptions[i].split('|').first().toInt();
                if (!parsed_indices.contains(orig_idx)) {
                    retry_files.append(file_descriptions[i]);
                    retry_map[orig_idx] = retry_pos++;
                }
            }

            if (!retry_files.isEmpty()) {
                QString retry_prompt = build_analysis_prompt(retry_files, available_folders);
                retry_prompt += "\nIMPORTANT: Return ONLY valid JSON. No extra text.\n";
                QString retry_error;
                QString retry_response = send_api_request(retry_prompt, retry_error);
                if (retry_error.isEmpty()) {
                    auto retry_results = parse_ai_response(retry_response, retry_map);
                    for (auto& s : retry_results) {
                        batch_suggestions.push_back(s);
                    }
                    files_classified += static_cast<int>(retry_results.size());
                    log(QString("  Retry recovered %1 more files").arg(retry_results.size()));
                }
            }
        }

        // Store suggestions
        for (auto& s : batch_suggestions) {
            suggestions_.push_back(s);
        }

        log(QString("Batch %1/%2 complete \xe2\x80\x94 %3 files classified (%4s)")
            .arg(batch + 1).arg(total_batches).arg(files_classified)
            .arg(batch_secs, 0, 'f', 1));

        prog_bar->setValue(batch + 1);
        QApplication::processEvents();
    }

    double total_secs = elapsed.elapsed() / 1000.0;
    log(QString("\xe2\x9c\x85 Analysis complete \xe2\x80\x94 %1 files classified (%2s)")
        .arg(files_classified).arg(total_secs, 0, 'f', 1));

    // Handle new categories: add AI-generated folders to the tree
    if (category_mode_ != AiCategoryMode::KeepExisting) {
        QSet<QString> new_folders;
        for (const auto& s : suggestions_) {
            for (const QString& folder : s.suggested_folders) {
                if (folder_model_ && !folder_model_->find_node(folder)
                    && folder != source_folder_) {
                    new_folders.insert(folder);
                }
            }
        }

        // Cap new folders
        if (new_folders.size() > kMaxAiGeneratedFolders) {
            log(QString("\xe2\x9a\xa0\xef\xb8\x8f AI suggested %1 new folders (capped at %2)")
                .arg(new_folders.size()).arg(kMaxAiGeneratedFolders));
            QStringList sorted_list = new_folders.values();
            sorted_list.sort();
            new_folders.clear();
            for (int i = 0; i < kMaxAiGeneratedFolders && i < sorted_list.size(); ++i) {
                new_folders.insert(sorted_list[i]);
            }
        }

        if (folder_model_ && !new_folders.isEmpty()) {
            folder_model_->blockSignals(true);
            for (const QString& folder : new_folders) {
                bool is_virtual = !QDir(folder).exists();
                folder_model_->add_folder(folder, is_virtual);
            }
            folder_model_->blockSignals(false);
            if (mind_map_view_) mind_map_view_->refresh_layout();
            log(QString("Added %1 new folder(s) to the grid").arg(new_folders.size()));
        }
    }

    // Wait a moment so user can see the completion message
    QTimer::singleShot(1500, &progress_dialog, &QDialog::accept);
    progress_dialog.exec();

    // Apply results based on mode
    if (sort_mode_ == AiSortMode::Auto) {
        apply_auto_suggestions();
    } else {
        apply_semi_suggestions();
    }
}

QString AiFileTinderDialog::build_analysis_prompt(const QStringList& file_descriptions,
                                                   const QStringList& available_folders) {
    QString prompt;
    int suggestion_count = (sort_mode_ == AiSortMode::Semi) ? semi_count_ : 1;

    prompt += "You are a file organization assistant with expertise in taxonomy and categorization. ";
    prompt += "Analyze each file carefully \xe2\x80\x94 investigate its name, extension, size, and MIME type ";
    prompt += "to determine precisely what it is and where it belongs. ";
    prompt += "Don't just sort by extension \xe2\x80\x94 understand what the file actually IS. ";
    prompt += "For example, 'Serum_x64.dll' (47MB) is not just a DLL \xe2\x80\x94 it's a VST synthesizer plugin (generator type). ";
    prompt += "'receipt_2024_03.pdf' is not just a PDF \xe2\x80\x94 it's a financial receipt.\n\n";

    switch (category_mode_) {
        case AiCategoryMode::KeepExisting:
            prompt += "IMPORTANT: Use ONLY the existing folders listed below. Do NOT create new folders.\n";
            break;
        case AiCategoryMode::GenerateNew:
            prompt += "IGNORE the existing folders. Create entirely new category folders under the source folder. ";
            prompt += QString("Maximum subcategory depth: %1 levels. ").arg(category_depth_);
            prompt += QString("Maximum %1 total new folders.\n").arg(kMaxAiGeneratedFolders);
            break;
        case AiCategoryMode::SynthesizeNew:
            prompt += "Look at the existing folders to understand the user's organizational intent, ";
            prompt += "then create improved categories that blend that intent with fresh analysis. ";
            prompt += QString("Maximum subcategory depth: %1 levels. ").arg(category_depth_);
            prompt += QString("Maximum %1 total new folders.\n").arg(kMaxAiGeneratedFolders);
            break;
        case AiCategoryMode::KeepPlusGenerate:
            prompt += "Keep ALL existing folders AND add new ones as needed. ";
            prompt += QString("Maximum subcategory depth: %1 levels. ").arg(category_depth_);
            prompt += QString("Maximum %1 total new folders.\n").arg(kMaxAiGeneratedFolders);
            break;
    }

    if (!folder_purpose_.isEmpty()) {
        prompt += QString("\nFolder purpose (from user): \"%1\"\n").arg(folder_purpose_);
    }

    prompt += "\nSource folder (root): " + source_folder_ + "\n";

    if (!available_folders.isEmpty() && category_mode_ != AiCategoryMode::GenerateNew) {
        prompt += "\nExisting folders:\n";
        for (const QString& f : available_folders) {
            prompt += "  - " + f + "\n";
        }
    }

    prompt += QString("\nFor each file, suggest the top %1 best-matching folder(s), ordered by confidence.\n")
        .arg(suggestion_count);
    prompt += "New folder paths MUST be under the source folder root.\n\n";

    prompt += "Files (format: index|name|extension|size_bytes|mime_type):\n";
    for (const QString& desc : file_descriptions) {
        prompt += desc + "\n";
    }

    prompt += "\nRespond with ONLY a JSON array. Each element:\n";
    prompt += "  {\"i\": <file_index>, \"f\": [\"<full_folder_path>\", ...], \"r\": \"<short_reasoning>\"}\n";
    prompt += "Example: [{\"i\":0,\"f\":[\"" + source_folder_ + "/Images\"],\"r\":\"JPEG photo\"}]\n";
    prompt += "Return ONLY the JSON array, no markdown, no explanation.\n";

    return prompt;
}

QString AiFileTinderDialog::send_api_request(const QString& prompt, QString& error_out) {
    QUrl url(provider_config_.endpoint_url);
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    int timeout = provider_config_.is_local ? kLocalTimeoutMs : kCloudTimeoutMs;
    request.setTransferTimeout(timeout);

    // Set auth headers
    if (!provider_config_.api_key.isEmpty()) {
        if (provider_config_.provider_name == "Anthropic") {
            request.setRawHeader("x-api-key", provider_config_.api_key.toUtf8());
            request.setRawHeader("anthropic-version", "2023-06-01");
        } else if (provider_config_.provider_name != "Google Gemini") {
            request.setRawHeader("Authorization",
                QByteArray("Bearer ") + provider_config_.api_key.toUtf8());
        }
    }

    // Build request body
    QJsonObject body;

    if (provider_config_.provider_name == "Anthropic") {
        body["model"] = provider_config_.model_name;
        body["max_tokens"] = 4096;
        QJsonArray messages;
        QJsonObject msg;
        msg["role"] = "user";
        msg["content"] = prompt;
        messages.append(msg);
        body["messages"] = messages;
    } else if (provider_config_.provider_name == "Google Gemini") {
        // Gemini uses different URL/body structure
        QString gemini_url = QString("%1/%2:generateContent?key=%3")
            .arg(provider_config_.endpoint_url,
                 provider_config_.model_name,
                 provider_config_.api_key);
        url = QUrl(gemini_url);
        request.setUrl(url);
        request.setRawHeader("Authorization", QByteArray());

        QJsonObject contents;
        QJsonArray parts;
        QJsonObject part;
        part["text"] = prompt;
        parts.append(part);
        contents["parts"] = parts;
        QJsonArray contents_arr;
        contents_arr.append(contents);
        body["contents"] = contents_arr;
    } else {
        // OpenAI-compatible (OpenAI, Groq, OpenRouter, Mistral, Ollama, LM Studio)
        body["model"] = provider_config_.model_name;
        body["temperature"] = 0.3;
        body["max_tokens"] = 4096;
        QJsonArray messages;
        QJsonObject sys_msg;
        sys_msg["role"] = "system";
        sys_msg["content"] = QString("You are a file organization assistant. Respond only with the exact JSON array requested. No markdown formatting.");
        messages.append(sys_msg);
        QJsonObject user_msg;
        user_msg["role"] = "user";
        user_msg["content"] = prompt;
        messages.append(user_msg);
        body["messages"] = messages;
    }

    requests_this_minute_++;
    QNetworkReply* reply = network_manager_->post(request, QJsonDocument(body).toJson());

    QEventLoop loop;
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    if (reply->error() != QNetworkReply::NoError) {
        error_out = reply->errorString();
        QString resp_body = reply->readAll();
        if (!resp_body.isEmpty()) {
            error_out += " \xe2\x80\x94 " + resp_body.left(300);
        }
        reply->deleteLater();
        return {};
    }

    QByteArray response_data = reply->readAll();
    reply->deleteLater();

    QJsonDocument doc = QJsonDocument::fromJson(response_data);
    QString content_text;

    if (provider_config_.provider_name == "Anthropic") {
        QJsonArray content_arr = doc.object()["content"].toArray();
        if (!content_arr.isEmpty()) {
            content_text = content_arr[0].toObject()["text"].toString();
        }
    } else if (provider_config_.provider_name == "Google Gemini") {
        QJsonArray candidates = doc.object()["candidates"].toArray();
        if (!candidates.isEmpty()) {
            QJsonObject content = candidates[0].toObject()["content"].toObject();
            QJsonArray parts = content["parts"].toArray();
            if (!parts.isEmpty()) {
                content_text = parts[0].toObject()["text"].toString();
            }
        }
    } else {
        QJsonArray choices = doc.object()["choices"].toArray();
        if (!choices.isEmpty()) {
            content_text = choices[0].toObject()["message"].toObject()["content"].toString();
        }
    }

    if (content_text.isEmpty()) {
        error_out = "Empty response from AI";
        return {};
    }

    return content_text;
}

std::vector<AiFileSuggestion> AiFileTinderDialog::parse_ai_response(
    const QString& response, const QMap<int, int>& batch_index_map) {

    std::vector<AiFileSuggestion> results;

    // Level 1: Try full JSON parse
    QJsonDocument doc = QJsonDocument::fromJson(response.toUtf8());
    if (doc.isArray()) {
        QJsonArray arr = doc.array();
        for (const QJsonValue& val : arr) {
            QJsonObject obj = val.toObject();
            AiFileSuggestion s;
            s.file_index = obj["i"].toInt(-1);
            if (s.file_index < 0 || s.file_index >= static_cast<int>(files_.size())) continue;

            QJsonArray folders = obj["f"].toArray();
            for (const QJsonValue& fv : folders) {
                QString folder = fv.toString();
                if (!folder.isEmpty()) s.suggested_folders.append(folder);
            }
            s.reasoning = obj["r"].toString();

            if (!s.suggested_folders.isEmpty()) results.push_back(s);
        }
        if (!results.empty()) return results;
    }

    // Level 2: Extract JSON array from surrounding text
    int start = response.indexOf('[');
    int end = response.lastIndexOf(']');
    if (start >= 0 && end > start) {
        QString json_str = response.mid(start, end - start + 1);
        doc = QJsonDocument::fromJson(json_str.toUtf8());
        if (doc.isArray()) {
            QJsonArray arr = doc.array();
            for (const QJsonValue& val : arr) {
                QJsonObject obj = val.toObject();
                AiFileSuggestion s;
                s.file_index = obj["i"].toInt(-1);
                if (s.file_index < 0 || s.file_index >= static_cast<int>(files_.size())) continue;

                QJsonArray folders = obj["f"].toArray();
                for (const QJsonValue& fv : folders) {
                    QString folder = fv.toString();
                    if (!folder.isEmpty()) s.suggested_folders.append(folder);
                }
                s.reasoning = obj["r"].toString();

                if (!s.suggested_folders.isEmpty()) results.push_back(s);
            }
            if (!results.empty()) return results;
        }
    }

    // Level 3: Line-by-line JSON object parsing
    QStringList lines = response.split('\n');
    for (const QString& line : lines) {
        QString trimmed = line.trimmed();
        if (!trimmed.startsWith('{')) continue;
        if (trimmed.endsWith(',')) trimmed.chop(1);

        QJsonDocument line_doc = QJsonDocument::fromJson(trimmed.toUtf8());
        if (!line_doc.isObject()) continue;

        QJsonObject obj = line_doc.object();
        AiFileSuggestion s;
        s.file_index = obj["i"].toInt(-1);
        if (s.file_index < 0 || s.file_index >= static_cast<int>(files_.size())) continue;

        QJsonArray folders = obj["f"].toArray();
        for (const QJsonValue& fv : folders) {
            QString folder = fv.toString();
            if (!folder.isEmpty()) s.suggested_folders.append(folder);
        }
        s.reasoning = obj["r"].toString();

        if (!s.suggested_folders.isEmpty()) results.push_back(s);
    }

    if (!results.empty()) {
        LOG_INFO("AIMode", QString("Level 3 parse recovered %1 suggestions").arg(results.size()));
    } else {
        LOG_ERROR("AIMode", QString("All parse levels failed for response: %1").arg(response.left(500)));
    }

    return results;
}

void AiFileTinderDialog::apply_auto_suggestions() {
    for (const auto& s : suggestions_) {
        if (s.file_index < 0 || s.file_index >= static_cast<int>(files_.size())) continue;

        auto& file = files_[s.file_index];
        if (file.decision != "pending") continue;
        if (s.suggested_folders.isEmpty()) continue;

        QString dest = s.suggested_folders.first();

        if (dest == source_folder_) {
            file.decision = "keep";
            keep_count_++;
        } else {
            file.decision = "move";
            file.destination_folder = dest;
            move_count_++;
            if (folder_model_) folder_model_->assign_file_to_folder(dest);
        }
    }

    save_session_state();
    update_stats();
    LOG_INFO("AIMode", "Auto mode complete \xe2\x80\x94 showing review screen");

    show_review_summary();
}

void AiFileTinderDialog::apply_semi_suggestions() {
    LOG_INFO("AIMode", QString("Semi mode \xe2\x80\x94 %1 suggestions ready").arg(suggestions_.size()));
    current_filtered_index_ = 0;
    show_current_file();
}

void AiFileTinderDialog::show_current_file() {
    clear_folder_highlights();
    AdvancedFileTinderDialog::show_current_file();

    // In semi mode, highlight AI-suggested folders
    if (sort_mode_ == AiSortMode::Semi && !suggestions_.empty()) {
        int file_idx = get_current_file_index();
        if (file_idx >= 0) {
            for (const auto& s : suggestions_) {
                if (s.file_index == file_idx) {
                    highlight_suggested_folders(s.suggested_folders);
                    break;
                }
            }
        }
    }
}

void AiFileTinderDialog::highlight_suggested_folders(const QStringList& folders) {
    highlighted_folders_ = folders;

    // Highlight best match in the mind map
    if (mind_map_view_ && !folders.isEmpty()) {
        mind_map_view_->set_selected_folder(folders.first());
    }

    // Add AI suggestions to quick access temporarily
    if (quick_access_list_) {
        for (int i = 0; i < folders.size(); ++i) {
            QString path = folders[i];
            QString name = QFileInfo(path).fileName();
            if (name.length() > 12) name = name.left(11) + "\xe2\x80\xa6";
            QString label = QString("AI %1: %2").arg(i + 1).arg(name);

            auto* item = new QListWidgetItem(label);
            item->setData(Qt::UserRole, path);
            item->setToolTip(QString("AI suggestion: %1").arg(path));
            item->setSizeHint(QSize(ui::scaling::scaled(120), ui::scaling::scaled(28)));
            item->setBackground(QColor("#1a3a5c"));
            item->setForeground(QColor("#3498db"));
            quick_access_list_->insertItem(0, item);
        }
    }
}

void AiFileTinderDialog::clear_folder_highlights() {
    highlighted_folders_.clear();

    if (quick_access_list_) {
        for (int i = quick_access_list_->count() - 1; i >= 0; --i) {
            auto* item = quick_access_list_->item(i);
            if (item && item->text().startsWith("AI ")) {
                delete quick_access_list_->takeItem(i);
            }
        }
    }
}

bool AiFileTinderDialog::check_rate_limit() {
    return requests_this_minute_ < provider_config_.rate_limit_rpm;
}

void AiFileTinderDialog::reset_rate_limit() {
    requests_this_minute_ = 0;
}
