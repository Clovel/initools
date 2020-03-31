/**
 * @brief INI class implementation
 * 
 * @file INI.cpp
 */

/* Includes -------------------------------------------- */
#include "INI.hpp"

/* C++ System */
#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <map>
#include <vector>
#include <algorithm>

/* C System */
#include <cstdlib>
#include <cstdint>
#include <climits>

/* Defines --------------------------------------------- */

/* Type definitions ------------------------------------ */

/* Helper functions ------------------------------------ */
static void split(const std::string &pStr, const char pDelim, std::vector<std::string> &pWords) {
    size_t lPos = 0U, lOldPos = 0U;
    while(std::string::npos != lPos) {
        std::string lSub;

        /* Save the old position in the string */
        lOldPos = lPos;

        /* Find the position of the next '\n' */
        lPos = pStr.find(pDelim, lPos);

        /* Extract the substring (lSub) */
        lSub = pStr.substr(lOldPos, lPos - lOldPos);

        if(std::string::npos != lPos) {
            ++lPos;
        }

        if(lSub.empty()) {
            break;
        }

        pWords.push_back(lSub);
    }
}

static std::vector<std::string> split(const std::string &pStr, char pDelim) {
    std::vector<std::string> lWords;

    split(pStr, pDelim, lWords);

    return lWords;
}

static std::string getStrAfterDelim(const std::string &pStr, char pDelim) {
    size_t lDelimPos = pStr.find_last_of(pDelim);
    size_t lEOLPos   = pStr.find('\n');

    /* Check if we found the delimiter */
    if(std::string::npos == lDelimPos) {
        /* Delimiter is not there, simply return the argument */
        return pStr;
    }

    if(std::string::npos != lEOLPos) {
        ++lEOLPos;
    }

    std::string lSub = pStr.substr(lDelimPos, lEOLPos - lDelimPos);

    return lSub;
}

static std::string getStrBeforeDelim(const std::string &pStr, char pDelim) {
    return pStr.substr(0U, pStr.find_first_of(pDelim));
}

static void removeChar(std::string &pStr, const char pChar) {
    size_t lPos = 0U;

    while(std::string::npos != lPos) {
        if(std::string::npos == (lPos = pStr.find(pChar))) {
            break;
        }
        pStr = pStr.substr(0U, lPos) + pStr.substr(lPos + 1U);
    }
}

static void removeFirstChar(std::string &pStr, const char pChar) {
    if(0U == pStr.find(pChar)) {
        pStr = pStr.substr(1U);
    }
}

static void removeTrailingChar(std::string &pStr, const char pChar) {
    if(pChar == pStr.at(pStr.size() - 1)) {
        pStr = pStr.substr(0U, pStr.size() - 1);
    }
}

static void frontTrim(std::string &pStr, const std::string &pChars) {
    size_t lStart = pStr.find_first_not_of(pChars);
    pStr = (lStart == std::string::npos) ? pStr : pStr.substr(lStart);
}

static void backTrim(std::string &pStr, const std::string &pChars) {
    size_t lEnd = pStr.find_last_not_of(pChars);
    pStr = (lEnd == std::string::npos) ? pStr : pStr.substr(0U, lEnd + 1U);
}

static void removeChar(std::vector<std::string> &pStrs, const char pChar) {
    for(size_t i = 0U; i < pStrs.size(); ++i) {
        removeChar(pStrs[i], pChar);
    }
}

/* Private helper functions ---------------------------- */
bool INI::sectionExists(const std::string &pSection) const {
    /* Does this section exist ? */
    return (mSections.end() != mSections.find(pSection));
}

bool INI::keyExists(const std::string &pKey, const std::string &pSection) const {
    /* Does the section and key exist ? 
     * We also check the section here because otherwise
     * the "at" method will throw a out_of_range
     * exception if it soesn't exist. */
    return ((sectionExists(pSection))
        && (mSections.at(pSection).end() != mSections.at(pSection).find(pKey)));
}


/* INI class ------------------------------------------- */
INI::INI(const std::string &pFile) {
    mFileParsed = false;

    int lResult = parseFile(pFile);
    if(0 != lResult) {
        std::cerr << "[ERROR] <INI::INI> Failed to parse file " << pFile << std::endl;
        throw INIException();
    }

    mFileParsed = true;
}

INI::~INI() {
    /* Empty for now */
}

