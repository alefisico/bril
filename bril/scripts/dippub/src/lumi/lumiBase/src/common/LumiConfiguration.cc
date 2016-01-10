// Class which enables usage of configuration files containing "key = value" pairs.

#include <math.h>
#include <fstream>
#include <ctype.h>
#include <stdlib.h>
#include <limits.h>
#include <strings.h>

#include "LumiConfiguration.h"

//_____________________________________________________________________________________________
LumiConfiguration::LumiConfiguration(const char* path) throw (std::runtime_error)
{
   // Opens text file referenced by "path" and parses "key = value" lines, collecting results
   // into mCfg.
   //
   // If a line with invalid syntax is encountered, exception is raised.
   //
   // Valid syntax:
   //    - empty lines and lines starting with "#" or ";" are skipped;
   //    - all other lines must be of "key = value" form;
   //    - any number of spaces around "key" and "value" are allowed;
   //    - "key" must not contain the equality symbol ("=");
   //    - single/double quotes (' and ") are not specially treated;
   //    - if "key" is repeated over several lines, respective values are collected and
   //      considered as an array;
   //    - too long lines (>= 4kb) are not allowed.

   std::ifstream in(path);
   if (in.fail())
      throw std::runtime_error("file open failed");

   char buf[4096];

   // loop over lines and collect {key: array of values} into mCfg
   while (in.good()) {
      in.getline(buf, 4096);

      if (!in.eof() && in.fail())
         throw std::runtime_error("ifstream::getline() failed");

      std::string line = buf;

      // skip comments
      if (line.size() > 0 && (line[0] == '#' || line[0] == ';'))
         continue;

      RemoveLeadTrailSpaces(line);

      // skip empty lines
      if (line.size() == 0)
         continue;

      // search for the first equality symbol
      size_t eq = 0;
      while (eq < line.size() && line[eq] != '=')
         eq++;

      if (eq == 0 || eq == line.size())
         throw std::runtime_error("invalid syntax");

      // extract key and value
      std::string key = line.substr(0, eq);
      std::string value = line.substr(eq + 1);

      RemoveLeadTrailSpaces(key);
      RemoveLeadTrailSpaces(value);

      // append value collesponding to this key
      mCfg[key].push_back(value);
   }

   in.close();
}

//_____________________________________________________________________________________________
long int LumiConfiguration::GetInt(const char* key) throw (std::runtime_error)
{
   // Returns an integer identified by "key".
   //
   // Decimal, octal (must start with "0") and hex (must start with "0x") integers are equally
   // accepted.

   // verify existence of the key
   if (mCfg.find(key) == mCfg.end())
      throw std::runtime_error(std::string("key not found: ") + key);

   // verify that there is exactly one key with this name
   if (mCfg[key].size() != 1)
      throw std::runtime_error(std::string("more than one value for key: ") + key);

   // convert to integer (decimal, octal and hex numbers are accepted)
   char* ptr; // reference to first invalid character
   long int result = strtol(mCfg[key][0].c_str(), &ptr, 0);

   // validation
   if (result == LONG_MIN || result == LONG_MAX || *ptr != '\0')
      throw std::runtime_error(std::string("not an integer value for key: ") + key);

   return result;
}

//_____________________________________________________________________________________________
bool LumiConfiguration::GetBool(const char* key) throw (std::runtime_error)
{
   // Returns a boolean identified by "key".
   //
   // Value of the boolean in file must be equal to any of yes/no, true/false or 0/1 (case
   // insensitive).

   // verify existence of the key
   if (mCfg.find(key) == mCfg.end())
      throw std::runtime_error(std::string("key not found: ") + key);

   // verify that there is exactly one key with this name
   if (mCfg[key].size() != 1)
      throw std::runtime_error(std::string("more than one value for key: ") + key);

   const char* v = mCfg[key][0].c_str();

   // case insensitive comparison
   if (strcasecmp(v, "no") == 0 || strcasecmp(v, "false") == 0 || strcasecmp(v, "0") == 0)
      return false;

   if (strcasecmp(v, "yes") == 0 || strcasecmp(v, "true") == 0 || strcasecmp(v, "1") == 0)
      return true;

   throw std::runtime_error(std::string("not a boolean value for key: ") + key);
}

//_____________________________________________________________________________________________
double LumiConfiguration::GetDouble(const char* key) throw (std::runtime_error)
{
   // Returns a floating-point number identified by "key".

   // verify existence of the key
   if (mCfg.find(key) == mCfg.end())
      throw std::runtime_error(std::string("key not found: ") + key);

   // verify that there is exactly one key with this name
   if (mCfg[key].size() != 1)
      throw std::runtime_error(std::string("more than one value for key: ") + key);

   // convert to double
   // NOTE: hex numbers, nan and infinity objects are also accepted
   char* ptr; // reference to first invalid character
   double result = strtod(mCfg[key][0].c_str(), &ptr);

   // validation
   if (result == HUGE_VAL || result == -HUGE_VAL || *ptr != '\0')
      throw std::runtime_error(std::string("not a floating-point value for key: ") + key);

   return result;
}

//_____________________________________________________________________________________________
std::string LumiConfiguration::GetString(const char* key) throw (std::runtime_error)
{
   // Returns a string identified by "key".

   // verify existence of the key
   if (mCfg.find(key) == mCfg.end())
      throw std::runtime_error(std::string("key not found: ") + key);

   // verify that there is exactly one key with this name
   if (mCfg[key].size() != 1)
      throw std::runtime_error(std::string("more than one value for key: ") + key);

   return mCfg[key][0];
}

//_____________________________________________________________________________________________
std::vector<std::string> LumiConfiguration::GetArrString(const char* key)
   throw (std::runtime_error)
{
   // Returns a vector of strings identified by "key".

   // verify existence of the key
   if (mCfg.find(key) == mCfg.end())
      throw std::runtime_error(std::string("key not found: ") + key);

   return mCfg[key];
}

//_____________________________________________________________________________________________
void LumiConfiguration::RemoveLeadTrailSpaces(std::string& str)
{
   // Removes (in-place) leading and trailing spaces from given string.

   // remove trailing spaces
   size_t last = str.size() - 1;  // last non-space character
   while (last >= 0 && isspace(str[last]))
      last--;

   if (last + 1 < str.size())
      str.erase(last + 1);

   // remove leading spaces
   size_t first = 0;  // first non-space character
   while (first < last && isspace(str[first]))
      first++;

   if (first > 0)
      str.erase(0, first);
}
