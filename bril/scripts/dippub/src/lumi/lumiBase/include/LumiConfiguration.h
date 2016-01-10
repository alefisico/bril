#ifndef LUMICONFIGURATION_H
#define LUMICONFIGURATION_H

#include <map>
#include <string>
#include <vector>
#include <stdexcept>

class LumiConfiguration {
  public:

   LumiConfiguration(const char* path) throw (std::runtime_error);

   // main methods
   long int                 GetInt(const char* key)       throw (std::runtime_error);
   bool                     GetBool(const char* key)      throw (std::runtime_error);
   double                   GetDouble(const char* key)    throw (std::runtime_error);
   std::string              GetString(const char* key)    throw (std::runtime_error);
   std::vector<std::string> GetArrString(const char* key) throw (std::runtime_error);

  protected:

   void RemoveLeadTrailSpaces(std::string& str);

   // collected configuration: {key: array of strings}
   std::map<std::string,std::vector<std::string> > mCfg;
};

#endif
