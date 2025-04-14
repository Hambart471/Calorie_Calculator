#ifndef CONSTANTS_H
#define CONSTANTS_H

// -----------------------------------------------------------------------------
// File: constants.h
// Purpose: Define all global constants, configuration settings, and simple 
//          helper functions for console colors and sounds. This file is 
//          central to managing settings used throughout the Calorie Calculator 
//          application.
// -----------------------------------------------------------------------------

#include <windows.h>    // Windows-specific API functions (e.g., Beep, console control)
#include <string>       // STL string class

// -----------------------------------------------------------------------------
// Application Settings
// -----------------------------------------------------------------------------

// Console display dimensions used for formatting the UI (80 columns x 24 rows).
const int CONSOLE_WIDTH = 80;
const int CONSOLE_HEIGHT = 24;

// File path for persistent storage – the calorie data is saved and loaded from this file.
const std::string DATA_FILE = "calorie_data.txt";

// -----------------------------------------------------------------------------
// Console Color Definitions
// -----------------------------------------------------------------------------

// Namespace for simple color definitions for console output.
// Uses WinAPI color codes to create various foreground and background color combinations.
namespace ConsoleColors {
    // Default text color: combination of red, green and blue for white/light gray text.
    const WORD DEFAULT = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
    // Highlight background: blue background
    const WORD HIGHLIGHT = BACKGROUND_BLUE;
    // Button text color: intense green for buttons.
    const WORD BUTTON = FOREGROUND_GREEN | FOREGROUND_INTENSITY;
}

// -----------------------------------------------------------------------------
// Sound Functions
// -----------------------------------------------------------------------------

// Namespace for inline sound functions to provide audio feedback for various actions.
// Uses the Windows Beep API to play specific frequencies for sound effects.
namespace Sounds {
    // Play a tone for page switching actions.
    inline void PlayPageSwitchSound() { Beep(600, 150); }
    // Play a tone when navigating the menu.
    inline void PlayNavigationSound() { Beep(700, 150); }
    // Play a tone when a selection is made.
    inline void PlaySelectSound() { Beep(800, 150); }
}

#endif // CONSTANTS_H
