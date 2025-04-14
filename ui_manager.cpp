#include "ui_manager.h"
#include "constants.h"    // Provides console dimensions, color codes, and sound functions.
#include <iostream>
#include <conio.h>        // For _getch() used for capturing keyboard input.
#include <windows.h>
#include <ctime>          // For handling dates and time functions.
#include <cstdio>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <vector>
#include <limits>

// -----------------------------------------------------------------------------
// Global Variables and Helper Definitions for UIManager:
// -----------------------------------------------------------------------------

// A global vector holding predefined food templates.
static std::vector<Food> g_foodTemplates;

// Maximum display width allocated for food names in the UI table.
const int maxNameLen = 21;

// Utility function to get the day name from a given tm structure (e.g., Monday, Tuesday).
std::string getDayOfWeek(const std::tm &timeInfo) {
    static const char *days[] = { "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday" };
    return days[timeInfo.tm_wday];
}

// -----------------------------------------------------------------------------
// UIManager Implementation
// -----------------------------------------------------------------------------

// Constructor: Initializes the UIManager, sets the initial UI state, and prepares menu items.
UIManager::UIManager(DataManager &dm) : 
    dataManager(dm), 
    currentState(STATE_MAIN_MENU), 
    selectedIndex(0),
    totalCalories(0), 
    totalCarbs(0), 
    totalProtein(0), 
    totalFat(0),
    foodScrollOffset(0),
    selectedCalendarDay(1),
    calendarOriginalDate("")
{
    // Obtain and set the current system date in "DD/MM/YYYY" format.
    time_t now = time(0);
    tm localTime;
    localtime_s(&localTime, &now);
    char buffer[11];
    sprintf_s(buffer, "%02d/%02d/%04d", localTime.tm_mday, localTime.tm_mon + 1, localTime.tm_year + 1900);
    currentDate = buffer;

    // Define the main menu items.
    // Item 0: "Add from templates", Item 1: "Add custom food", Item 2: "Calendar", Item 3: "Reset goals"
    menuItems = { "Add from templates", "Add custom food", "Calendar", "Reset goals" };
}

// Destructor: Currently no dynamic allocation requires explicit cleanup.
UIManager::~UIManager() {
    // Nothing to delete explicitly.
}

// -----------------------------------------------------------------------------
// Method: init
// Purpose: Sets up the console environment (e.g., hides the cursor) and clears the screen.
// -----------------------------------------------------------------------------
void UIManager::init() {
    // Hide the blinking console cursor for a cleaner UI appearance.
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO cursorInfo;
    GetConsoleCursorInfo(hOut, &cursorInfo);
    cursorInfo.bVisible = FALSE;
    SetConsoleCursorInfo(hOut, &cursorInfo);
    clearScreen();
}

// -----------------------------------------------------------------------------
// Method: run
// Purpose: Core main loop of the UI; continuously refreshes and processes input.
// -----------------------------------------------------------------------------
void UIManager::run() {
    while (true) {
        // Check current UI state and render the corresponding screen.
        if (currentState == STATE_MAIN_MENU) {
            renderMainMenu();
            // Wait for a keypress to drive the UI navigation.
            char key = _getch();
            processInput(key);
        }
        else if (currentState == STATE_CALENDAR) {
            renderCalendar();
            // Similarly, block until user presses a key.
            char key = _getch();
            processCalendarInput(key);
        }
    }
}

// -----------------------------------------------------------------------------
// Utility: clearScreen
// Purpose: Clears the console window using system calls.
// -----------------------------------------------------------------------------
void UIManager::clearScreen() {
    system("cls");
}

// -----------------------------------------------------------------------------
// Utility: setCursorPosition
// Purpose: Sets the console cursor at a given (x,y) position.
// -----------------------------------------------------------------------------
void UIManager::setCursorPosition(int x, int y) {
    COORD coord = { static_cast<SHORT>(x), static_cast<SHORT>(y) };
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coord);
}

// -----------------------------------------------------------------------------
// Utility: drawBorder
// Purpose: Draws a border of given dimensions at the specified location.
// -----------------------------------------------------------------------------
void UIManager::drawBorder(int x, int y, int width, int height) {
    setCursorPosition(x, y);
    std::cout << "+" << std::string(width - 2, '-') << "+";
    for (int i = 1; i < height - 1; ++i) {
        setCursorPosition(x, y + i);
        std::cout << "|" << std::string(width - 2, ' ') << "|";
    }
    setCursorPosition(x, y + height - 1);
    std::cout << "+" << std::string(width - 2, '-') << "+";
}

// -----------------------------------------------------------------------------
// Method: getDisplayDate
// Purpose: Formats the current date along with the day name (e.g., "15/04/2025 - Tuesday").
// -----------------------------------------------------------------------------
std::string UIManager::getDisplayDate() const {
    int day, month, year;
    sscanf_s(currentDate.c_str(), "%d/%d/%d", &day, &month, &year);
    tm timeInfo = {};
    timeInfo.tm_mday = day;
    timeInfo.tm_mon = month - 1;
    timeInfo.tm_year = year - 1900;
    mktime(&timeInfo);
    std::string dayName = getDayOfWeek(timeInfo);
    return currentDate + " - " + dayName;
}

// -----------------------------------------------------------------------------
// Method: changeDateByOffset
// Purpose: Modifies the current date by a given number of days (negative for previous day, positive for next day).
// -----------------------------------------------------------------------------
void UIManager::changeDateByOffset(int offset) {
    int day, month, year;
    sscanf_s(currentDate.c_str(), "%d/%d/%d", &day, &month, &year);
    tm timeInfo = {};
    timeInfo.tm_mday = day + offset;
    timeInfo.tm_mon = month - 1;
    timeInfo.tm_year = year - 1900;
    mktime(&timeInfo);
    char buffer[11];
    sprintf_s(buffer, "%02d/%02d/%04d", timeInfo.tm_mday, timeInfo.tm_mon + 1, timeInfo.tm_year + 1900);
    currentDate = buffer;
    // Provide feedback for page switching.
    Sounds::PlayPageSwitchSound();
}

// -----------------------------------------------------------------------------
// Method: updateTotals
// Purpose: Recalculates the total nutritional values for the current day from all food entries.
// -----------------------------------------------------------------------------
void UIManager::updateTotals() {
    totalCalories = totalCarbs = totalProtein = totalFat = 0;
    DailyRecord &record = dataManager.getRecord(currentDate);
    // Sum each nutritional field across all food entries.
    for (const auto &food : record.foods) {
        totalCalories += food.calories;
        totalCarbs += food.carbs;
        totalProtein += food.protein;
        totalFat += food.fat;
    }
}

