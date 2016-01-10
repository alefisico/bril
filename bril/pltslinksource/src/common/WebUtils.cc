#include "bril/pltsource/WebUtils.h"
namespace bril{
  namespace pltsource{
    namespace WebUtils{
      std::string mapToHTML(const std::string& caption,
			    const std::map<std::string, xdata::Serializable*>& data,
			    const std::string& tablestyle,
			    bool isVertical){	
	std::ostringstream out;
	if(isVertical){
	  out << "<table class=\"xdaq-table-vertical\"";
	}else{
	  out << "<table class=\"xdaq-table\"";
	}
	if (!tablestyle.empty()){
	  out << "style=\""<<tablestyle<<"\"";
	}
	out << ">";
	out << "<caption>"<<caption<<"</caption>";
	std::map<std::string,xdata::Serializable*>::const_iterator dataIt;
	std::map<std::string,xdata::Serializable*>::const_iterator dataItBeg = data.begin();
	std::map<std::string,xdata::Serializable*>::const_iterator dataItEnd = data.end();
	if(isVertical){
	  out << "<tbody>";	  
	  for(dataIt=dataItBeg;dataIt!=dataItEnd;++dataIt){
	    out << "<tr>";
	    out << "<th>" << dataIt->first<< "</th>";
	    out << "<td>" << dataIt->second->toString()<< "</td>";
	    out << "</tr>";
	  }
	}else{
	  out << "<thead>";
	  out << "<tr>";
	  for(dataIt=dataItBeg;dataIt!=dataItEnd;++dataIt){
	    out << "<th>"<<dataIt->first<<"</th>";
	  }
	  out << "</tr>";
	  out << "</thead>";
	  
	  out << "<tbody>";
	  out << "<tr>";
	  for (dataIt = data.begin(); dataIt!= data.end(); ++dataIt){
	    out << "<td>" << dataIt->second->toString() << "</td>";
	  }
	  out << "</tr>";	  
	}
	out << "</tbody>";
	out << "</table>"; 
	std::cout<<out.str()<<std::endl;
	return out.str();
      }
      
      std::string xdatatableToHTML(xdata::Serializable* s){
	std::ostringstream out;
	out.precision(3);
	out<<std::scientific;
	xdata::Table* tref = dynamic_cast<xdata::Table*>(s);
	if (tref->getRowCount() == 0){
	  return "";
	}
	out << "<table class=\"xdaq-table\"";
	out << ">";
	out << "<thead>";
	out << "<tr>";
	std::vector<std::string> columns = tref->getColumns();
	for (std::vector<std::string>::size_type i = 0; i < columns.size(); i++ ){
	  std::string localName = columns[i].substr(columns[i].rfind(":")+1);
	out << "<th title=\"" << columns[i] << "\" class=\"xdaq-sortable\">" << localName << "</th>";
	}
	out << "<tr>" << std::endl;
	out << "</thead>";
	out << "<tbody>";
	for ( size_t j = 0; j <  tref->getRowCount(); j++ ){
	  out << "<tr>" << std::endl;
	  for (std::vector<std::string>::size_type k = 0; k < columns.size(); k++ ){
	    xdata::Serializable * s = tref->getValueAt(j, columns[k]);	                       
	    if (s->type() == "table"){//if nested table
	      out << "<td>";
	      out << xdatatableToHTML(s);
	      out << "</td>" ;
	    }else{                                               
	      out << "<td style=\"vertical-align: top;\">" << s->toString() << "</td>" ;
	    }	                
	  }
	  out << "</tr>";
	}
	out << "</tbody>";
	out << "</table>";    
	return out.str();
      }
      
    }}}
