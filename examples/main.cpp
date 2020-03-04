/**
 * @brief initools example program
 * 
 * @file main.cpp
 */

/* Includes -------------------------------------------- */
#include "INI.hpp"

/* C++ system */
#include <iostream>
#include <string>

/* C system */
#include <cstring>

/* Defines --------------------------------------------- */

/* Notes ----------------------------------------------- */

/* Variable declaration -------------------------------- */

/* Type definitions ------------------------------------ */

/* Support functions ----------------------------------- */
static void printUsage(const char * const pProgName)
{
    std::cout << "[USAGE] %s" << pProgName << std::endl;
    std::cout << "        <arg1> : INI file" << std::endl;
}

/* ----------------------------------------------------- */
/* Main tests ------------------------------------------ */
/* ----------------------------------------------------- */
int main(const int argc, const char * const * const argv) {
    if ((argc < 2) || (std::strcmp(argv[1U], "--help") == 0)) {
        printUsage(argv[0]);
        return EXIT_FAILURE;
    }

    const std::string lINIFile = std::string(argv[1U]);

    /* Create an INI instance */
    INI *lINI = nullptr;
    try {
        std::cout << "[DEBUG] Opening INI file " << argv[1U] << std::endl;
        lINI = new INI(lINIFile);
        std::cerr << "[ERROR] Successfully parsed INI file " << argv[1U] << " !" << std::endl;
    } catch (const std::exception &e) {
        std::cerr << "[ERROR] Failed to parse INI file " << argv[1U] << " !" << std::endl;
        return EXIT_FAILURE;
    }

    /* Generating the same INI file */
    std::string lCopyINIFile = lINIFile.substr(0U, lINIFile.find_last_of('.')) + ".copy.ini";
    std::cout << "[DEBUG] lCopyINIFile = " << lCopyINIFile << std::endl;
    if(0 > lINI->generateFile(lCopyINIFile)) {
        std::cerr << "[ERROR] Failed to generate copy if ini file ! " << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