// -----------------------------------------------------------------------------
// Method: renderMainMenu
// Purpose: Renders the main menu screen including header, nutritional totals,
//          food list table, and navigation tips.
// -----------------------------------------------------------------------------
void UIManager::renderMainMenu() {
    clearScreen();
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

    // Define bright colors to be used for visual feedback.
    int brightGreen   = FOREGROUND_GREEN | FOREGROUND_INTENSITY;
    int brightCyan    = FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
    int brightBlue    = FOREGROUND_BLUE | FOREGROUND_INTENSITY;
    int brightMagenta = FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
    int brightRed     = FOREGROUND_RED | FOREGROUND_INTENSITY;
    int selectedBrightRed = brightRed | BACKGROUND_BLUE;  
    int darkRed       = FOREGROUND_RED;
    int gray = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;

    // Render the date header.
    SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
    setCursorPosition(0, 0);
    std::cout << getDisplayDate();
    SetConsoleTextAttribute(hConsole, ConsoleColors::DEFAULT);

    updateTotals();  // Update totals before displaying nutritional info
    DailyGoals goals = dataManager.getDailyGoals();

    // Display the calories information with formatting.
    int calLineY = 2;
    setCursorPosition((CONSOLE_WIDTH / 2) - 10, calLineY);
    SetConsoleTextAttribute(hConsole, brightGreen);
    std::cout << "Calories: ";
    SetConsoleTextAttribute(hConsole, ConsoleColors::DEFAULT);
    int dispTotalCal = (totalCalories > 9999) ? 9999 : totalCalories;
    int dispGoalCal = (goals.calories > 9999) ? 9999 : goals.calories;
    std::ostringstream calStream;
    calStream << std::setw(4) << std::setfill('0') << dispTotalCal << " / " 
              << std::setw(4) << std::setfill('0') << dispGoalCal;
    std::string calStr = calStream.str();
    std::cout << calStr;
    std::cout << std::setfill(' ');
    if (totalCalories > goals.calories) {
        setCursorPosition((CONSOLE_WIDTH / 2) - 10 + 10, calLineY);
        SetConsoleTextAttribute(hConsole, darkRed);
        std::cout << calStr;
        SetConsoleTextAttribute(hConsole, ConsoleColors::DEFAULT);
    }

    // Display macronutrient details: Carbs, Protein, Fat.
    std::string carbsLabel = "Carbs: ";
    std::string protLabel  = "Protein: ";
    std::string fatLabel   = "Fat: ";
    int dispTotalCarbs = (totalCarbs > 999) ? 999 : totalCarbs;
    int dispGoalCarbs = (goals.carbs > 999) ? 999 : goals.carbs;
    int dispTotalProtein = (totalProtein > 999) ? 999 : totalProtein;
    int dispGoalProtein = (goals.protein > 999) ? 999 : goals.protein;
    int dispTotalFat = (totalFat > 999) ? 999 : totalFat;
    int dispGoalFat = (goals.fat > 999) ? 999 : goals.fat;
    std::ostringstream carbsStream, protStream, fatStream;
    carbsStream << std::setw(3) << std::setfill('0') << dispTotalCarbs << " / " 
                << std::setw(3) << std::setfill('0') << dispGoalCarbs;
    protStream  << std::setw(3) << std::setfill('0') << dispTotalProtein << " / " 
                << std::setw(3) << std::setfill('0') << dispGoalProtein;
    fatStream   << std::setw(3) << std::setfill('0') << dispTotalFat << " / " 
                << std::setw(3) << std::setfill('0') << dispGoalFat;
    std::string carbsNum = carbsStream.str();
    std::string protNum  = protStream.str();
    std::string fatNum   = fatStream.str();
    std::string macroCombined = carbsLabel + carbsNum + "  " +
                                protLabel  + protNum  + "  " +
                                fatLabel   + fatNum;
    int macroLineY = 3;
    int macroStartX = (CONSOLE_WIDTH - static_cast<int>(macroCombined.length())) / 2;
    // Print Carbs info.
    setCursorPosition(macroStartX, macroLineY);
    SetConsoleTextAttribute(hConsole, brightCyan);
    std::cout << carbsLabel;
    SetConsoleTextAttribute(hConsole, ConsoleColors::DEFAULT);
    int carbsNumbersX = macroStartX + static_cast<int>(carbsLabel.length());
    setCursorPosition(carbsNumbersX, macroLineY);
    if (totalCarbs > goals.carbs) {
        SetConsoleTextAttribute(hConsole, darkRed);
        std::cout << carbsNum;
        SetConsoleTextAttribute(hConsole, ConsoleColors::DEFAULT);
    } else {
        std::cout << carbsNum;
    }
    
    // Print Protein info.
    int protStartX = carbsNumbersX + static_cast<int>(carbsNum.length()) + 2;
    setCursorPosition(protStartX, macroLineY);
    SetConsoleTextAttribute(hConsole, brightBlue);
    std::cout << protLabel;
    SetConsoleTextAttribute(hConsole, ConsoleColors::DEFAULT);
    int protNumbersX = protStartX + static_cast<int>(protLabel.length());
    setCursorPosition(protNumbersX, macroLineY);
    if (totalProtein > goals.protein) {
        SetConsoleTextAttribute(hConsole, darkRed);
        std::cout << protNum;
        SetConsoleTextAttribute(hConsole, ConsoleColors::DEFAULT);
    } else {
        std::cout << protNum;
    }
    
    // Print Fat info.
    int fatStartX = protNumbersX + static_cast<int>(protNum.length()) + 2;
    setCursorPosition(fatStartX, macroLineY);
    SetConsoleTextAttribute(hConsole, brightMagenta);
    std::cout << fatLabel;
    SetConsoleTextAttribute(hConsole, ConsoleColors::DEFAULT);
    int fatNumbersX = fatStartX + static_cast<int>(fatLabel.length());
    setCursorPosition(fatNumbersX, macroLineY);
    if (totalFat > goals.fat) {
        SetConsoleTextAttribute(hConsole, darkRed);
        std::cout << fatNum;
        SetConsoleTextAttribute(hConsole, ConsoleColors::DEFAULT);
    } else {
        std::cout << fatNum;
    }
    
    // Draw horizontal separator line.
    setCursorPosition(0, 5);
    std::cout << std::string(CONSOLE_WIDTH, '=');

    int menuCount = static_cast<int>(menuItems.size());
    DailyRecord &record = dataManager.getRecord(currentDate);
    int foodCount = static_cast<int>(record.foods.size());
    int totalSelectable = menuCount + foodCount;
    int menuStartY = 6;
    
    // Render main menu buttons.
    for (int i = 0; i < menuCount; i++) {
        std::string displayText = "[" + menuItems[i] + "]";
        int xPos = (CONSOLE_WIDTH - static_cast<int>(displayText.length())) / 2;
        setCursorPosition(xPos, menuStartY + i);
        if (selectedIndex == i) {
            SetConsoleTextAttribute(hConsole, selectedBrightRed);
            std::cout << displayText;
            SetConsoleTextAttribute(hConsole, ConsoleColors::DEFAULT);
        } else {
            SetConsoleTextAttribute(hConsole, brightRed);
            std::cout << displayText;
            SetConsoleTextAttribute(hConsole, ConsoleColors::DEFAULT);
        }
    }
    
    // Draw another separator below menu buttons.
    int borderY = menuStartY + menuItems.size();
    setCursorPosition(0, borderY);
    std::cout << std::string(CONSOLE_WIDTH, '=');

    // Render the food list table.
    const int detailsX = maxNameLen + 1;  // Food name occupies columns 0..maxNameLen-1.
    int availableWidth = CONSOLE_WIDTH - 1 - detailsX; // Reserve rightmost column for scroll indicator.

    int foodListStartY = borderY + 1;
    int visibleFoodSlots = CONSOLE_HEIGHT - 4 - borderY;
    if (visibleFoodSlots < 0)
        visibleFoodSlots = 0;
    if (foodScrollOffset > foodCount - visibleFoodSlots)
        foodScrollOffset = (foodCount - visibleFoodSlots >= 0 ? foodCount - visibleFoodSlots : 0);

    // Render each food entry in the current viewport.
    for (int j = foodScrollOffset; j < foodCount && j < foodScrollOffset + visibleFoodSlots; j++) {
        int globalIndex = menuCount + j;
        int currentRow = foodListStartY + j - foodScrollOffset;
        // Format the food name to fit in the allocated width.
        std::stringstream nameStream;
        nameStream << std::setw(maxNameLen) << std::left << record.foods[j].name;
        std::string formattedName = nameStream.str();

        // Format food details for display.
        int dispFoodGrams = (record.foods[j].grams > 9999) ? 9999 : record.foods[j].grams;
        int dispFoodCal = (record.foods[j].calories > 9999) ? 9999 : record.foods[j].calories;
        int dispFoodCarbs = (record.foods[j].carbs > 999) ? 999 : record.foods[j].carbs;
        int dispFoodProtein = (record.foods[j].protein > 999) ? 999 : record.foods[j].protein;
        int dispFoodFat = (record.foods[j].fat > 999) ? 999 : record.foods[j].fat;

        std::ostringstream gramsStream, calStreamFood, carbsStreamFood, protStreamFood, fatStreamFood;
        gramsStream << std::setw(4) << std::setfill('0') << dispFoodGrams << " grams";
        calStreamFood << std::setw(4) << std::setfill('0') << dispFoodCal << " calories";
        carbsStreamFood << std::setw(3) << std::setfill('0') << dispFoodCarbs << " carbs";
        protStreamFood  << std::setw(3) << std::setfill('0') << dispFoodProtein << " protein";
        fatStreamFood   << std::setw(3) << std::setfill('0') << dispFoodFat << " fat";
        std::string gramsStr = gramsStream.str();
        std::string calStrFood = calStreamFood.str();
        std::string carbsStr = carbsStreamFood.str();
        std::string protStr = protStreamFood.str();
        std::string fatStr = fatStreamFood.str();

        // Build a combined details string.
        std::string detailsCombined = gramsStr + " " + calStrFood + " " + carbsStr + " " + protStr + " " + fatStr;
        int detailsLength = static_cast<int>(detailsCombined.length());
        int detailsPrintX = detailsX;
        if (detailsLength < availableWidth)
            detailsPrintX += (availableWidth - detailsLength);

        // Render the food details differently if this row is selected.
        if (selectedIndex == globalIndex) {
            SetConsoleTextAttribute(hConsole, selectedBrightRed);
            setCursorPosition(0, currentRow);
            std::cout << formattedName;
            setCursorPosition(detailsPrintX, currentRow);
            SetConsoleTextAttribute(hConsole, gray);
            std::cout << gramsStr;
            std::cout << " ";
            SetConsoleTextAttribute(hConsole, brightGreen);
            std::cout << calStrFood;
            std::cout << " ";
            SetConsoleTextAttribute(hConsole, brightCyan);
            std::cout << carbsStr;
            std::cout << " ";
            SetConsoleTextAttribute(hConsole, brightBlue);
            std::cout << protStr;
            std::cout << " ";
            SetConsoleTextAttribute(hConsole, brightMagenta);
            std::cout << fatStr;
            SetConsoleTextAttribute(hConsole, ConsoleColors::DEFAULT);
        } else {
            setCursorPosition(0, currentRow);
            SetConsoleTextAttribute(hConsole, brightRed);
            std::cout << formattedName;
            SetConsoleTextAttribute(hConsole, ConsoleColors::DEFAULT);
            setCursorPosition(detailsPrintX, currentRow);
            SetConsoleTextAttribute(hConsole, gray);
            std::cout << gramsStr;
            std::cout << " ";
            SetConsoleTextAttribute(hConsole, brightGreen);
            std::cout << calStrFood;
            std::cout << " ";
            SetConsoleTextAttribute(hConsole, brightCyan);
            std::cout << carbsStr;
            std::cout << " ";
            SetConsoleTextAttribute(hConsole, brightBlue);
            std::cout << protStr;
            std::cout << " ";
            SetConsoleTextAttribute(hConsole, brightMagenta);
            std::cout << fatStr;
            SetConsoleTextAttribute(hConsole, ConsoleColors::DEFAULT);
        }
    }

    // Render scroll indicator if there are more food items than visible.
    if (foodCount > visibleFoodSlots) {
        int scrollColumn = CONSOLE_WIDTH - 1;
        for (int row = foodListStartY; row < foodListStartY + visibleFoodSlots; row++) {
            setCursorPosition(scrollColumn, row);
            std::cout << "|";
        }
        int maxIndicatorPosition = visibleFoodSlots - 1;
        int scrollRange = foodCount - visibleFoodSlots;
        int indicatorRow = foodListStartY;
        if (scrollRange > 0) {
            indicatorRow = foodListStartY + (foodScrollOffset * maxIndicatorPosition) / scrollRange;
        }
        SetConsoleTextAttribute(hConsole, selectedBrightRed);
        setCursorPosition(scrollColumn, indicatorRow);
        std::cout << char(219);
        SetConsoleTextAttribute(hConsole, ConsoleColors::DEFAULT);
    }
    
    // Render tips and instructions along the bottom.
    SetConsoleTextAttribute(hConsole, 8);
    setCursorPosition(0, CONSOLE_HEIGHT - 3);
    std::cout << std::string(CONSOLE_WIDTH, '-');
    SetConsoleTextAttribute(hConsole, ConsoleColors::DEFAULT);
    
    std::string tips = "[q] Quit  [j/k] Down/Up  [h/l] Prev Day/Next Day  [Enter] Select  [x] Delete";
    SetConsoleTextAttribute(hConsole, 8);
    int tipX = (CONSOLE_WIDTH - static_cast<int>(tips.length())) / 2;
    setCursorPosition(tipX, CONSOLE_HEIGHT - 2);
    std::cout << tips;
    SetConsoleTextAttribute(hConsole, ConsoleColors::DEFAULT);
}

