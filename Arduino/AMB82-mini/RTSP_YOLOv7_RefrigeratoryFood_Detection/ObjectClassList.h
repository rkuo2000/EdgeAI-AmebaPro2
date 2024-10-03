#ifndef __OBJECTCLASSLIST_H__
#define __OBJECTCLASSLIST_H__

struct ObjectDetectionItem {
    uint8_t index;
    const char* objectName;
    uint8_t filter;
};

// List of objects the pre-trained model is capable of recognizing
// Index number is fixed and hard-coded from training
// Set the filter value to 0 to ignore any recognized objects
ObjectDetectionItem itemList[76] = {
    {0, "Apple",		1},
    {1, "Banana",		1},
    {2, "Cucumber",		1},
    {3, "Orange",		1},
    {4, "Tomato",		1},
    {5, "apple",		1},
    {6, "asparagus",		1},
    {7, "avocado",		1},
    {8, "banana",		1},
    {9, "beef",			1},
    {10, "bell_pepper",		1},
    {11, "bento",		1},
    {12, "blueberries",		1},
    {13, "bread",		1},
    {14, "broccoli",		1},
    {15, "butter",		1},
    {16, "carrot",		1},
    {17, "cauliflower",		1},
    {18, "cheese",		1},
    {19, "chicken",		1},
    {20, "chicken_breast",	1},
    {21, "chocolate",		1},
    {22, "coffee",		1},
    {23, "corn",		1},
    {24, "cucumber",		1},
    {25, "egg",			1},
    {26, "eggs",		1},
    {27, "energy_drink",	1},
    {28, "fish",		1},
    {29, "flour",		1},
    {30, "garlic",		1},
    {31, "goat_cheese",		1},
    {32, "grated_cheese",	1},
    {33, "green_beans",		1},
    {34, "ground_beef",		1},
    {35, "guacamole",		1},
    {36, "ham",			1},
    {37, "heavy_cream",		1},
    {38, "humus",		1},
    {39, "ketchup",		1},
    {40, "leek",		1},
    {41, "lemon",		1},
    {42, "lettuce",		1},
    {43, "lime",		1},
    {44, "mango",		1},
    {45, "marmelade",		1},
    {46, "mayonaise",		1},
    {47, "milk",		1},
    {48, "mushrooms",		1},
    {49, "mustard",		1},
    {50, "nuts",		1},
    {51, "onion",		1},
    {52, "pak_choi",		1},
    {53, "pear",		1},
    {54, "pineapple",		1},
    {55, "potato",		1},
    {56, "potatoes",		1},
    {57, "pudding",		1},
    {58, "rice_ball",		1},
    {59, "salad",		1},
    {60, "sandwich",		1},
    {61, "sausage",		1},
    {62, "shrimp",		1},
    {63, "smoothie",		1},
    {64, "spinach",		1},
    {65, "spring_onion",	1},
    {66, "strawberries",	1},
    {67, "sugar",		1},
    {68, "sweet_potato",	1},
    {69, "tea_a",		1},
    {70, "tea_i",		1},
    {71, "tomato",		1},
    {72, "tomato_sauce",	1},
    {73, "tortillas",		1},
    {74, "turkey",		1},
    {75, "yogurt",		1},
};

#endif
