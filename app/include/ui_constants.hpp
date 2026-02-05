#ifndef UI_CONSTANTS_HPP
#define UI_CONSTANTS_HPP

namespace ui::dimensions {
    // Standalone File Tinder (Basic Mode)
    constexpr int kStandaloneFileTinderMinWidth = 900;
    constexpr int kStandaloneFileTinderMinHeight = 700;
    
    // Advanced File Tinder
    constexpr int kAdvancedFileTinderMinWidth = 1200;
    constexpr int kAdvancedFileTinderMinHeight = 800;
    constexpr int kFolderTreePanelMinWidth = 400;
    constexpr int kFolderTreePanelMaxWidth = 600;
    
    // Quick Access Bar
    constexpr int kQuickAccessBarHeight = 60;
    constexpr int kQuickAccessButtonWidth = 120;
    
    // Mind Map Node sizes (larger for visual orientation)
    constexpr int kNodeWidth = 180;
    constexpr int kNodeHeight = 80;
    constexpr int kNodeSpacingHorizontal = 60;
    constexpr int kNodeSpacingVertical = 40;
    constexpr int kNodeBorderRadius = 12;
    
    // File preview
    constexpr int kPreviewMaxWidth = 500;
    constexpr int kPreviewMaxHeight = 400;
}

namespace ui::colors {
    constexpr const char* kNodeDefaultBg = "#3498db";
    constexpr const char* kNodeSelectedBg = "#2ecc71";
    constexpr const char* kNodeVirtualBg = "#9b59b6";
    constexpr const char* kNodeConnectedBg = "#e67e22";
    constexpr const char* kConnectionLine = "#7f8c8d";
    constexpr const char* kDeleteColor = "#e74c3c";
    constexpr const char* kKeepColor = "#2ecc71";
    constexpr const char* kSkipColor = "#f39c12";
    constexpr const char* kMoveColor = "#3498db";
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