int INI::parseFile(const std::string &pFile) {
    if(mFileParsed) {
        /* File has already been parsed, need to flush all data and start over */
        std::cerr << "[ERROR] <INI::parseFile> Ini file is not empty, clearing data" << std::endl;
        mSections.clear();
        mSectionElementOrder.clear();
        mSectionOrder.clear();
    }

    mFileStream.open(pFile, std::ios::in | std::ios::out);

    /* Check if the file was opened correctly */
    if(!mFileStream.is_open()) {
        std::cerr << "[ERROR] <INI::parseFile> Failed to open file " << pFile << std::endl;
        return -1;
    }

    /* Set default section name */
    std::string lSection    = "default";
    bool        lNewSection = false;

    /* Parse the INI file */
    uint32_t lLineCount = 0U;
    for(std::string lLine = ""; std::getline(mFileStream, lLine);) {
        std::istringstream lStrStream(lLine);
        std::string lKey, lEq, lValue;

        ++lLineCount;

        /* Get the name out of the string stream.
         * Either it is a section name, a comment
         * or a value */

        /* Check if there is something on this line */
        if(!(lStrStream >> lKey)) {
            /* Got nothing from the stream */
            if(EOF == lStrStream.get()) {
                /* This line is empty or EOF */
                continue;
            }

            /* Something went wrong if you are here */
            std::cerr << "[ERROR] <INI::parseFile> Unknown parsing error at line "
                      << lLineCount << std::endl;
            mFileStream.close();
            return -1;
        }

        /* Remove prefix and trailing whitespaces */
        frontTrim(lLine, " ");
        backTrim(lLine, " ");

        /* Check if this line is a comment */
        if('#' == lLine[0U] || ';' == lLine[0U]) {
            /* This is a comment */
            std::cout << "[DEBUG] <INI::parseFile> (" << lLineCount 
                      << ") This is a comment" << std::endl;
            continue;
        }

        /* Check if the is a section name */
        if('[' == lLine[0U]) {
            /* The section tag is [tag] */
            size_t lPos = lLine.find_first_of(']');
            if(std::string::npos == lPos) {
                /* End og tag not found, this INI file is corrupt */
                std::cerr << "[ERROR] <INI::parseFile> Found unclosed section tag at line " 
                          << lLineCount << std::endl;
                mFileStream.close();
                return -1;
            }

            /* Get the section name */
            lSection = lLine.substr(1, lPos - 1);

            /* Does this section exist already ? */
            if(sectionExists(lSection)) {
                /* This section already exists ! */
                std::cerr << "[ERROR] <INI::parseFile> Duplicate Section in INI file at line " << lLineCount << std::endl;
                mFileStream.close();
                return -1;
            }

            /* Save the section in the section order vector */
            mSectionOrder.push_back(lSection);

            lNewSection = true;
            continue;
        }

        /* Now, try to extract the name/value pair */
        std::vector<std::string> lKeyValue = split(lLine, '=');

        if(2U != lKeyValue.size()) {
            if(1U == lKeyValue.size()) {
                /* Either the key is empty or the value.
                 * An empty key is invalid.
                 * An empty value is tolerated.
                 */
                if('=' == lLine.at(0U)) {
                    /* First char is '=', meanign that the key is empty */
                    std::cerr << "[ERROR] <INI::parseFile> Empty key at line "
                              << lLineCount << std::endl;
                    mFileStream.close();
                    return -1;
                } else if('=' == lLine.back()){
                    /* Value is empty. We tolerate this case
                     * by adding an empty string for the value
                     */
                    lKeyValue.push_back(std::string());
                    std::cout << "[WARN ] <INI::parseFile> Empty value at line "
                              << lLineCount << std::endl;
                } else {
                    /* There is no equal sign in the string */
                    std::cerr << "[ERROR] <INI::parseFile> No '=' sign at line "
                              << lLineCount << std::endl;
                    mFileStream.close();
                    return -1;
                }
            } else {
                /* Three strings seperated by an equal sign ?
                 * TODO : Do we consider the second '=' as
                 * part of the value string ?
                 */
                std::cerr << "[ERROR] <INI::parseFile> Invalid key/name pair at line "
                        << lLineCount << std::endl;
                mFileStream.close();
                return -1;
            }
        }

        /* We got a valid keyvalue pair */
        lKey   = lKeyValue[0U];
        lValue = lKeyValue[1U];

        /* This is commented because we accept spaces in the keyname */
        // if(std::string::npos != lKey.find(' ')) {
        //     std::cerr << "[ERROR] <INI::parseFile> Space in key at line "
        //             << lLineCount << std::endl;
        //     mFileStream.close();
        //     return -1;
        // }

        /* This is commented because we accept spaces in the value */
        // if(std::string::npos != lValue.find(' ')) {
        //     std::cerr << "[ERROR] <INI::parseFile> Space in value at line "
        //             << lLineCount << std::endl;
        //     mFileStream.close();
        //     return -1;
        // }

        /* Does this key exist already ? 
         * First, we must check if the section exists
         */
        if(!lNewSection) {
            if(keyExists(lKey)) {
                /* This key already exists ! */
                std::cerr << "[ERROR] <INI::parseFile> Duplicate Key in INI file at line " << lLineCount << std::endl;
                mFileStream.close();
                return -1;
            }
        }


        /* Save our entry */
        mSections[lSection][lKey] = lValue;

        /* Save the entry in the order vector */
        mSectionElementOrder[lSection].push_back(lKey);

        lNewSection = false;

        std::cout << "[INFO ] <INI::parseFile> [" << lSection << "] " << lKey << " = " << mSections.at(lSection).at(lKey) << std::endl;
    }

    std::cout << "[INFO ] <INI::parseFile> Parsed INI file " << pFile << " successfully !" << std::endl;
    mFileStream.close();

    return 0;
}

