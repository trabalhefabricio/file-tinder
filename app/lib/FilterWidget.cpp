// FilterWidget.cpp
// Shared filter and sort component implementation

#include "FilterWidget.hpp"
#include "StandaloneFileTinderDialog.hpp"  // For FileFilterType and SortOrder enums
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QMessageBox>

// ============================================================================
// CustomExtensionDialog
// ============================================================================

CustomExtensionDialog::CustomExtensionDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("Specify Extensions");
    setMinimumSize(300, 250);

    auto* layout = new QVBoxLayout(this);

    // Instructions
    auto* instructions = new QLabel("Enter file extensions (without dot):");
    layout->addWidget(instructions);

    // Input row
    auto* input_row = new QHBoxLayout();
    extension_input_ = new QLineEdit();
    extension_input_->setPlaceholderText("e.g., txt, pdf, docx");
    add_btn_ = new QPushButton("Add");
    input_row->addWidget(extension_input_);
    input_row->addWidget(add_btn_);
    layout->addLayout(input_row);

    // Extension list
    extension_list_ = new QListWidget();
    layout->addWidget(extension_list_);

    // Remove button
    remove_btn_ = new QPushButton("Remove Selected");
    layout->addWidget(remove_btn_);

    // Dialog buttons
    auto* button_row = new QHBoxLayout();
    ok_btn_ = new QPushButton("OK");
    cancel_btn_ = new QPushButton("Cancel");
    button_row->addStretch();
    button_row->addWidget(ok_btn_);
    button_row->addWidget(cancel_btn_);
    layout->addLayout(button_row);

    // Connections
    connect(add_btn_, &QPushButton::clicked, this, [this]() {
        QString ext = extension_input_->text().trimmed().toLower();
        if (!ext.isEmpty()) {
            // Remove leading dot if present
            if (ext.startsWith('.')) {
                ext = ext.mid(1);
            }
            // Check if already exists
            for (int i = 0; i < extension_list_->count(); ++i) {
                if (extension_list_->item(i)->text() == ext) {
                    return;
                }
            }
            extension_list_->addItem(ext);
            extension_input_->clear();
        }
    });

    connect(extension_input_, &QLineEdit::returnPressed, add_btn_, &QPushButton::click);

    connect(remove_btn_, &QPushButton::clicked, this, [this]() {
        auto* item = extension_list_->currentItem();
        if (item) {
            delete extension_list_->takeItem(extension_list_->row(item));
        }
    });

    connect(ok_btn_, &QPushButton::clicked, this, &QDialog::accept);
    connect(cancel_btn_, &QPushButton::clicked, this, &QDialog::reject);
}

QStringList CustomExtensionDialog::get_extensions() const {
    QStringList extensions;
    for (int i = 0; i < extension_list_->count(); ++i) {
        extensions.append(extension_list_->item(i)->text());
    }
    return extensions;
}

void CustomExtensionDialog::set_extensions(const QStringList& extensions) {
    extension_list_->clear();
    for (const auto& ext : extensions) {
        extension_list_->addItem(ext);
    }
}

// ============================================================================
// FilterWidget
// ============================================================================

FilterWidget::FilterWidget(QWidget* parent)
    : QWidget(parent)
    , current_filter_(FileFilterType::All)
    , current_sort_field_(SortField::Name)
    , current_sort_order_(SortOrder::Ascending)
{
    setup_ui();
}

