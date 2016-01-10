#ifndef _bril_pltslinksource_WebUtils_h_
#define _bril_pltslinksource_WebUtils_h_
#include <string>
#include <map>
#include <sstream>
#include "xdata/Table.h"
#include "xdata/Serializable.h"
namespace bril{ namespace pltslinksource { namespace WebUtils{
      std::string mapToHTML(
		     const std::string& caption,
		     const std::map<std::string, xdata::Serializable*>& data,
		     const std::string& tablestyle,
		     bool isVertical=true);
      std::string xdatatableToHTML(xdata::Serializable* s);
}}}
#endif