// -----------------------------------------------------------------------------
// Method: processInput
// Purpose: Handle keypresses in the main menu state, such as navigation,
//          adding or editing food entries, changing dates, etc.
// -----------------------------------------------------------------------------
void UIManager::processInput(char key) {
    int menuCount = static_cast<int>(menuItems.size());
    DailyRecord &record = dataManager.getRecord(currentDate);
    int foodCount = static_cast<int>(record.foods.size());
    int totalSelectable = menuCount + foodCount;
    int borderY = 6 + menuItems.size();
    int visibleSlots = CONSOLE_HEIGHT - 4 - borderY;
    if (visibleSlots < 0)
        visibleSlots = 0;
    
    if (currentState == STATE_MAIN_MENU) {
        if (key == 'j') {
            if (totalSelectable > 0) {
                if (selectedIndex < totalSelectable - 1)
                    selectedIndex++;
                else
                    selectedIndex = 0;
            }
            if (selectedIndex >= menuCount) {
                int foodIndex = selectedIndex - menuCount;
                if (foodIndex >= visibleSlots + foodScrollOffset)
                    foodScrollOffset = foodIndex - visibleSlots + 1;
            }
            if (selectedIndex < menuCount)
                foodScrollOffset = 0;
            Sounds::PlayNavigationSound();
        } else if (key == 'k') {
            if (totalSelectable > 0) {
                if (selectedIndex > 0)
                    selectedIndex--;
                else
                    selectedIndex = totalSelectable - 1;
            }
            if (selectedIndex >= menuCount) {
                int foodIndex = selectedIndex - menuCount;
                if (foodIndex < foodScrollOffset)
                    foodScrollOffset = foodIndex;
                if (selectedIndex == totalSelectable - 1) {
                    if (foodCount > visibleSlots)
                        foodScrollOffset = foodCount - visibleSlots;
                    else
                        foodScrollOffset = 0;
                }
            }
            if (selectedIndex < menuCount)
                foodScrollOffset = 0;
            Sounds::PlayNavigationSound();
        } else if (key == '\r') {
            Sounds::PlaySelectSound();
            if (selectedIndex < menuCount) {
                if (selectedIndex == 0) {
                    // [Add from templates] option.
                    handleAddFromTemplate();
                    return;
                } else if (selectedIndex == 1) {
                    // [Add custom food] option.
                    handleAddCustomFood();
                    return;
                } else if (selectedIndex == 2) {
                    // Switch to calendar view.
                    calendarOriginalDate = currentDate;
                    currentState = STATE_CALENDAR;
                    selectedCalendarDay = 1;
                } else if (selectedIndex == 3) {
                    // Option to reset daily nutritional goals.
                    handleResetGoals();
                }
            } else {
                int foodIndex = selectedIndex - menuCount;
                if (foodIndex >= 0 && foodIndex < foodCount) {
                    // Call the inline food editing interface.
                    handleEditFood(foodIndex);
                }
            }
        } else if (key == 'x') {
            // Delete the selected food entry.
            if (selectedIndex >= menuCount) {
                int foodIndex = selectedIndex - menuCount;
                if (foodIndex >= 0 && foodIndex < foodCount) {
                    record.foods.erase(record.foods.begin() + foodIndex);
                    if (selectedIndex >= menuCount + static_cast<int>(record.foods.size()))
                        selectedIndex = menuCount + static_cast<int>(record.foods.size()) - 1;
                    dataManager.saveData();
                    Sounds::PlaySelectSound();
                }
            }
        } else if (key == 'h') {
            // Move to the previous day.
            changeDateByOffset(-1);
            selectedIndex = 0;
            foodScrollOffset = 0;
        } else if (key == 'l') {
            // Move to the next day.
            changeDateByOffset(1);
            selectedIndex = 0;
            foodScrollOffset = 0;
        } else if (key == 'q') {
            // Quit the application.
            Sounds::PlaySelectSound();
            exit(0);
        }
    }
}