std::string INI::fileName(void) const {
    return mFileName;
}

int INI::getValue(const std::string &pKey,
    std::string &pOut,
    const std::string &pSection) const
{
    /* Check if the section exists */
    if(mSections.end() != mSections.find(pSection)) {
        /* Check that the key exists */
        if(mSections.at(pSection).end() != mSections.at(pSection).find(pKey)) {
            pOut = mSections.at(pSection).at(pKey);
            return 0;
        }
    }

    return -1;
}

std::vector<std::string> INI::getSections(void) const {
    std::vector<std::string> lSections;

    for(const auto &lElmt : mSections) {
        lSections.push_back(lElmt.first);
    }

    return lSections;
}

std::vector<std::string> INI::getKeys(const std::string &pSection) const {
    std::vector<std::string> lKeys;

    if(mSections.end() != mSections.find(pSection)) {
        for(const auto &lElmt : mSections.at(pSection)) {
            lKeys.push_back(lElmt.first);
        }
    }

    return lKeys;
}

std::vector<std::string> INI::getValues(const std::string &pSection) const {
    std::vector<std::string> lValues;

    if(mSections.end() != mSections.find(pSection)) {
        for(const auto &lElmt : mSections.at(pSection)) {
            lValues.push_back(lElmt.second);
        }
    }

    return lValues;
}

std::map<std::string, std::string> INI::getSectionContents(const std::string &pSection) const {
    if(sectionExists(pSection)) {
        return mSections.at(pSection);
    } else return std::map<std::string, std::string>();
}

int INI::getInt64(const std::string &pKey, int64_t &pValue, const std::string &pSection) const {
    std::string lVal;

    if(0 > getValue(pKey, lVal, pSection)) {
        /* Key/Value pair not found.
         * This is either because the section is unknown
         * or the key is unknown */
        std::cerr << "[ERROR] <INI::getInt64> Key/value pair not found (" << pSection << ", " << pKey << ")" << std::endl;
        return -1;
    }

    /* Cast the value */
    char *lEnd = 0;
    if(std::string::npos != lVal.find("0x")) {
        /* Haxadecimal value */
        pValue = strtoll(mSections.at(pSection).at(pKey).c_str(), &lEnd, 16);
    } else {
        pValue = strtoll(mSections.at(pSection).at(pKey).c_str(), &lEnd, 10);
    }

    return *lEnd == 0 ? 0 : -1;
}

int INI::getInt32(const std::string &pKey, int32_t &pValue, const std::string &pSection) const {
    std::string lVal;

    if(0 > getValue(pKey, lVal, pSection)) {
        /* Key/Value pair not found.
         * This is either because the section is unknown
         * or the key is unknown */
        std::cerr << "[ERROR] <INI::getInt32> Key/value pair not found (" << pSection << ", " << pKey << ")" << std::endl;
        return -1;
    }

    /* Cast the value */
    char *lEnd = 0;
    if(std::string::npos != lVal.find("0x")) {
        /* Haxadecimal value */
        pValue = strtol(mSections.at(pSection).at(pKey).c_str(), &lEnd, 16);
    } else {
        pValue = strtol(mSections.at(pSection).at(pKey).c_str(), &lEnd, 10);
    }

    return *lEnd == 0 ? 0 : -1;
}

