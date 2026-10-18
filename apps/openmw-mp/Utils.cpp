#include "Utils.hpp"

#include <cstdarg>

const std::vector<std::string> Utils::split(const std::string &str, int delimiter)
{
    std::string buffer;
    std::vector<std::string> result;

    for (auto symb:str)
        if (symb != delimiter)
            buffer += symb;
        else if (!buffer.empty())
        {
            result.push_back(move(buffer));
            buffer.clear();
        }
    if (!buffer.empty())
        result.push_back(move(buffer));

    return result;
}

ESM::Cell Utils::getCellFromDescription(std::string cellDescription)
{
    ESM::Cell cell;
    cell.blank();

    static std::regex exteriorCellPattern("^(-?\\d+), (-?\\d+)$");
    std::smatch baseMatch;

    if (std::regex_match(cellDescription, baseMatch, exteriorCellPattern))
    {
        cell.mData.mFlags &= ~ESM::Cell::Interior;

        // The first sub match is the whole string, so check for a length of 3
        if (baseMatch.size() == 3)
        {
            cell.mData.mX = stoi(baseMatch[1].str());
            cell.mData.mY = stoi(baseMatch[2].str());
        }
    }
    else
    {
        cell.mData.mFlags |= ESM::Cell::Interior;
        cell.mName = cellDescription;
    }

    return cell;
}

void Utils::getArguments(std::vector<boost::any> &params, va_list args, const std::string &def)
{
    params.reserve(def.length());

    try
    {
        for (char c : def)
        {
            switch (c)
            {
            case 'i':
                params.emplace_back(va_arg(args, unsigned int));
                break;

            case 'q':
                params.emplace_back(va_arg(args, signed int));
                break;

            case 'l':
                params.emplace_back(va_arg(args, unsigned long long));
                break;

            case 'w':
                params.emplace_back(va_arg(args, signed long long));
                break;

            case 'f':
                params.emplace_back(va_arg(args, double));
                break;

            case 'p':
                params.emplace_back(va_arg(args, void*));
                break;

            case 's':
                params.emplace_back(va_arg(args, const char*));
                break;

            case 'b':
                params.emplace_back(va_arg(args, int));
                break;

            default:
                throw std::runtime_error("C++ call: Unknown argument identifier " + c);
            }
        }
    }

    catch (...)
    {
        va_end(args);
        throw;
    }
    va_end(args);
}

const char *Utils::refIdCStr(const ESM::RefId &id)
{
    if (id.empty() || id.is<ESM::StringRefId>())
        return id.getRefIdString().c_str();
    thread_local std::string text;
    text = id.toString();
    return text.c_str();
}