// -----------------------------------------------------------------------------
// Method: handleEditFood
// Purpose: Provides inline editing functionality for an existing food entry.
// -----------------------------------------------------------------------------
void UIManager::handleEditFood(int foodIndex) {
    DailyRecord &record = dataManager.getRecord(currentDate);
    if (foodIndex < 0 || foodIndex >= record.foods.size()) return;
    
    Food &foodToEdit = record.foods[foodIndex];
    clearScreen();
    int midX = CONSOLE_WIDTH / 2;
    int startY = 8;
    int localSelection = 0;
    bool done = false;
    // Pre-populate local variables with the current food details.
    std::string foodName = foodToEdit.name;
    int calories = foodToEdit.calories;
    int carbs = foodToEdit.carbs;
    int protein = foodToEdit.protein;
    int fat = foodToEdit.fat;
    int grams = foodToEdit.grams;
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    
    while (!done) {
        clearScreen();
        
        // Define editable fields and display their current values.
        std::string fieldLabels[6] = { "Food Name", "Calories", "Carbs", "Protein", "Fat", "Grams" };
        std::string fieldValues[6];
        fieldValues[0] = (foodName.empty() ? "<empty>" : foodName);
        fieldValues[1] = std::to_string(calories);
        fieldValues[2] = std::to_string(carbs);
        fieldValues[3] = std::to_string(protein);
        fieldValues[4] = std::to_string(fat);
        fieldValues[5] = std::to_string(grams);
        
        // Render each field as a button with the current value.
        for (int i = 0; i < 6; i++) {
            std::stringstream ss;
            ss << "[" << fieldLabels[i] << ": " << fieldValues[i] << "]";
            std::string buttonText = ss.str();
            int buttonX = (CONSOLE_WIDTH - static_cast<int>(buttonText.length())) / 2;
            setCursorPosition(buttonX, startY + i);
            if (localSelection == i) {
                SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_INTENSITY | BACKGROUND_BLUE);
                std::cout << buttonText;
                SetConsoleTextAttribute(hConsole, ConsoleColors::DEFAULT);
            } else {
                SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_INTENSITY);
                std::cout << buttonText;
                SetConsoleTextAttribute(hConsole, ConsoleColors::DEFAULT);
            }
        }
        
        // Render the update button.
        std::string updateButton = "[Update]";
        int updateY = startY + 7;
        int updateX = (CONSOLE_WIDTH - static_cast<int>(updateButton.length())) / 2;
        setCursorPosition(updateX, updateY);
        if (localSelection == 6) {
            SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_INTENSITY | BACKGROUND_BLUE);
            std::cout << updateButton;
            SetConsoleTextAttribute(hConsole, ConsoleColors::DEFAULT);
        } else {
            SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_INTENSITY);
            std::cout << updateButton;
            SetConsoleTextAttribute(hConsole, ConsoleColors::DEFAULT);
        }
        
        // Render tips at the bottom.
        SetConsoleTextAttribute(hConsole, 8);
        setCursorPosition(0, CONSOLE_HEIGHT - 3);
        std::cout << std::string(CONSOLE_WIDTH, '-');
        SetConsoleTextAttribute(hConsole, ConsoleColors::DEFAULT);
        std::string tips = "[q] Back  [j/k] Down/Up  [Enter] Select";
        int tipX = (CONSOLE_WIDTH - static_cast<int>(tips.length())) / 2;
        setCursorPosition(tipX, CONSOLE_HEIGHT - 2);
        SetConsoleTextAttribute(hConsole, 8);
        std::cout << tips;
        SetConsoleTextAttribute(hConsole, ConsoleColors::DEFAULT);
        
        // Process keyboard input for editing fields.
        char key = _getch();
        if (key == 'j') {
            localSelection++;
            if (localSelection > 6)
                localSelection = 0;
            Sounds::PlayNavigationSound();
        } else if (key == 'k') {
            localSelection--;
            if (localSelection < 0)
                localSelection = 6;
            Sounds::PlayNavigationSound();
        } else if (key == '\r') {
            Sounds::PlaySelectSound();
            if (localSelection >= 0 && localSelection < 6) {
                std::stringstream prefixStream;
                prefixStream << "[" << fieldLabels[localSelection] << ": ";
                std::string prefix = prefixStream.str();
                std::stringstream fullButtonStream;
                fullButtonStream << "[" << fieldLabels[localSelection] << ": " << fieldValues[localSelection] << "]";
                std::string buttonText = fullButtonStream.str();
                int buttonX = (CONSOLE_WIDTH - static_cast<int>(buttonText.length())) / 2;
                int editX = buttonX + static_cast<int>(prefix.length());
                int editY = startY + localSelection;
                setCursorPosition(editX, editY);
                std::cout << std::string(20, ' ');
                setCursorPosition(editX, editY);
                std::string input;
                std::getline(std::cin, input);
                if (!input.empty()) {
                    if (localSelection == 0) {
                        if (input.size() > maxNameLen)
                            foodName = input.substr(0, maxNameLen);
                        else
                            foodName = input;
                    } else {
                        try {
                            int value = std::stoi(input);
                            switch(localSelection) {
                                case 1: calories = value; break;
                                case 2: carbs = value; break;
                                case 3: protein = value; break;
                                case 4: fat = value; break;
                                case 5: grams = value; break;
                            }
                        } catch (...) {
                            // Handle conversion errors gracefully.
                        }
                    }
                }
            } else if (localSelection == 6) {
                // Update the food entry with new values.
                foodToEdit.name = (foodName.empty() ? "<empty>" : foodName);
                foodToEdit.calories = calories;
                foodToEdit.carbs = carbs;
                foodToEdit.protein = protein;
                foodToEdit.fat = fat;
                foodToEdit.grams = grams;
                dataManager.saveData();
                done = true;
            }
        } else if (key == 'q') {
            Sounds::PlaySelectSound();
            done = true;
        }
    }
}

