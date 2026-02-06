add_library(KiraCommonUtils STATIC
    src/filesystem.cpp
    src/darkmode.cpp
    src/stringfunc.cpp
    src/updatechecker.cpp
    src/dialogs/listviewdialog.cpp
    src/dialogs/showhtmldialog.cpp
    src/dialogs/tableviewdialog.cpp
    src/dialogs/showdiffdialog.cpp
    src/i18n/translation.cpp
    src/i18n/translationmanager.cpp
    src/widgets/atleastonecheckedbuttongroup.cpp
    src/widgets/dirnameeditwithbrowse.cpp
    src/widgets/filenameeditwithbrowse.cpp
    src/widgets/misc.cpp
    src/widgets/stringlistmodifywidget.cpp
    src/widgets/qballontip.cpp
    # Third-party: diff-match-patch
    lib/diff-match-patch/diff_match_patch.cpp
    # Headers with Q_OBJECT (needed for AUTOMOC)
    src/include/kira/darkmode.h
    src/include/kira/updatechecker.h
    src/include/kira/dialogs/listviewdialog.h
    src/include/kira/dialogs/showhtmldialog.h
    src/include/kira/dialogs/tableviewdialog.h
    src/include/kira/dialogs/showdiffdialog.h
    src/include/kira/widgets/atleastonecheckedbuttongroup.h
    src/include/kira/widgets/dirnameeditwithbrowse.h
    src/include/kira/widgets/filenameeditwithbrowse.h
    src/include/kira/widgets/stringlistmodifywidget.h
    src/include/kira/widgets/qballontip.h
    # UI files
    src/dialogs/showhtmldialog.ui
    src/widgets/filenameeditwithbrowse.ui
    src/widgets/stringlistmodifywidget.ui
    src/dialogs/showdiffdialog.ui
)

target_include_directories(KiraCommonUtils
    PUBLIC src/include
    PRIVATE lib/fplus/include
)

target_link_libraries(KiraCommonUtils
    PUBLIC Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Network
    PUBLIC kfr eternal
)
