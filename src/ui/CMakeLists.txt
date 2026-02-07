# UI Components
target_sources(KiraWAVTar PRIVATE
    MainWindow.cpp
    MainWindow.h
    MainWindow.ui
)

target_include_directories(KiraWAVTar PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# Add UI subdirectories
add_subdirectory(dialogs)
add_subdirectory(widgets)
add_subdirectory(models)
