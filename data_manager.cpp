#include "data_manager.h"
#include "constants.h"  // Provides DATA_FILE and other constant definitions
#include <fstream>      // For file I/O operations
#include <sstream>      // For string stream processing
#include <iostream>     // For standard I/O (e.g., error output)
#include <algorithm>    // For standard algorithms like std::replace
#include <cstdio>       // For formatted input/output

// -----------------------------------------------------------------------------
// Constructor: DataManager
// Purpose: Initialize default daily nutritional goals and set firstRun flag.
// -----------------------------------------------------------------------------
DataManager::DataManager() : firstRun(false) {
    // Set default nutritional goals in case no data exists from a previous run.
    dailyGoals.calories = 2000;
    dailyGoals.carbs = 250;
    dailyGoals.protein = 150;
    dailyGoals.fat = 70;
}

// -----------------------------------------------------------------------------
// Destructor: DataManager
// Purpose: Clean up any allocated resources (none needed here).
// -----------------------------------------------------------------------------
DataManager::~DataManager() {
    // No dynamic memory to cleanup.
}

// -----------------------------------------------------------------------------
// Method: isFirstRun
// Purpose: Returns true if no data file was found on initialization.
// -----------------------------------------------------------------------------
bool DataManager::isFirstRun() const {
    return firstRun;
}

// -----------------------------------------------------------------------------
// Getter: getDailyGoals
// Purpose: Returns the current daily nutritional goals.
// -----------------------------------------------------------------------------
DailyGoals DataManager::getDailyGoals() const {
    return dailyGoals;
}

// -----------------------------------------------------------------------------
// Setter: setDailyGoals
// Purpose: Updates the current daily nutritional goals.
// -----------------------------------------------------------------------------
void DataManager::setDailyGoals(const DailyGoals &goals) {
    dailyGoals = goals;
}

// -----------------------------------------------------------------------------
// Method: getRecord
// Purpose: Retrieves or creates a DailyRecord for the specified date.
// -----------------------------------------------------------------------------
DailyRecord &DataManager::getRecord(const std::string &date) {
    // Loop through existing records to find one matching the date.
    for (auto &record : records) {
        if (record.date == date) {
            return record;
        }
    }
    // If not found, create a new record and add it to the records vector.
    records.push_back(DailyRecord(date));
    return records.back();
}

// -----------------------------------------------------------------------------
// Method: getAllRecords
// Purpose: Provides a constant reference to the entire set of daily records.
// -----------------------------------------------------------------------------
const std::vector<DailyRecord> &DataManager::getAllRecords() const {
    return records;
}

// -----------------------------------------------------------------------------
// Method: loadData
// Purpose: Reads stored data (goals and food entries) from the designated file.
// -----------------------------------------------------------------------------
bool DataManager::loadData() {
    std::ifstream inFile(DATA_FILE);
    if (!inFile.is_open()) {
        // File not found implies the application is being run for the first time.
        firstRun = true;
        return false;
    }
    std::string line;
    DailyRecord *currentRecord = nullptr;
    // Read file line by line and parse different types of data entries.
    while (std::getline(inFile, line)) {
        if (line.find("DAILY_GOALS:") == 0) {
            // Format: DAILY_GOALS: calories,carbs,protein,fat
            std::string goalsStr = line.substr(12);  // Extract substring after label
            // Replace commas with spaces to facilitate extraction.
            std::replace(goalsStr.begin(), goalsStr.end(), ',', ' ');
            std::istringstream iss(goalsStr);
            iss >> dailyGoals.calories >> dailyGoals.carbs >> dailyGoals.protein >> dailyGoals.fat;
        }
        else if (line.find("DATE:") == 0) {
            // Each new date starts a new daily record.
            std::string dateStr = line.substr(5); // Extract date after "DATE:"
            dateStr.erase(0, dateStr.find_first_not_of(" \t"));  // Trim leading whitespace
            // Get (or create) the record corresponding to this date.
            currentRecord = &getRecord(dateStr);
        }
        else if (line.find("FOOD:") == 0) {
            // Food entry lines, only processed if a valid DailyRecord is active.
            if (currentRecord) {
                std::string foodStr = line.substr(5); // Remove "FOOD:" label
                std::istringstream iss(foodStr);
                std::string token;
                Food food;
                // Tokenize the food data using the '|' delimiter.
                if (std::getline(iss, token, '|'))
                    food.name = token;
                if (std::getline(iss, token, '|'))
                    food.calories = std::stoi(token);
                if (std::getline(iss, token, '|'))
                    food.carbs = std::stoi(token);
                if (std::getline(iss, token, '|'))
                    food.protein = std::stoi(token);
                if (std::getline(iss, token, '|'))
                    food.fat = std::stoi(token);
                if (std::getline(iss, token, '|'))
                    food.grams = std::stoi(token);
                // Add the food item to the current day's record.
                currentRecord->foods.push_back(food);
            }
        }
    }
    inFile.close();
    return true;
}

// -----------------------------------------------------------------------------
// Method: saveData
// Purpose: Writes current daily goals and all daily records to the persistent file.
// -----------------------------------------------------------------------------
bool DataManager::saveData() {
    std::ofstream outFile(DATA_FILE);
    if (!outFile.is_open()) {
        std::cerr << "Error saving data!" << std::endl;
        return false;
    }
    // Write the nutritional goals first.
    outFile << "DAILY_GOALS: " << dailyGoals.calories << "," 
            << dailyGoals.carbs << "," << dailyGoals.protein << "," 
            << dailyGoals.fat << std::endl;
    // Iterate through each day’s record.
    for (const auto &record : records) {
        outFile << "DATE: " << record.date << std::endl;
        // For every food item in the daily record, write the details in a delimited format.
        for (const auto &food : record.foods) {
            outFile << "FOOD: " << food.name << "|" << food.calories << "|"
                    << food.carbs << "|" << food.protein << "|" 
                    << food.fat << "|" << food.grams << std::endl;
        }
    }
    outFile.close();
    return true;
}