int INI::getInt16(const std::string &pKey, int16_t &pValue, const std::string &pSection) const {
    std::string lVal;

    if(0 > getValue(pKey, lVal, pSection)) {
        /* Key/Value pair not found.
         * This is either because the section is unknown
         * or the key is unknown */
        std::cerr << "[ERROR] <INI::getInt16> Key/value pair not found (" << pSection << ", " << pKey << ")" << std::endl;
        return -1;
    }

    /* Cast the value */
    char *lEnd = 0;
    int32_t lTempVal = 0;
    if(std::string::npos != lVal.find("0x")) {
        /* Haxadecimal value */
        lTempVal = (int16_t)strtol(mSections.at(pSection).at(pKey).c_str(), &lEnd, 16);
    } else {
        lTempVal = (int16_t)strtol(mSections.at(pSection).at(pKey).c_str(), &lEnd, 10);
    }

    /* Check limits */
    if(SHRT_MAX < lTempVal || SHRT_MIN > lTempVal) {
        std::cerr << "[ERROR] <INI::getInt16> Value is out of bounds !" << std::endl;
        return -1;
    } else {
        pValue = (int16_t)lTempVal;
    }

    return *lEnd == 0 ? 0 : -1;
}

int INI::getInt8(const std::string &pKey, int8_t &pValue, const std::string &pSection) const {
    std::string lVal;

    if(0 > getValue(pKey, lVal, pSection)) {
        /* Key/Value pair not found.
         * This is either because the section is unknown
         * or the key is unknown */
        std::cerr << "[ERROR] <INI::getInt8> Key/value pair not found (" << pSection << ", " << pKey << ")" << std::endl;
        return -1;
    }

    /* Cast the value */
    char *lEnd = 0;
    int32_t lTempVal = 0;
    if(std::string::npos != lVal.find("0x")) {
        /* Haxadecimal value */
        lTempVal = (int16_t)strtol(mSections.at(pSection).at(pKey).c_str(), &lEnd, 16);
    } else {
        lTempVal = (int16_t)strtol(mSections.at(pSection).at(pKey).c_str(), &lEnd, 10);
    }

    /* Check limits */
    if(SCHAR_MAX < lTempVal || SCHAR_MIN > lTempVal) {
        std::cerr << "[ERROR] <INI::getInt8> Value is out of bounds !" << std::endl;
        return -1;
    } else {
        pValue = (int8_t)lTempVal;
    }

    return *lEnd == 0 ? 0 : -1;
}

int INI::getUInt64(const std::string &pKey, uint64_t &pValue, const std::string &pSection) const {
    std::string lVal;

    if(0 > getValue(pKey, lVal, pSection)) {
        /* Key/Value pair not found.
         * This is either because the section is unknown
         * or the key is unknown */
        std::cerr << "[ERROR] <INI::getUInt64> Key/value pair not found (" << pSection << ", " << pKey << ")" << std::endl;
        return -1;
    }

    /* Cast the value */
    char *lEnd = 0;
    if(std::string::npos != lVal.find("0x")) {
        /* Haxadecimal value */
        pValue = strtoull(mSections.at(pSection).at(pKey).c_str(), &lEnd, 16);
    } else {
        pValue = strtoull(mSections.at(pSection).at(pKey).c_str(), &lEnd, 10);
    }
    
    return *lEnd == 0 ? 0 : -1;
}

int INI::getUInt32(const std::string &pKey, uint32_t &pValue, const std::string &pSection) const {
    std::string lVal;

    if(0 > getValue(pKey, lVal, pSection)) {
        /* Key/Value pair not found.
         * This is either because the section is unknown
         * or the key is unknown */
        std::cerr << "[ERROR] <INI::getUInt32> Key/value pair not found (" << pSection << ", " << pKey << ")" << std::endl;
        return -1;
    }

    /* Cast the value */
    char *lEnd = 0;
    if(std::string::npos != lVal.find("0x")) {
        /* Haxadecimal value */
        pValue = strtoul(mSections.at(pSection).at(pKey).c_str(), &lEnd, 16);
    } else {
        pValue = strtoul(mSections.at(pSection).at(pKey).c_str(), &lEnd, 10);
    }
    
    return *lEnd == 0 ? 0 : -1;
}

