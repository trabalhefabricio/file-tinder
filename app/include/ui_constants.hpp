#ifndef UI_CONSTANTS_HPP
#define UI_CONSTANTS_HPP

#include <QApplication>
#include <QScreen>

namespace ui::scaling {
    // Returns scale factor relative to 96 DPI baseline.
    // e.g. 1.0 at 96 DPI, 1.5 at 144 DPI, 2.0 at 192 DPI
    // Cached after first call for performance.
    inline double factor() {
        static double cached = -1.0;
        if (cached < 0.0) {
            if (auto* screen = QApplication::primaryScreen()) {
                cached = screen->logicalDotsPerInch() / 96.0;
            } else {
                cached = 1.0;
            }
        }
        return cached;
    }
    
    // Scale a pixel value by the DPI factor
    inline int scaled(int base_value) {
        return static_cast<int>(base_value * factor());
    }
}

namespace ui::dimensions {
    // Base values at 96 DPI (1x scale)
    // Standalone File Tinder (Basic Mode)
    constexpr int kStandaloneFileTinderMinWidth = 700;
    constexpr int kStandaloneFileTinderMinHeight = 450;
    
    // Advanced File Tinder
    constexpr int kAdvancedFileTinderMinWidth = 800;
    constexpr int kAdvancedFileTinderMinHeight = 600;
    constexpr int kFolderTreePanelMinWidth = 350;
    constexpr int kFolderTreePanelMaxWidth = 600;
    
    // Quick Access Bar
    constexpr int kQuickAccessBarHeight = 50;
    constexpr int kQuickAccessButtonWidth = 100;
    
    // Mind Map Node sizes (larger, clickable, sleek)
    constexpr int kNodeWidth = 250;   // 2x larger as requested
    constexpr int kNodeHeight = 60;   // Larger for easier clicking
    constexpr int kNodeSpacingHorizontal = 50;
    constexpr int kNodeSpacingVertical = 25;
    constexpr int kNodeBorderRadius = 4;
    
    // Add node ("+" button) size
    constexpr int kAddNodeWidth = 60;
    constexpr int kAddNodeHeight = 60;
    
    // File preview
    constexpr int kPreviewMaxWidth = 500;
    constexpr int kPreviewMaxHeight = 400;
    
    // Main action buttons (2x larger for Keep/Delete)
    constexpr int kMainButtonWidth = 200;
    constexpr int kMainButtonHeight = 70;
    constexpr int kSecondaryButtonWidth = 100;
    constexpr int kSecondaryButtonHeight = 50;
    constexpr int kThinButtonHeight = 40;  // For Back/Skip below
}

namespace ui::colors {
    constexpr const char* kNodeDefaultBg = "#ffffff";
    constexpr const char* kNodeSelectedBg = "#e8f4fc";
    constexpr const char* kNodeVirtualBg = "#fff8e1";
    constexpr const char* kNodeConnectedBg = "#e8f4fc";
    constexpr const char* kNodeBorder = "#cccccc";
    constexpr const char* kNodeSelectedBorder = "#0078d4";
    constexpr const char* kConnectionLine = "#0078d4";
    constexpr const char* kDeleteColor = "#e74c3c";
    constexpr const char* kKeepColor = "#2ecc71";
    constexpr const char* kSkipColor = "#f39c12";
    constexpr const char* kMoveColor = "#3498db";
    constexpr const char* kAddNodeBg = "#27ae60";
    constexpr const char* kAddNodeHover = "#2ecc71";
}

namespace ui::icons {
    constexpr const char* kFolderExisting = ":/icons/folder.svg";
    constexpr const char* kFolderVirtual = ":/icons/folder-new.svg";
    constexpr const char* kFolderConnected = ":/icons/folder-linked.svg";
    constexpr const char* kConnectionLine = ":/icons/link.svg";
    constexpr const char* kDelete = ":/icons/delete.svg";
    constexpr const char* kKeep = ":/icons/keep.svg";
    constexpr const char* kSkip = ":/icons/skip.svg";
    constexpr const char* kBack = ":/icons/back.svg";
}

namespace ui::fonts {
    constexpr int kNodeTitleSize = 14;
    constexpr int kNodeSubtitleSize = 10;
    constexpr int kButtonSize = 12;
    constexpr int kHeaderSize = 18;
}

#endif // UI_CONSTANTS_HPP