void FilterWidget::setup_ui() {
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);

    // Filter section
    auto* filter_label = new QLabel("Filter:");
    layout->addWidget(filter_label);

    filter_combo_ = new QComboBox();
    filter_combo_->addItem("All", static_cast<int>(FileFilterType::All));
    filter_combo_->addItem("Images", static_cast<int>(FileFilterType::Images));
    filter_combo_->addItem("Videos", static_cast<int>(FileFilterType::Videos));
    filter_combo_->addItem("Audio", static_cast<int>(FileFilterType::Audio));
    filter_combo_->addItem("Documents", static_cast<int>(FileFilterType::Documents));
    filter_combo_->addItem("Archives", static_cast<int>(FileFilterType::Archives));
    filter_combo_->addItem("Folders Only", static_cast<int>(FileFilterType::FoldersOnly));
    filter_combo_->addItem("Specify...", static_cast<int>(FileFilterType::Custom));
    filter_combo_->setMinimumWidth(100);
    layout->addWidget(filter_combo_);

    // Include folders checkbox
    include_folders_check_ = new QCheckBox("Include Folders");
    layout->addWidget(include_folders_check_);

    // Spacer
    layout->addSpacing(16);

    // Sort section
    auto* sort_label = new QLabel("Sort:");
    layout->addWidget(sort_label);

    sort_combo_ = new QComboBox();
    sort_combo_->addItem("Name", static_cast<int>(SortField::Name));
    sort_combo_->addItem("Size", static_cast<int>(SortField::Size));
    sort_combo_->addItem("Type", static_cast<int>(SortField::Type));
    sort_combo_->addItem("Date Modified", static_cast<int>(SortField::DateModified));
    sort_combo_->setMinimumWidth(100);
    layout->addWidget(sort_combo_);

    // Sort order button
    sort_order_btn_ = new QPushButton("Asc");
    sort_order_btn_->setFixedWidth(50);
    sort_order_btn_->setCheckable(true);
    sort_order_btn_->setToolTip("Toggle Ascending/Descending");
    layout->addWidget(sort_order_btn_);

    layout->addStretch();

    // Connections
    connect(filter_combo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &FilterWidget::on_filter_changed);
    connect(sort_combo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &FilterWidget::on_sort_field_changed);
    connect(sort_order_btn_, &QPushButton::toggled,
            this, &FilterWidget::on_sort_order_toggled);
    connect(include_folders_check_, &QCheckBox::toggled,
            this, &FilterWidget::on_include_folders_toggled);
}

void FilterWidget::on_filter_changed(int index) {
    auto type = static_cast<FileFilterType>(filter_combo_->itemData(index).toInt());
    
    if (type == FileFilterType::Custom) {
        on_specify_clicked();
        return;
    }

    current_filter_ = type;
    
    // Disable "include folders" when "Folders Only" is selected
    include_folders_check_->setEnabled(type != FileFilterType::FoldersOnly);
    
    emit filter_changed();
}

void FilterWidget::on_sort_field_changed(int index) {
    current_sort_field_ = static_cast<SortField>(sort_combo_->itemData(index).toInt());
    emit sort_changed();
}

void FilterWidget::on_sort_order_toggled() {
    if (sort_order_btn_->isChecked()) {
        current_sort_order_ = SortOrder::Descending;
        sort_order_btn_->setText("Desc");
    } else {
        current_sort_order_ = SortOrder::Ascending;
        sort_order_btn_->setText("Asc");
    }
    emit sort_changed();
}

void FilterWidget::on_include_folders_toggled(bool checked) {
    emit include_folders_changed(checked);
}

void FilterWidget::on_specify_clicked() {
    CustomExtensionDialog dialog(this);
    dialog.set_extensions(custom_extensions_);
    
    if (dialog.exec() == QDialog::Accepted) {
        custom_extensions_ = dialog.get_extensions();
        if (!custom_extensions_.isEmpty()) {
            current_filter_ = FileFilterType::Custom;
            emit filter_changed();
        } else {
            // If no extensions specified, revert to All
            filter_combo_->setCurrentIndex(0);
        }
    } else {
        // Revert to previous filter
        int prev_index = filter_combo_->findData(static_cast<int>(current_filter_));
        if (prev_index >= 0) {
            filter_combo_->blockSignals(true);
            filter_combo_->setCurrentIndex(prev_index);
            filter_combo_->blockSignals(false);
        }
    }
}

FileFilterType FilterWidget::get_filter_type() const {
    return current_filter_;
}

SortField FilterWidget::get_sort_field() const {
    return current_sort_field_;
}

SortOrder FilterWidget::get_sort_order() const {
    return current_sort_order_;
}

bool FilterWidget::get_include_folders() const {
    return include_folders_check_->isChecked();
}

QStringList FilterWidget::get_custom_extensions() const {
    return custom_extensions_;
}

void FilterWidget::set_filter_type(FileFilterType type) {
    current_filter_ = type;
    int index = filter_combo_->findData(static_cast<int>(type));
    if (index >= 0) {
        filter_combo_->setCurrentIndex(index);
    }
}

void FilterWidget::set_sort_field(SortField field) {
    current_sort_field_ = field;
    int index = sort_combo_->findData(static_cast<int>(field));
    if (index >= 0) {
        sort_combo_->setCurrentIndex(index);
    }
}

void FilterWidget::set_sort_order(SortOrder order) {
    current_sort_order_ = order;
    sort_order_btn_->setChecked(order == SortOrder::Descending);
}

void FilterWidget::set_include_folders(bool include) {
    include_folders_check_->setChecked(include);
}

void FilterWidget::set_custom_extensions(const QStringList& extensions) {
    custom_extensions_ = extensions;
}