int INI::getUInt16(const std::string &pKey, uint16_t &pValue, const std::string &pSection) const {
    std::string lVal;

    if(0 > getValue(pKey, lVal, pSection)) {
        /* Key/Value pair not found.
         * This is either because the section is unknown
         * or the key is unknown */
        std::cerr << "[ERROR] <INI::getUInt16> Key/value pair not found (" << pSection << ", " << pKey << ")" << std::endl;
        return -1;
    }

    /* Cast the value */
    char *lEnd = 0;
    uint32_t lTempVal = 0U;
    if(std::string::npos != lVal.find("0x")) {
        /* Haxadecimal value */
        lTempVal = strtoul(mSections.at(pSection).at(pKey).c_str(), &lEnd, 16);
    } else {
        lTempVal = strtoul(mSections.at(pSection).at(pKey).c_str(), &lEnd, 10);
    }

    /* Check limits */
    if(USHRT_MAX < lTempVal) {
        std::cerr << "[ERROR] <INI::getUInt16> Value is out of bounds !" << std::endl;
        return -1;
    } else {
        pValue = (uint16_t)lTempVal;
    }
    
    return *lEnd == 0 ? 0 : -1;
}

int INI::getUInt8(const std::string &pKey, uint8_t &pValue, const std::string &pSection) const {
    std::string lVal;

    if(0 > getValue(pKey, lVal, pSection)) {
        /* Key/Value pair not found.
         * This is either because the section is unknown
         * or the key is unknown */
        std::cerr << "[ERROR] <INI::getUInt8> Key/value pair not found (" << pSection << ", " << pKey << ")" << std::endl;
        return -1;
    }

    /* Cast the value */
    char *lEnd = 0;
    uint32_t lTempVal = 0U;
    if(std::string::npos != lVal.find("0x")) {
        /* Haxadecimal value */
        lTempVal = strtoul(mSections.at(pSection).at(pKey).c_str(), &lEnd, 16);
    } else {
        lTempVal = strtoul(mSections.at(pSection).at(pKey).c_str(), &lEnd, 10);
    }

    /* Check limits */
    if(UCHAR_MAX < lTempVal) {
        std::cerr << "[ERROR] <INI::getUInt8> Value is out of bounds !" << std::endl;
        return -1;
    } else {
        pValue = (uint8_t)lTempVal;
    }
    
    return *lEnd == 0 ? 0 : -1;
}

int INI::getString(const std::string &pKey, std::string &pValue, const std::string &pSection) const {
    return getValue(pKey, pValue, pSection);
}

int INI::getBoolean(const std::string &pKey, bool &pValue, const std::string &pSection) const {
    std::string lVal;

    if(0 > getValue(pKey, lVal, pSection)) {
        /* Key/Value pair not found.
         * This is either because the section is unknown
         * or the key is unknown */
        std::cerr << "[ERROR] <INI::get> Key/value pair not found (" << pSection << ", " << pKey << ")" << std::endl;
        return -1;
    }

    /* Cast the value */
    if(("true" == mSections.at(pSection).at(pKey))
        || (mSections.at(pSection).at(pKey) == "True")
        || (mSections.at(pSection).at(pKey) == "1"))
    {
        pValue = true;
    } else if ((mSections.at(pSection).at(pKey) == "false")
        || (mSections.at(pSection).at(pKey) == "False")
        || (mSections.at(pSection).at(pKey) == "0"))
    {
        pValue = false;
    } else {
        /* Unexpected value */
        return -1;
    }

    return 0;
}

int INI::getDouble(const std::string &pKey, double &pValue, const std::string &pSection) const {
    std::string lVal;

    if(0 > getValue(pKey, lVal, pSection)) {
        /* Key/Value pair not found.
         * This is either because the section is unknown
         * or the key is unknown */
        std::cerr << "[ERROR] <INI::get> Key/value pair not found (" << pSection << ", " << pKey << ")" << std::endl;
        return -1;
    }

    /* Cast the value */
    char *lEnd = 0;
    pValue = strtod(mSections.at(pSection).at(pKey).c_str(), &lEnd);
    return *lEnd == 0 ? 0 : -1;
}


/* Setters */
int INI::setInt64(const std::string &pKey, const int64_t &pValue, const std::string &pSection) {
    std::string lVal = std::to_string(pValue);

    /* Check if the section exists */
    if(mSections.end() != mSections.find(pSection)) {
        /* Check if the key exists */
        if(mSections.at(pSection).end() != mSections.at(pSection).find(pKey)) {
            /* The key does exist ! */
            mSections.at(pSection).at(pKey) = lVal;

            return 0;
        }
    }

    return -1;
}

