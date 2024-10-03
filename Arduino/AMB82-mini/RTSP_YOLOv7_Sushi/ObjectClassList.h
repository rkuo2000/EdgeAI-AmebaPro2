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
ObjectDetectionItem itemList[26] = {
    {0, "Akagai",		1},
    {1, "Awabi",		1},
    {2, "Chutoro",		1}, 
    {3, "Ebi",			1},       
    {4, "Engawa",		1},
    {5, "Engawaburi",		1},
    {6, "Gunkan",		1},  
    {7, "Hamachi",		1},
    {8, "Hokkigai",     1},
    {9, "Hotate",      1},
    {10, "Ika",         1},  
    {11, "Ikura",       1},
    {12, "Iwashi",      1},
    {13, "Kanpachi",     1},
    {14, "Kappamaki",    1},
    {15, "Kobsshira",    1},
    {16, "Maguro",       1},
    {17, "Negitorogunkan",1},
    {18, "Negitoromaki",  1},
    {19, "Otoro",        1},
    {20, "Salmon",       1},
    {21, "Tako",         1},
    {22, "Tmago",        1},
    {23, "Tekkamaki",    1},
    {24, "Torigai",      1},
    {25, "Uni",          1}
};

#endif
