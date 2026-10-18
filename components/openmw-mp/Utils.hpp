#ifndef UTILS_HPP
#define UTILS_HPP

#include <algorithm>
#include <string>
#include <sstream>
#include <vector>

#if (defined __WIN32__ || defined _WIN32 || defined WIN32)
#define __WINDOWS
#endif

#ifdef __WINDOWS
int setenv(const char *name, const char *value, int overwrite);
#endif

namespace Utils
{
    std::string convertPath(std::string str);
    bool doesFileHaveChecksum(std::string filePath, unsigned int requiredChecksum);

    void timestamp();

    int progressFunc(double TotalToDownload, double NowDownloaded);

    unsigned int getNumberOfDigits(int integer);
    bool compareDoubles(double a, double b, double epsilon = 0.005);
    bool compareFloats(float a, float b, float epsilon = 0.005f);

    template <class Type, class Type2>
    bool vectorContains(const std::vector<Type> &vectorChecked, const Type2 &value)
    {
        return std::find(vectorChecked.begin(), vectorChecked.end(), value) != vectorChecked.end();
    }

    template <class Type>
    uint32_t getVectorSize(const std::vector<Type> &vectorChecked)
    {
        return static_cast<uint32_t>(vectorChecked.size());
    }

    template <class Type>
    void resetVector(std::vector<Type> &vectorInput, uint32_t newSize)
    {
        vectorInput.clear();
        vectorInput.resize(newSize);
    }

    std::string replaceString(const std::string &source, const char *find, const char *replace);

    std::string toString(int num);

    std::string &removeExtension(std::string &file);

    long int getFileLength(const char *file);

    unsigned int crc32Checksum(const std::string &file);

    std::string getOperatingSystemType();
    std::string getArchitectureType();

    std::string getVersionInfo(std::string appName, std::string version, std::string commitHash, int protocol);

    void printWithWidth(std::ostringstream &sstr, std::string str, size_t width);

    std::string intToHexStr(unsigned val);
    unsigned int hexStrToInt(std::string hexString);
}
#endif //UTILS_HPP