// -----------------------------------------------------------------------------
// Method: handleAddFromTemplate
// Purpose: Allows users to add a food entry using a pre-defined template.
//          Supports inline search editing, template creation, and deletion.
// -----------------------------------------------------------------------------
void UIManager::handleAddFromTemplate() {
    std::string searchTerm = "";
    int localSelection = 0; // 0: [Search: <term>], 1: [Create new template], then subsequent options.
    std::vector<Food> matches;
    bool done = false;
    bool searchEditing = false;
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

    int midY = CONSOLE_HEIGHT / 2;
    int popUpTop = midY - 4;

    // Variables to manage scrolling in the template list.
    int templateScrollOffset = 0;
    int visibleRows = CONSOLE_HEIGHT - popUpTop - 6;  // Adjust rows for buttons and tips.

    while (!done) {
        clearScreen();
        matches.clear();
        // Filter available templates based on search term.
        for (const auto &tpl : g_foodTemplates) {
            if (tpl.name.find(searchTerm) != std::string::npos) {
                matches.push_back(tpl);
            }
        }
        int totalOptions = 2 + static_cast<int>(matches.size()); // Top two options plus templates.

        // Render top buttons: Search and Create new template.
        std::string opt0 = "[Search: " + searchTerm + "]";
        std::string opt1 = "[Create new template]";
        int opt0X = (CONSOLE_WIDTH - static_cast<int>(opt0.length())) / 2;
        int opt1X = (CONSOLE_WIDTH - static_cast<int>(opt1.length())) / 2;
        setCursorPosition(opt0X, popUpTop);
        if (localSelection == 0) {
            SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | BACKGROUND_BLUE);
            std::cout << opt0;
            SetConsoleTextAttribute(hConsole, ConsoleColors::DEFAULT);
        } else {
            std::cout << opt0;
        }

        setCursorPosition(opt1X, popUpTop + 1);
        if (localSelection == 1) {
            Sounds::PlaySelectSound();
            SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | BACKGROUND_BLUE);
            std::cout << opt1;
            SetConsoleTextAttribute(hConsole, ConsoleColors::DEFAULT);
        } else {
            std::cout << opt1;
        }

        // Add space between buttons and template list.
        setCursorPosition((CONSOLE_WIDTH - 30) / 2, popUpTop + 2);
        std::cout << std::string(30, ' ');

        // Adjust scrolling if a template is selected.
        if (localSelection >= 2) {
            int relSelection = localSelection - 2;
            if (relSelection < templateScrollOffset)
                templateScrollOffset = relSelection;
            else if (relSelection >= visibleRows)
                templateScrollOffset = relSelection - visibleRows + 1;
        }

        // Render matching food templates.
        for (size_t i = templateScrollOffset; i < matches.size() && i < templateScrollOffset + visibleRows; i++) {
            int selectionIndex = 2 + static_cast<int>(i - templateScrollOffset);
            int row = popUpTop + 3 + static_cast<int>(i - templateScrollOffset);
            // Format template fields.
            std::stringstream nameStream, gramsStream, calStream, carbsStream, protStream, fatStream;
            nameStream << std::setw(maxNameLen) << std::left << matches[i].name;
            std::string foodNameStr = nameStream.str();
            gramsStream << std::setw(4) << std::setfill('0') << matches[i].grams << " grams";
            std::string gramsStr = gramsStream.str();
            calStream << std::setw(4) << std::setfill('0') << matches[i].calories << " calories";
            std::string calStr = calStream.str();
            carbsStream << std::setw(3) << std::setfill('0') << matches[i].carbs << " carbs";
            std::string carbsStr = carbsStream.str();
            protStream << std::setw(3) << std::setfill('0') << matches[i].protein << " protein";
            std::string protStr = protStream.str();
            fatStream << std::setw(3) << std::setfill('0') << matches[i].fat << " fat";
            std::string fatStr = fatStream.str();

            std::string combinedStr = foodNameStr + " " + gramsStr + " " + calStr + " " + carbsStr + " " + protStr + " " + fatStr;
            int startX = (CONSOLE_WIDTH - static_cast<int>(combinedStr.length())) / 2;
            setCursorPosition(startX, row);

            if (localSelection == selectionIndex) {
                SetConsoleTextAttribute(hConsole, FOREGROUND_RED | BACKGROUND_BLUE | FOREGROUND_INTENSITY);
                std::cout << foodNameStr;
                SetConsoleTextAttribute(hConsole, ConsoleColors::DEFAULT);
                std::cout << " " << gramsStr;
                SetConsoleTextAttribute(hConsole, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
                std::cout << " " << calStr;
                SetConsoleTextAttribute(hConsole, FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
                std::cout << " " << carbsStr;
                SetConsoleTextAttribute(hConsole, FOREGROUND_BLUE | FOREGROUND_INTENSITY);
                std::cout << " " << protStr;
                SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
                std::cout << " " << fatStr;
                SetConsoleTextAttribute(hConsole, ConsoleColors::DEFAULT);
            } else {
                SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_INTENSITY);
                std::cout << foodNameStr;
                SetConsoleTextAttribute(hConsole, ConsoleColors::DEFAULT);
                std::cout << " " << gramsStr;
                SetConsoleTextAttribute(hConsole, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
                std::cout << " " << calStr;
                SetConsoleTextAttribute(hConsole, FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
                std::cout << " " << carbsStr;
                SetConsoleTextAttribute(hConsole, FOREGROUND_BLUE | FOREGROUND_INTENSITY);
                std::cout << " " << protStr;
                SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
                std::cout << " " << fatStr;
                SetConsoleTextAttribute(hConsole, ConsoleColors::DEFAULT);
            }
        }

        // Optional vertical scroll indicator for template list.
        if (matches.size() > static_cast<size_t>(visibleRows)) {
            int indicatorColumn = CONSOLE_WIDTH - 2;
            for (int r = popUpTop + 3; r < popUpTop + 3 + visibleRows; r++) {
                setCursorPosition(indicatorColumn, r);
                std::cout << "|";
            }
            int scrollRange = static_cast<int>(matches.size()) - visibleRows;
            int indicatorRow = popUpTop + 3;
            if (scrollRange > 0)
                indicatorRow += (templateScrollOffset * (visibleRows - 1)) / scrollRange;
            SetConsoleTextAttribute(hConsole, FOREGROUND_RED | BACKGROUND_BLUE | FOREGROUND_INTENSITY);
            setCursorPosition(indicatorColumn, indicatorRow);
            std::cout << char(219);
            SetConsoleTextAttribute(hConsole, ConsoleColors::DEFAULT);
        }

        // Render bottom tips for this UI.
        SetConsoleTextAttribute(hConsole, 8);
        setCursorPosition(0, CONSOLE_HEIGHT - 3);
        std::cout << std::string(CONSOLE_WIDTH, '-');
        SetConsoleTextAttribute(hConsole, ConsoleColors::DEFAULT);
        std::string tips = "[q] Back  [j/k] Down/Up  [Enter] Select  [x] Delete";
        int tipX = (CONSOLE_WIDTH - static_cast<int>(tips.length())) / 2;
        setCursorPosition(tipX, CONSOLE_HEIGHT - 2);
        SetConsoleTextAttribute(hConsole, 8);
        std::cout << tips;
        SetConsoleTextAttribute(hConsole, ConsoleColors::DEFAULT);

        char key = _getch();

        if (localSelection == 0) {
            if (!searchEditing) {
                if (key == '\r') {
                    searchEditing = true;
                    continue;
                }
                if (key == 'j' || key == 'k' || key == 'x' || key == 'q')
                    ; // No specific action
                else
                    continue;
            } else {
                if (key == '\r') {
                    searchEditing = false;
                } else if (key == '\b') {
                    if (!searchTerm.empty())
                        searchTerm.pop_back();
                } else if (key >= 32 && key <= 126) {
                    searchTerm.push_back(key);
                }
                continue;
            }
        }

        if (key == 'j') {
            localSelection++;
            if (localSelection >= totalOptions)
                localSelection = 0;
            Sounds::PlayNavigationSound();
        } else if (key == 'k') {
            if (localSelection > 0)
                localSelection--;
            else
                localSelection = totalOptions - 1;
            Sounds::PlayNavigationSound();
        } else if (key == '\r') {
            if (localSelection == 0) {
                continue;
            } else if (localSelection == 1) {
                // Inline editing to create a new template.
                int editSelection = 0; // Fields: Template Name, Calories, Carbs, Protein, Fat, then Add button.
                std::string fieldLabels[5] = { "Template Name", "Calories", "Carbs", "Protein", "Fat" };
                std::string tplName = "";
                int cal = 0, carbs = 0, prot = 0, fat = 0;
                while (true) {
                    clearScreen();
                    int startY = midY - 4;
                    for (int i = 0; i < 5; i++) {
                        std::stringstream ss;
                        if (i == 0) {
                            ss << "[" << fieldLabels[i] << ": " << (tplName.empty() ? "<empty>" : tplName) << "]";
                        } else {
                            int value = (i == 1 ? cal : (i == 2 ? carbs : (i == 3 ? prot : fat)));
                            ss << "[" << fieldLabels[i] << ": " << value << "]";
                        }
                        std::string buttonText = ss.str();
                        int buttonX = (CONSOLE_WIDTH - static_cast<int>(buttonText.length())) / 2;
                        setCursorPosition(buttonX, startY + i);
                        if (editSelection == i) {
                            SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_INTENSITY | BACKGROUND_BLUE);
                            std::cout << buttonText;
                            SetConsoleTextAttribute(hConsole, ConsoleColors::DEFAULT);
                        } else {
                            std::cout << buttonText;
                        }
                    }
                    std::string addButton = "[Add]";
                    int addY = midY + 2;
                    int addX = (CONSOLE_WIDTH - static_cast<int>(addButton.length())) / 2;
                    setCursorPosition(addX, addY);
                    if (editSelection == 5) {
                        Sounds::PlaySelectSound();
                        SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_INTENSITY | BACKGROUND_BLUE);
                        std::cout << addButton;
                        SetConsoleTextAttribute(hConsole, ConsoleColors::DEFAULT);
                    } else {
                        std::cout << addButton;
                    }
                    
                    SetConsoleTextAttribute(hConsole, 8);
                    setCursorPosition(0, CONSOLE_HEIGHT - 3);
                    std::cout << std::string(CONSOLE_WIDTH, '-');
                    std::string tips = "[q] Back  [j/k] Down/Up  [Enter] Select";
                    int tipX = (CONSOLE_WIDTH - static_cast<int>(tips.length())) / 2;
                    setCursorPosition(tipX, CONSOLE_HEIGHT - 2);
                    std::cout << tips;
                    SetConsoleTextAttribute(hConsole, ConsoleColors::DEFAULT);

                    char editKey = _getch();
                    if (editKey == 'j') {
                        editSelection++;
                        if (editSelection > 5) editSelection = 0;
                        Sounds::PlayNavigationSound();
                    } else if (editKey == 'k') {
                        editSelection--;
                        if (editSelection < 0) editSelection = 5;
                        Sounds::PlayNavigationSound();
                    } else if (editKey == '\r') {
                        Sounds::PlaySelectSound();
                        if (editSelection < 5) {
                            std::string input;
                            int fieldY = midY - 4 + editSelection;
                            std::stringstream prefix;
                            prefix << "[" << fieldLabels[editSelection] << ": ";
                            int buttonX = (CONSOLE_WIDTH - static_cast<int>(prefix.str().length() + 10)) / 2;
                            setCursorPosition(buttonX + static_cast<int>(prefix.str().length()), fieldY);
                            std::cout << std::string(10, ' ');
                            setCursorPosition(buttonX + static_cast<int>(prefix.str().length()), fieldY);
                            std::getline(std::cin, input);
                            if (editSelection == 0) {
                                tplName = input;
                            } else {
                                try {
                                    int value = std::stoi(input);
                                    switch(editSelection) {
                                        case 1: cal = value; break;
                                        case 2: carbs = value; break;
                                        case 3: prot = value; break;
                                        case 4: fat = value; break;
                                    }
                                } catch (...) {
                                    // Handle conversion errors.
                                }
                            }
                        } else {
                            // Create the new template and add to the global template list.
                            Food newTpl(tplName, cal, carbs, prot, fat, 0);
                            g_foodTemplates.push_back(newTpl);
                            std::sort(g_foodTemplates.begin(), g_foodTemplates.end(), [](const Food &a, const Food &b) {
                                return a.name < b.name;
                            });
                            break;
                        }
                    }
                    else if (editKey == 'q') {
                        break;
                    }
                }
                localSelection = 2; // Return focus to the template list.
            } else {
                // When selecting an existing template.
                Sounds::PlaySelectSound();
                int templateIndex = localSelection - 2 + templateScrollOffset;
                if (templateIndex >= 0 && templateIndex < static_cast<int>(matches.size())) {
                    Food selectedTemplate = matches[templateIndex];
                    done = true;
                    clearScreen();
                    setCursorPosition((CONSOLE_WIDTH - 30) / 2, midY - 1);
                    std::cout << "Template: " << selectedTemplate.name;
                    setCursorPosition((CONSOLE_WIDTH - 30) / 2, midY + 1);
                    std::cout << "Enter grams to add: ";
                    int grams;
                    std::cin >> grams;
                    Food newFood = selectedTemplate;
                    newFood.grams = grams;
                    newFood.calories = (selectedTemplate.calories * grams) / 100;
                    newFood.carbs = (selectedTemplate.carbs * grams) / 100;
                    newFood.protein = (selectedTemplate.protein * grams) / 100;
                    newFood.fat = (selectedTemplate.fat * grams) / 100;
                    DailyRecord &currentRecord = dataManager.getRecord(currentDate);
                    currentRecord.foods.push_back(newFood);
                    dataManager.saveData();
                    clearScreen();
                    setCursorPosition((CONSOLE_WIDTH - 30) / 2, midY);
                    std::cout << "Template food added.";
                    setCursorPosition((CONSOLE_WIDTH - 30) / 2, midY + 1);
                    std::cout << "Press any key to continue.";
                    (void)_getch();
                    return;
                }
            }
        } else if (key == 'x') {
            // Handle deletion of a template.
            if (localSelection >= 2) {
                Sounds::PlaySelectSound();
                int index = localSelection - 2 + templateScrollOffset;
                if (index >= 0 && index < static_cast<int>(matches.size())) {
                    auto it = std::remove_if(g_foodTemplates.begin(), g_foodTemplates.end(),
                        [&matches, index](const Food &tpl) {
                            return tpl.name == matches[index].name;
                        });
                    g_foodTemplates.erase(it, g_foodTemplates.end());
                    searchTerm = "";
                    localSelection = 0;
                    templateScrollOffset = 0;
                }
            }
        } else if (key == 'q') {
            return;
        }
    }
}

