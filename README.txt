Calorie Calculator Project
==========================

Overview:
---------
The Calorie Calculator is a C++ console application designed to track daily nutritional intake.
It allows users to set personal nutritional goals and log food entries for each day. The project
features a user-friendly console interface, calendar navigation, and inline editing for food entries
and nutritional goals.

Project Structure:
------------------
- constants.h       : Defines global constants, console colors, and sound functions.
- data_manager.h/cpp: Manages persistent data (daily goals and food records).
- ui_manager.h/cpp  : Contains the user interface logic, including rendering and input handling.
- food.h            : Defines the Food structure for individual food entries.
- main.cpp          : Entry point which initializes the DataManager and UIManager, and starts the application.

Features:
---------
- Set and reset daily nutritional goals (calories, carbs, protein, fat).
- Log individual food entries with detailed nutritional information.
- Use food templates to quickly add common food items.
- Navigate through records using a calendar interface.
- Detailed console UI with color-coded navigation and real-time feedback.

Future Enhancements:
--------------------
- Integration with a database for cloud storage of data.
- Enhanced reporting and graphical visualization of nutritional intake.
- Extended user settings and custom templates.

Instructions:
-------------
1. Compile the project using your preferred C++ compiler on a Windows system.
2. Run the executable and follow on-screen prompts to input nutritional goals and log food entries.
3. Data is persisted in the file "calorie_data.txt" in the project directory.