int INI::setInt32(const std::string &pKey, const int32_t &pValue, const std::string &pSection) {
    std::string lVal = std::to_string(pValue);

    /* Check if the section exists */
    if(mSections.end() != mSections.find(pSection)) {
        /* Check if the key exists */
        if(mSections.at(pSection).end() != mSections.at(pSection).find(pKey)) {
            /* The key does exist ! */
            mSections.at(pSection).at(pKey) = lVal;

            return 0;
        }
    }

    return -1;
}

int INI::setInt16(const std::string &pKey, const int16_t &pValue, const std::string &pSection) {
    std::string lVal = std::to_string(pValue);

    /* Check if the section exists */
    if(mSections.end() != mSections.find(pSection)) {
        /* Check if the key exists */
        if(mSections.at(pSection).end() != mSections.at(pSection).find(pKey)) {
            /* The key does exist ! */
            mSections.at(pSection).at(pKey) = lVal;

            return 0;
        }
    }

    return -1;
}

int INI::setInt8(const std::string &pKey, const int8_t &pValue, const std::string &pSection) {
    std::string lVal = std::to_string(pValue);

    /* Check if the section exists */
    if(mSections.end() != mSections.find(pSection)) {
        /* Check if the key exists */
        if(mSections.at(pSection).end() != mSections.at(pSection).find(pKey)) {
            /* The key does exist ! */
            mSections.at(pSection).at(pKey) = lVal;

            return 0;
        }
    }

    return -1;
}

int INI::setUInt64(const std::string &pKey, const uint64_t &pValue, const std::string &pSection, const int &pBase) {
    std::string lVal;

    if(10 == pBase) {
        lVal = std::to_string(pValue);
    } else if (16 == pBase) {
        char lStr[19U];
        std::snprintf(lStr, 19U, "0x%016X", pValue);
        lVal = std::string(lStr);
    } else {
        std::cerr << "[ERROR] <INI::setUInt64> Unknown base specified" << std::endl;
        return -1;
    }

    /* Check if the section exists */
    if(mSections.end() != mSections.find(pSection)) {
        /* Check if the key exists */
        if(mSections.at(pSection).end() != mSections.at(pSection).find(pKey)) {
            /* The key does exist ! */
            mSections.at(pSection).at(pKey) = lVal;

            return 0;
        }
    }

    return -1;
}

int INI::setUInt32(const std::string &pKey, const uint32_t &pValue, const std::string &pSection, const int &pBase) {
    std::string lVal;

    if(10 == pBase) {
        lVal = std::to_string(pValue);
    } else if (16 == pBase) {
        char lStr[11U];
        std::snprintf(lStr, 11U, "0x%08X", pValue);
        lVal = std::string(lStr);
    } else {
        std::cerr << "[ERROR] <INI::setUInt32> Unknown base specified" << std::endl;
        return -1;
    }

    /* Check if the section exists */
    if(mSections.end() != mSections.find(pSection)) {
        /* Check if the key exists */
        if(mSections.at(pSection).end() != mSections.at(pSection).find(pKey)) {
            /* The key does exist ! */
            mSections.at(pSection).at(pKey) = lVal;

            return 0;
        }
    }

    return -1;
}

int INI::setUInt16(const std::string &pKey, const uint16_t &pValue, const std::string &pSection, const int &pBase) {
    std::string lVal;

    if(10 == pBase) {
        lVal = std::to_string(pValue);
    } else if (16 == pBase) {
        char lStr[7U];
        std::snprintf(lStr, 7U, "0x%04X", pValue);
        lVal = std::string(lStr);
    } else {
        std::cerr << "[ERROR] <INI::setUInt16> Unknown base specified" << std::endl;
        return -1;
    }

    /* Check if the section exists */
    if(mSections.end() != mSections.find(pSection)) {
        /* Check if the key exists */
        if(mSections.at(pSection).end() != mSections.at(pSection).find(pKey)) {
            /* The key does exist ! */
            mSections.at(pSection).at(pKey) = lVal;

            return 0;
        }
    }

    return -1;
}

