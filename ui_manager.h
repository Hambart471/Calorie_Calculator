#ifndef UI_MANAGER_H
#define UI_MANAGER_H

// -----------------------------------------------------------------------------
// File: ui_manager.h
// Purpose: Declare the UIManager class which manages the entire console UI.
//          Includes methods for rendering different screens, processing input,
//          and navigating between the different sections of the application.
// -----------------------------------------------------------------------------

#include <string>
#include <vector>
#include "data_manager.h"  // Provides access to persistent data

// -----------------------------------------------------------------------------
// Enum: UIState
// Purpose: Enumerate possible states of the UI (e.g., main menu, calendar view).
// -----------------------------------------------------------------------------
enum UIState {
    STATE_MAIN_MENU,  // The main menu where general actions are available.
    STATE_CALENDAR    // Calendar view for selecting dates.
    // Future states (for editing, settings, etc.) can be added as needed.
};

// -----------------------------------------------------------------------------
// Class: UIManager
// Purpose: Provides all functions for rendering the UI in the console, processing 
//          keyboard input, and calling business logic in DataManager.
// -----------------------------------------------------------------------------
class UIManager {
public:
    // Constructor: requires a reference to a DataManager instance.
    UIManager(DataManager &dataManager);
    ~UIManager();

    // Initializes the console UI; hides the cursor and clears the screen.
    void init();
    // Enters the main application loop waiting for user input to update the UI.
    void run();
    // Renders the main menu screen including nutritional totals and food lists.
    void renderMainMenu();
    // Renders the calendar UI to select dates.
    void renderCalendar();
    // Processes key inputs when in the main menu.
    void processInput(char key);
    // Processes key inputs when in the calendar view.
    void processCalendarInput(char key);
    // Utility to clear the console screen.
    void clearScreen();
    // Utility to set the console cursor to a specific (x,y) location.
    void setCursorPosition(int x, int y);
    // Draws borders around UI components (e.g., for better visual grouping).
    void drawBorder(int x, int y, int width, int height);
    // Returns the current date formatted as "DD/MM/YYYY - DayName".
    std::string getDisplayDate() const;
    // Allows the date to be changed by a specified day offset.
    void changeDateByOffset(int offset);

    // -------------------------------------------------------------------------
    // New method: handleStartGoals
    // Purpose: On the first run, prompt the user to enter their daily nutritional goals 
    //          using an inline editing UI screen.
    // -------------------------------------------------------------------------
    void handleStartGoals();

private:
    std::string calendarOriginalDate;  // Stores date before switching to calendar view
    // Private helper methods for food template operations and UI updates.
    void handleEditFood(int foodIndex);    // Edit an existing food entry
    void handleAddFromTemplate();          // Add food from a list of predefined templates
    void handleAddCustomFood();            // Add a food entry manually
    void updateTotals();                   // Recalculates nutritional totals for the day
    void playSoundForKey(char key);        // (Future extension) Play a sound based on key input
    void handleResetGoals();               // Reset current daily nutritional goals

    DataManager &dataManager;  // Reference to the DataManager object for data operations.
    UIState currentState;      // Represents the current state of the UI.
    std::string currentDate;   // Stores the current date in "DD/MM/YYYY" format.

    // Variables to manage selection in menus and scrolling for food entries.
    int selectedIndex;                 // Global selection index for menu and food list items.
    std::vector<std::string> menuItems; // List of menu options for easy rendering.
    int foodScrollOffset;              // Offset for scrolling through food entries.

    // Nutritional totals for the currently displayed day.
    int totalCalories;
    int totalCarbs;
    int totalProtein;
    int totalFat;

    // Variables specific to the calendar view.
    int selectedCalendarDay;  // Currently selected day in the calendar grid.
};

#endif // UI_MANAGER_H
