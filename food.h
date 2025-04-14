#ifndef FOOD_H
#define FOOD_H

// -----------------------------------------------------------------------------
// File: food.h
// Purpose: Define the Food structure that encapsulates details of each food item.
//          This is used within daily records to represent individual food entries.
// -----------------------------------------------------------------------------

#include <string>

// -----------------------------------------------------------------------------
// Structure: Food
// Purpose: Contains all nutritional information about a given food along with 
//          its serving size in grams.
// -----------------------------------------------------------------------------
struct Food {
    std::string name;  // Name of the food item (e.g., "Apple", "Chicken Breast")
    int calories;      // Energy provided by the food, measured in kilocalories
    int carbs;         // Carbohydrates in grams
    int protein;       // Protein in grams
    int fat;           // Fat in grams
    int grams;         // Portion size in grams

    // Default constructor initializes fields to default values.
    Food() : name(""), calories(0), carbs(0), protein(0), fat(0), grams(0) {}

    // Parameterized constructor allows instant initialization of all values.
    Food(const std::string &n, int cal, int c, int p, int f, int g)
        : name(n), calories(cal), carbs(c), protein(p), fat(f), grams(g) {}
};

#endif // FOOD_H