int INI::setUInt8(const std::string &pKey, const uint8_t &pValue, const std::string &pSection, const int &pBase) {
    std::string lVal;

    if(10 == pBase) {
        lVal = std::to_string(pValue);
    } else if (16 == pBase) {
        char lStr[5U];
        std::snprintf(lStr, 5U, "0x%02X", pValue);
        lVal = std::string(lStr);
    } else {
        std::cerr << "[ERROR] <INI::setUInt8> Unknown base specified" << std::endl;
        return -1;
    }

    /* Check if the section exists */
    if(mSections.end() != mSections.find(pSection)) {
        /* Check if the key exists */
        if(mSections.at(pSection).end() != mSections.at(pSection).find(pKey)) {
            /* The key does exist ! */
            mSections.at(pSection).at(pKey) = lVal;

            return 0;
        }
    }

    return -1;
}

int INI::setString(const std::string &pKey, const std::string &pValue, const std::string &pSection) {
    /* Check if the section exists */
    if(mSections.end() != mSections.find(pSection)) {
        /* Check if the key exists */
        if(mSections.at(pSection).end() != mSections.at(pSection).find(pKey)) {
            /* The key does exist ! */
            mSections.at(pSection).at(pKey) = pValue;

            return 0;
        }
    }

    return -1;
}

int INI::setBoolean(const std::string &pKey, const bool &pValue, const std::string &pSection) {
    std::string lVal = pValue ? "true" : "false";

    /* Check if the section exists */
    if(mSections.end() != mSections.find(pSection)) {
        /* Check if the key exists */
        if(mSections.at(pSection).end() != mSections.at(pSection).find(pKey)) {
            /* The key does exist ! */
            mSections.at(pSection).at(pKey) = lVal;

            return 0;
        }
    }

    return -1;
}

int INI::setDouble(const std::string &pKey, const double &pValue, const std::string &pSection) {
    std::string lVal = std::to_string(pValue);

    /* Check if the section exists */
    if(mSections.end() != mSections.find(pSection)) {
        /* Check if the key exists */
        if(mSections.at(pSection).end() != mSections.at(pSection).find(pKey)) {
            /* The key does exist ! */
            mSections.at(pSection).at(pKey) = lVal;

            return 0;
        }
    }

    return -1;
}


/* Adders */
int INI::addSection(const std::string &pSection) {
    /* Does this section exist already ? */
    if(sectionExists(pSection)) {
        /* This section already exists ! */
        std::cerr << "[ERROR] <INI::addSection> Section already exists" << std::endl;
        return -1;
    }

    /* Add the section with no keys */
    mSections[pSection] = std::map<std::string, std::string>();
    mSectionOrder.push_back(pSection);
    mSectionElementOrder[pSection] = std::vector<std::string>();

    return -1;
}

int INI::addInt64(const std::string &pKey, const int64_t &pValue, const std::string &pSection) {
    return addString(pKey, std::to_string(pValue), pSection);
}

int INI::addInt32(const std::string &pKey, const int32_t &pValue, const std::string &pSection) {
    return addString(pKey, std::to_string(pValue), pSection);
}

int INI::addInt16(const std::string &pKey, const int16_t &pValue, const std::string &pSection) {
    return addString(pKey, std::to_string(pValue), pSection);
}

int INI::addInt8(const std::string &pKey, const int8_t &pValue, const std::string &pSection) {
    return addString(pKey, std::to_string(pValue), pSection);
}

int INI::addUInt64(const std::string &pKey, const uint64_t &pValue, const std::string &pSection, const int &pBase) {
    if(10 == pBase) {
        return addString(pKey, std::to_string(pValue), pSection);
    } else if (16 == pBase) {
        char lStr[19U];
        std::snprintf(lStr, 19U, "0x%016X", pValue);
        return addString(pKey, std::string(lStr), pSection);
    } else {
        std::cerr << "[ERROR] <INI::addUInt64> Unknown base specified" << std::endl;
        return -1;
    }
}

int INI::addUInt32(const std::string &pKey, const uint32_t &pValue, const std::string &pSection, const int &pBase) {
    if(10 == pBase) {
        return addString(pKey, std::to_string(pValue), pSection);
    } else if (16 == pBase) {
        char lStr[11U];
        std::snprintf(lStr, 11U, "0x%08X", pValue);
        return addString(pKey, std::string(lStr), pSection);
    } else {
        std::cerr << "[ERROR] <INI::addUInt32> Unknown base specified" << std::endl;
        return -1;
    }
}