// -----------------------------------------------------------------------------
// Method: handleAddCustomFood
// Purpose: Allows the user to manually add a custom food entry by entering each
//          nutritional value using inline editing.
// -----------------------------------------------------------------------------
void UIManager::handleAddCustomFood() {
    clearScreen();
    int midX = CONSOLE_WIDTH / 2;
    int startY = 8;
    int localSelection = 0;
    bool done = false;
    std::string foodName = "";
    int calories = -1;
    int carbs = -1;
    int protein = -1;
    int fat = -1;
    int grams = -1;
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    
    while (!done) {
        clearScreen();
        
        // Define fields for custom food entry.
        std::string fieldLabels[6] = { "Food Name", "Calories", "Carbs", "Protein", "Fat", "Grams" };
        std::string fieldValues[6];
        fieldValues[0] = (foodName.empty() ? "<empty>" : foodName);
        fieldValues[1] = (calories == -1 ? "0" : std::to_string(calories));
        fieldValues[2] = (carbs == -1 ? "0" : std::to_string(carbs));
        fieldValues[3] = (protein == -1 ? "0" : std::to_string(protein));
        fieldValues[4] = (fat == -1 ? "0" : std::to_string(fat));
        fieldValues[5] = (grams == -1 ? "0" : std::to_string(grams));
        
        // Render input fields.
        for (int i = 0; i < 6; i++) {
            std::stringstream ss;
            ss << "[" << fieldLabels[i] << ": " << fieldValues[i] << "]";
            std::string buttonText = ss.str();
            int buttonX = (CONSOLE_WIDTH - static_cast<int>(buttonText.length())) / 2;
            setCursorPosition(buttonX, startY + i);
            if (localSelection == i) {
                SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_INTENSITY | BACKGROUND_BLUE);
                std::cout << buttonText;
                SetConsoleTextAttribute(hConsole, ConsoleColors::DEFAULT);
            } else {
                SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_INTENSITY);
                std::cout << buttonText;
                SetConsoleTextAttribute(hConsole, ConsoleColors::DEFAULT);
            }
        }
        
        // Render the Add button.
        std::string addButton = "[Add]";
        int addY = startY + 7;
        int addX = (CONSOLE_WIDTH - static_cast<int>(addButton.length())) / 2;
        setCursorPosition(addX, addY);
        if (localSelection == 6) {
            SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_INTENSITY | BACKGROUND_BLUE);
            std::cout << addButton;
            SetConsoleTextAttribute(hConsole, ConsoleColors::DEFAULT);
        } else {
            SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_INTENSITY);
            std::cout << addButton;
            SetConsoleTextAttribute(hConsole, ConsoleColors::DEFAULT);
        }
        
        // Render bottom tips.
        SetConsoleTextAttribute(hConsole, 8);
        setCursorPosition(0, CONSOLE_HEIGHT - 3);
        std::cout << std::string(CONSOLE_WIDTH, '-');
        SetConsoleTextAttribute(hConsole, ConsoleColors::DEFAULT);
        std::string tips = "[q] Back  [j/k] Down/Up  [Enter] Select";
        int tipX = (CONSOLE_WIDTH - static_cast<int>(tips.length())) / 2;
        setCursorPosition(tipX, CONSOLE_HEIGHT - 2);
        SetConsoleTextAttribute(hConsole, 8);
        std::cout << tips;
        SetConsoleTextAttribute(hConsole, ConsoleColors::DEFAULT);
        
        char key = _getch();
        if (key == 'j') {
            localSelection++;
            if (localSelection > 6)
                localSelection = 0;
            Sounds::PlayNavigationSound();
        } else if (key == 'k') {
            localSelection--;
            if (localSelection < 0)
                localSelection = 6;
            Sounds::PlayNavigationSound();
        } else if (key == '\r') {
            Sounds::PlaySelectSound();
            if (localSelection >= 0 && localSelection < 6) {
                std::stringstream prefixStream;
                prefixStream << "[" << fieldLabels[localSelection] << ": ";
                std::string prefix = prefixStream.str();
                std::stringstream fullButtonStream;
                fullButtonStream << "[" << fieldLabels[localSelection] << ": " << fieldValues[localSelection] << "]";
                std::string buttonText = fullButtonStream.str();
                int buttonX = (CONSOLE_WIDTH - static_cast<int>(buttonText.length())) / 2;
                int editX = buttonX + static_cast<int>(prefix.length());
                int editY = startY + localSelection;
                setCursorPosition(editX, editY);
                std::cout << std::string(20, ' ');
                setCursorPosition(editX, editY);
                std::string input;
                std::getline(std::cin, input);
                if (!input.empty()) {
                    if (localSelection == 0) {
                        if (input.size() > maxNameLen)
                            foodName = input.substr(0, maxNameLen);
                        else
                            foodName = input;
                    } else {
                        try {
                            int value = std::stoi(input);
                            switch(localSelection) {
                                case 1: calories = value; break;
                                case 2: carbs = value; break;
                                case 3: protein = value; break;
                                case 4: fat = value; break;
                                case 5: grams = value; break;
                            }
                        } catch (...) {
                            // Ignore conversion failures.
                        }
                    }
                }
            } else if (localSelection == 6) {
                std::string finalName = (foodName.empty() ? "<empty>" : foodName);
                int finalCalories = (calories == -1 ? 0 : calories);
                int finalCarbs = (carbs == -1 ? 0 : carbs);
                int finalProtein = (protein == -1 ? 0 : protein);
                int finalFat = (fat == -1 ? 0 : fat);
                int finalGrams = (grams == -1 ? 0 : grams);
                DailyRecord &record = dataManager.getRecord(currentDate);
                record.foods.push_back(Food(finalName, finalCalories, finalCarbs, finalProtein, finalFat, finalGrams));
                dataManager.saveData();
                return;
            }
        } else if (key == 'q') {
            Sounds::PlaySelectSound();
            return;
        }
    }
}

