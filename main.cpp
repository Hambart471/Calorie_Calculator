#include "ui_manager.h"    // Manages user interaction, rendering UI, and input processing.
#include "data_manager.h"  // Handles loading and storing persistent data.
#include "constants.h"     // Global constants and helper functions
#include <iostream>
#include <string>

// -----------------------------------------------------------------------------
// Utility Lambda: center
// Purpose: Centrally align any given text on an 80-character line.
// -----------------------------------------------------------------------------
auto center = [](const std::string &text) -> std::string {
    int totalWidth = 80;
    int padding = (totalWidth - static_cast<int>(text.size())) / 2;
    if (padding < 0) padding = 0;
    return std::string(padding, ' ') + text;
};

int main() {
    // -------------------------------------------------------------------------
    // Create and initialize the DataManager:
    // - Load saved data from file (if exists)
    // - This instance maintains the data for daily nutritional records and goals.
    // -------------------------------------------------------------------------
    DataManager dataManager;
    dataManager.loadData();

    // -------------------------------------------------------------------------
    // Initialize the UIManager with the DataManager reference:
    // - This object handles rendering the console UI, keyboard interactions, etc.
    // -------------------------------------------------------------------------
    UIManager ui(dataManager);
    ui.init();

    // -------------------------------------------------------------------------
    // If running for the first time (i.e., no saved data exists),
    // prompt the user to input their daily nutritional goals.
    // -------------------------------------------------------------------------
    if (dataManager.isFirstRun()) {
        ui.handleStartGoals();
    }

    // -------------------------------------------------------------------------
    // Start the main UI loop.
    // - Keeps running until user quits via input.
    // -------------------------------------------------------------------------
    ui.run();
    return 0;
}