int INI::addUInt16(const std::string &pKey, const uint16_t &pValue, const std::string &pSection, const int &pBase) {
    if(10 == pBase) {
        return addString(pKey, std::to_string(pValue), pSection);
    } else if (16 == pBase) {
        char lStr[7U];
        std::snprintf(lStr, 7U, "0x%04X", pValue);
        return addString(pKey, std::string(lStr), pSection);
    } else {
        std::cerr << "[ERROR] <INI::addUInt16> Unknown base specified" << std::endl;
        return -1;
    }
}

int INI::addUInt8(const std::string &pKey, const uint8_t &pValue, const std::string &pSection, const int &pBase) {
    if(10 == pBase) {
        return addString(pKey, std::to_string(pValue), pSection);
    } else if (16 == pBase) {
        char lStr[5U];
        std::snprintf(lStr, 5U, "0x%02X", pValue);
        return addString(pKey, std::string(lStr), pSection);
    } else {
        std::cerr << "[ERROR] <INI::addUInt8> Unknown base specified" << std::endl;
        return -1;
    }
}

int INI::addString(const std::string &pKey, const std::string &pValue, const std::string &pSection) {
    /* Does this section exist ? */
    if(!sectionExists(pSection)) {
        /* This section doesn't exist ! */
        std::cerr << "[ERROR] <INI::addString> Section doesn't exist" << std::endl;
        return -1;
    }

    /* Does the key already exist ? */
    if(keyExists(pKey)) {
        /* This key already exists ! */
        std::cerr << "[ERROR] <INI::addString> Key already exists" << std::endl;
        return -1;
    }

    /* Add the key to the associated section */
    mSections.at(pSection)[pKey] = pValue;
    mSectionElementOrder.at(pSection).push_back(pKey);

    return 0;
}

int INI::addBoolean(const std::string &pKey, const bool &pValue, const std::string &pSection) {
    return addString(pKey, pValue ? "true" : "false", pSection);
}

int INI::addDouble(const std::string &pKey, const double &pValue, const std::string &pSection) {
    return addString(pKey, std::to_string(pValue), pSection);
}

int INI::removeSection(const std::string &pSection) {
    /* Does this section exist ? */
    if(!sectionExists(pSection)) {
        /* This section doesn't exist ! */
        std::cerr << "[ERROR] <INI::removeSection> Section doesn't exist" << std::endl;
        return -1;
    }

    mSectionOrder.erase(find(mSectionOrder.begin(), mSectionOrder.end(), pSection));
    mSections.erase(pSection);
    mSectionElementOrder.erase(pSection);

    return 0;
}

int INI::removeKey(const std::string &pSection, const std::string &pKey) {
    /* Does this section exist ? */
    if(!sectionExists(pSection)) {
        /* This section doesn't exist ! */
        std::cerr << "[ERROR] <INI::removeKey> Section doesn't exist" << std::endl;
        return -1;
    }

    /* Does this key exist . */
    if(!keyExists(pKey, pSection)) {
        /* This key doesn't exist ! */
        std::cerr << "[ERROR] <INI::removeKey> Key doesn't exist" << std::endl;
        return -1;
    }

    mSections.at(pSection).erase(pKey);
    std::vector<std::string> *lTempVector = &mSectionElementOrder.at(pSection);
    lTempVector->erase(find(lTempVector->begin(), lTempVector->end(), pKey));
    
    return 0;
}


/* Generator */
int INI::generateFile(const std::string &pDest) const {
    /* Are we overwriting our original INI file ? */
    if(mFileName == pDest) {
        /* Overwrite detectedn not supported for now */
        std::cerr << "[ERROR] <INI::generateFile> Overwrite detectedn not supported for now." << std::endl;
        return -1;
    }

    std::fstream lOutputFileStream(pDest, std::ios::out);
    if(!lOutputFileStream.is_open()) {
        std::cerr << "[ERROR] <INI::generateFile> Failed to open file " << pDest << std::endl;
        return -1;
    }

    /* For each section */
    for(const auto &lSection : mSectionOrder) {
        /* Write the section name */
        lOutputFileStream << "[" << lSection << "]" << std::endl;

        /* For each key in the section */
        for(const auto &lKey : mSectionElementOrder.at(lSection)) {
            /* Write the key, the equal sign and the value */
            lOutputFileStream << lKey << "=" << mSections.at(lSection).at(lKey) << std::endl;
        }

        /* Add an empty line between sections.
         * This will also add an empty line at EOF.
        */
        lOutputFileStream << std::endl;
    }

    std::cout << "[INFO ] <INI::generateFile> Successfully generated INI file " << pDest << std::endl;
    return 0;
}