// -----------------------------------------------------------------------------
// Method: handleStartGoals
// Purpose: When the application runs for the first time, prompt the user to 
//          enter their daily nutritional goals using inline editing.
// -----------------------------------------------------------------------------
void UIManager::handleStartGoals() {
    int midX = CONSOLE_WIDTH / 2;
    int startY = 8;
    int localSelection = 0;  // Fields: Calories, Carbs, Protein, Fat, and then the [Start] button.
    bool done = false;
    std::string fieldLabels[4] = { "Calories", "Carbs", "Protein", "Fat" };
    int fieldValues[4] = { 0, 0, 0, 0 };
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    
    while (!done) {
        clearScreen();
        // Display each nutritional goal field.
        for (int i = 0; i < 4; i++) {
            std::stringstream ss;
            ss << "[" << fieldLabels[i] << ": " << fieldValues[i] << "]";
            std::string buttonText = ss.str();
            int buttonX = (CONSOLE_WIDTH - static_cast<int>(buttonText.length())) / 2;
            setCursorPosition(buttonX, startY + i);
            if (localSelection == i) {
                SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_INTENSITY | BACKGROUND_BLUE);
                std::cout << buttonText;
                SetConsoleTextAttribute(hConsole, ConsoleColors::DEFAULT);
            } else {
                SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_INTENSITY);
                std::cout << buttonText;
                SetConsoleTextAttribute(hConsole, ConsoleColors::DEFAULT);
            }
        }
        // Render the Start button.
        std::string startButton = "[Start]";
        int buttonY = startY + 5;
        int buttonX = (CONSOLE_WIDTH - static_cast<int>(startButton.length())) / 2;
        setCursorPosition(buttonX, buttonY);
        if (localSelection == 4) {
            SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_INTENSITY | BACKGROUND_BLUE);
            std::cout << startButton;
            SetConsoleTextAttribute(hConsole, ConsoleColors::DEFAULT);
        } else {
            SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_INTENSITY);
            std::cout << startButton;
            SetConsoleTextAttribute(hConsole, ConsoleColors::DEFAULT);
        }
        
        // Display tips.
        SetConsoleTextAttribute(hConsole, 8);
        setCursorPosition(0, CONSOLE_HEIGHT - 3);
        std::cout << std::string(CONSOLE_WIDTH, '-');
        SetConsoleTextAttribute(hConsole, ConsoleColors::DEFAULT);
        std::string tips = "[q] Cancel  [j/k] Down/Up  [Enter] Select";
        int tipX = (CONSOLE_WIDTH - static_cast<int>(tips.length())) / 2;
        setCursorPosition(tipX, CONSOLE_HEIGHT - 2);
        std::cout << tips;
        SetConsoleTextAttribute(hConsole, ConsoleColors::DEFAULT);

        char key = _getch();
        
        if (key == 'j') {
            localSelection++;
            if (localSelection > 4)
                localSelection = 0;
            Sounds::PlayNavigationSound();
        } else if (key == 'k') {
            localSelection--;
            if (localSelection < 0)
                localSelection = 4;
            Sounds::PlayNavigationSound();
        } else if (key == '\r') {
            Sounds::PlaySelectSound();
            if (localSelection < 4) {
                std::stringstream prefixStream;
                prefixStream << "[" << fieldLabels[localSelection] << ": ";
                std::string prefix = prefixStream.str();
                std::stringstream fullButtonStream;
                fullButtonStream << "[" << fieldLabels[localSelection] << ": " << fieldValues[localSelection] << "]";
                std::string buttonText = fullButtonStream.str();
                int buttonX = (CONSOLE_WIDTH - static_cast<int>(buttonText.length())) / 2;
                int editX = buttonX + static_cast<int>(prefix.length());
                int editY = startY + localSelection;
                setCursorPosition(editX, editY);
                std::cout << std::string(10, ' ');
                setCursorPosition(editX, editY);
                std::string input;
                std::getline(std::cin, input);
                if (!input.empty()) {
                    try {
                        int value = std::stoi(input);
                        fieldValues[localSelection] = value;
                    } catch (...) {
                        fieldValues[localSelection] = 0;
                    }
                } else {
                    fieldValues[localSelection] = 0;
                }
            } else {
                // Set the new nutritional goals and save data.
                DailyGoals newGoals;
                newGoals.calories = fieldValues[0];
                newGoals.carbs = fieldValues[1];
                newGoals.protein = fieldValues[2];
                newGoals.fat = fieldValues[3];
                dataManager.setDailyGoals(newGoals);
                dataManager.saveData();
                done = true;
            }
        } else if (key == 'q') {
            Sounds::PlaySelectSound();
            done = true;
        }
    }
}

// -----------------------------------------------------------------------------
// Method: handleResetGoals
// Purpose: Allows the user to reset current nutritional goals via an inline editing screen.
// -----------------------------------------------------------------------------
void UIManager::handleResetGoals() {
    int midX = CONSOLE_WIDTH / 2;
    int startY = 8;
    int localSelection = 0;  // Fields: Calories, Carbs, Protein, Fat, then Update button.
    bool done = false;
    DailyGoals currentGoals = dataManager.getDailyGoals();
    std::string fieldLabels[4] = { "Calories", "Carbs", "Protein", "Fat" };
    int fieldValues[4] = { currentGoals.calories, currentGoals.carbs, currentGoals.protein, currentGoals.fat };
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    
    while (!done) {
        clearScreen();
        // Render each field with the current goal values.
        for (int i = 0; i < 4; i++) {
            std::stringstream ss;
            ss << "[" << fieldLabels[i] << ": " << fieldValues[i] << "]";
            std::string buttonText = ss.str();
            int buttonX = (CONSOLE_WIDTH - static_cast<int>(buttonText.length())) / 2;
            setCursorPosition(buttonX, startY + i);
            if (localSelection == i) {
                SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_INTENSITY | BACKGROUND_BLUE);
                std::cout << buttonText;
                SetConsoleTextAttribute(hConsole, ConsoleColors::DEFAULT);
            } else {
                SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_INTENSITY);
                std::cout << buttonText;
                SetConsoleTextAttribute(hConsole, ConsoleColors::DEFAULT);
            }
        }
        // Render the Update button.
        std::string updateButton = "[Update]";
        int updateY = startY + 5;
        int updateX = (CONSOLE_WIDTH - static_cast<int>(updateButton.length())) / 2;
        setCursorPosition(updateX, updateY);
        if (localSelection == 4) {
            SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_INTENSITY | BACKGROUND_BLUE);
            std::cout << updateButton;
            SetConsoleTextAttribute(hConsole, ConsoleColors::DEFAULT);
        } else {
            SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_INTENSITY);
            std::cout << updateButton;
            SetConsoleTextAttribute(hConsole, ConsoleColors::DEFAULT);
        }
        
        // Render bottom tips.
        SetConsoleTextAttribute(hConsole, 8);
        setCursorPosition(0, CONSOLE_HEIGHT - 3);
        std::cout << std::string(CONSOLE_WIDTH, '-');
        SetConsoleTextAttribute(hConsole, ConsoleColors::DEFAULT);
        std::string tips = "[q] Back  [j/k] Down/Up  [Enter] Select";
        int tipX = (CONSOLE_WIDTH - static_cast<int>(tips.length())) / 2;
        setCursorPosition(tipX, CONSOLE_HEIGHT - 2);
        SetConsoleTextAttribute(hConsole, 8);
        std::cout << tips;
        SetConsoleTextAttribute(hConsole, ConsoleColors::DEFAULT);
        
        char key = _getch();
        if (key == 'j') {
            localSelection++;
            if (localSelection > 4)
                localSelection = 0;
            Sounds::PlayNavigationSound();
        } else if (key == 'k') {
            localSelection--;
            if (localSelection < 0)
                localSelection = 4;
            Sounds::PlayNavigationSound();
        } else if (key == '\r') {
            Sounds::PlaySelectSound();
            if (localSelection < 4) {
                std::stringstream prefixStream;
                prefixStream << "[" << fieldLabels[localSelection] << ": ";
                std::string prefix = prefixStream.str();
                std::stringstream fullButtonStream;
                fullButtonStream << "[" << fieldLabels[localSelection] << ": " << fieldValues[localSelection] << "]";
                std::string buttonText = fullButtonStream.str();
                int buttonX = (CONSOLE_WIDTH - static_cast<int>(buttonText.length())) / 2;
                int editX = buttonX + static_cast<int>(prefix.length());
                int editY = startY + localSelection;
                setCursorPosition(editX, editY);
                std::cout << std::string(10, ' ');
                setCursorPosition(editX, editY);
                std::string input;
                std::getline(std::cin, input);
                if (!input.empty()) {
                    try {
                        int value = std::stoi(input);
                        fieldValues[localSelection] = value;
                    } catch (...) {
                        // On conversion error, keep existing value.
                    }
                }
            } else {
                DailyGoals newGoals;
                newGoals.calories = fieldValues[0];
                newGoals.carbs = fieldValues[1];
                newGoals.protein = fieldValues[2];
                newGoals.fat = fieldValues[3];
                dataManager.setDailyGoals(newGoals);
                dataManager.saveData();
                done = true;
            }
        } else if (key == 'q') {
            Sounds::PlaySelectSound();
            return;
        }
    }
}

