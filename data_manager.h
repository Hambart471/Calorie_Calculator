#ifndef DATA_MANAGER_H
#define DATA_MANAGER_H

// -----------------------------------------------------------------------------
// File: data_manager.h
// Purpose: Declare the structures and the DataManager class for handling all
//          persistent data interactions for the Calorie Calculator.
// -----------------------------------------------------------------------------

#include <string>
#include <vector>
#include "food.h"  // Include definition for the Food structure

// -----------------------------------------------------------------------------
// Structure: DailyGoals
// Purpose: Store the user's daily nutritional goals (calories, carbs, protein, fat).
// -----------------------------------------------------------------------------
struct DailyGoals {
    int calories;   // Total daily calories
    int carbs;      // Total daily carbohydrates (in grams)
    int protein;    // Total daily protein (in grams)
    int fat;        // Total daily fat (in grams)
};

// -----------------------------------------------------------------------------
// Structure: DailyRecord
// Purpose: Represent a single day’s record including date and all food entries.
// -----------------------------------------------------------------------------
struct DailyRecord {
    std::string date;            // Date string in "DD/MM/YYYY" format
    std::vector<Food> foods;     // List to store multiple food entries for the day

    // Constructor initializes a new record with the specified date.
    DailyRecord(const std::string &d) : date(d) {}
};

// -----------------------------------------------------------------------------
// Class: DataManager
// Purpose: Encapsulate all data-related operations such as loading/saving data,
//          accessing daily goals and records, and tracking first run state.
// -----------------------------------------------------------------------------
class DataManager {
public:
    DataManager();
    ~DataManager();

    // Attempts to load data from the persistent file.
    // Returns true if data is successfully loaded.
    bool loadData();

    // Saves all current data (goals and records) to the persistent file.
    // Returns true if saving was successful.
    bool saveData();

    // Determines whether this is the first run of the application by checking file existence.
    bool isFirstRun() const;

    // Get and update the user’s daily nutritional goals.
    DailyGoals getDailyGoals() const;
    void setDailyGoals(const DailyGoals &goals);

    // Retrieves the record for the given date. If it does not exist, creates a new record.
    DailyRecord &getRecord(const std::string &date);

    // Provides a constant reference to all stored daily records.
    const std::vector<DailyRecord> &getAllRecords() const;

private:
    DailyGoals dailyGoals;              // User's nutritional goals to be achieved in a day
    std::vector<DailyRecord> records;   // Container holding records for multiple days
    bool firstRun;                      // Flag: true if data file not found, i.e., first run

    // Helper function to parse a single line from the data file and update internal structures.
    void parseDataLine(const std::string &line);
};

#endif // DATA_MANAGER_H