// -----------------------------------------------------------------------------
// Method: renderCalendar
// Purpose: Displays a calendar view for the user to choose a specific date.
// -----------------------------------------------------------------------------
void UIManager::renderCalendar() {
    clearScreen();
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

    int day, month, year;
    sscanf_s(currentDate.c_str(), "%d/%d/%d", &day, &month, &year);

    // Calculate the first day of the month.
    tm firstDay = {};
    firstDay.tm_mday = 1;
    firstDay.tm_mon = month - 1;
    firstDay.tm_year = year - 1900;
    mktime(&firstDay);
    int startWeekday = firstDay.tm_wday;

    // Calculate the number of days in the month.
    tm nextMonth = firstDay;
    nextMonth.tm_mon += 1;
    mktime(&nextMonth);
    time_t firstOfNextMonth = mktime(&nextMonth);
    time_t lastDayTime = firstOfNextMonth - 86400;
    tm lastDay;
    localtime_s(&lastDay, &lastDayTime);
    int daysInMonth = lastDay.tm_mday;

    int gridRows = (startWeekday + daysInMonth + 6) / 7;
    int calendarBlockHeight = 2 + gridRows;
    int verticalOffset = (CONSOLE_HEIGHT - calendarBlockHeight) / 2;
    if (verticalOffset < 0)
        verticalOffset = 0;

    // Render the calendar header with month and year.
    SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
    static const char* monthNames[] = {"January", "February", "March", "April", "May", "June",
                                        "July", "August", "September", "October", "November", "December"};
    std::string header = std::string(monthNames[month-1]) + " " + std::to_string(year);
    int headerStartX = (CONSOLE_WIDTH - static_cast<int>(header.length())) / 2;
    setCursorPosition(headerStartX, verticalOffset);
    std::cout << header;
    SetConsoleTextAttribute(hConsole, ConsoleColors::DEFAULT);

    // Render the weekday names.
    SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
    std::string daysHeader = "Su Mo Tu We Th Fr Sa";
    int daysHeaderStartX = (CONSOLE_WIDTH - static_cast<int>(daysHeader.length())) / 2;
    setCursorPosition(daysHeaderStartX, verticalOffset + 1);
    std::cout << daysHeader;
    SetConsoleTextAttribute(hConsole, ConsoleColors::DEFAULT);

    // Render the days grid.
    int gridStartRow = verticalOffset + 2;
    int currentRow = gridStartRow;
    int currentCol = startWeekday;
    int colStart = daysHeaderStartX;
    for (int d = 1; d <= daysInMonth; d++) {
        int posX = colStart + currentCol * 3;
        int posY = currentRow;
        if (d == selectedCalendarDay) {
            SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | BACKGROUND_BLUE);
        }
        setCursorPosition(posX, posY);
        if (d < 10)
            std::cout << "  " << d;
        else
            std::cout << " " << d;
        SetConsoleTextAttribute(hConsole, ConsoleColors::DEFAULT);
        currentCol++;
        if (currentCol > 6) {
            currentCol = 0;
            currentRow++;
        }
    }

    // Render bottom tips.
    SetConsoleTextAttribute(hConsole, 8);
    setCursorPosition(0, CONSOLE_HEIGHT - 3);
    std::cout << std::string(CONSOLE_WIDTH, '-');
    SetConsoleTextAttribute(hConsole, ConsoleColors::DEFAULT);

    std::string calendarTips = "[q] Back  [j/k] Down/Up  [h/l] Left/Right  [b/w] Previous/Next  [Enter] Select";
    int tipStartX = (CONSOLE_WIDTH - static_cast<int>(calendarTips.length())) / 2;
    setCursorPosition(tipStartX, CONSOLE_HEIGHT - 2);
    SetConsoleTextAttribute(hConsole, 8);
    std::cout << calendarTips;
    SetConsoleTextAttribute(hConsole, ConsoleColors::DEFAULT);
}

// -----------------------------------------------------------------------------
// Method: processCalendarInput
// Purpose: Process key events in the calendar view to allow date navigation.
// -----------------------------------------------------------------------------
void UIManager::processCalendarInput(char key) {
    int day, month, year;
    sscanf_s(currentDate.c_str(), "%d/%d/%d", &day, &month, &year);
    tm firstDay = {};
    firstDay.tm_mday = 1;
    firstDay.tm_mon = month - 1;
    firstDay.tm_year = year - 1900;
    mktime(&firstDay);
    tm nextMonth = firstDay;
    nextMonth.tm_mon += 1;
    mktime(&nextMonth);
    time_t firstOfNextMonth = mktime(&nextMonth);
    time_t lastDayTime = firstOfNextMonth - 86400;
    tm lastDay;
    localtime_s(&lastDay, &lastDayTime);
    int daysInMonth = lastDay.tm_mday;

    int startWeekday = firstDay.tm_wday;
    int col = (startWeekday + selectedCalendarDay - 1) % 7;

    // Navigate between months.
    if (key == 'b') {
        month--;
        if (month < 1) { month = 12; year--; }
        selectedCalendarDay = 1;
        char buffer[11];
        sprintf_s(buffer, "%02d/%02d/%04d", day, month, year);
        currentDate = buffer;
        Sounds::PlayPageSwitchSound();
    }
    else if (key == 'w') {
        month++;
        if (month > 12) { month = 1; year++; }
        selectedCalendarDay = 1;
        char buffer[11];
        sprintf_s(buffer, "%02d/%02d/%04d", day, month, year);
        currentDate = buffer;
        Sounds::PlayPageSwitchSound();
    }
    else if (key == 'q') {
        Sounds::PlaySelectSound();
        currentDate = calendarOriginalDate;  // Restore original date if user cancels.
        currentState = STATE_MAIN_MENU;
    }
    else if (key == 'h') {
        if (selectedCalendarDay > 1 && col > 0) {
            selectedCalendarDay--;
            Sounds::PlayNavigationSound();
        }
    }
    else if (key == 'l') {
        if (col < 6 && selectedCalendarDay < daysInMonth) {
            selectedCalendarDay++;
            Sounds::PlayNavigationSound();
        }
    }
    else if (key == 'j') {
        if (selectedCalendarDay + 7 <= daysInMonth) {
            selectedCalendarDay += 7;
            Sounds::PlayNavigationSound();
        }
    }
    else if (key == 'k') {
        if (selectedCalendarDay - 7 >= 1) {
            selectedCalendarDay -= 7;
            Sounds::PlayNavigationSound();
        }
    }
    else if (key == '\r') {
        // Set current date to selected date from the calendar.
        char buffer[11];
        sprintf_s(buffer, "%02d/%02d/%04d", selectedCalendarDay, month, year);
        currentDate = buffer;
        currentState = STATE_MAIN_MENU;
        Sounds::PlaySelectSound();
    }
}
