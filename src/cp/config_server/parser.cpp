#include "parser.h"
#include "default_conf.h"
#include "log.h"
#include "fox/fox_config_server.h"

#include <cstdint>
#include <string>
#include <stdlib.h>
#include <iostream>
#include <sstream>

//#include <boost/spirit/include/lex_lexertl.hpp>
//#include <boost/bind.hpp>
//#include <boost/ref.hpp>

std::vector<conf_entry> * cf_tbl = NULL;
// Table from which channel configuration entries are served.

static void logConfigTableName(std::string name) {
  std::ostringstream log_entry;
  log_entry << "Selected configuration table: ";
  log_entry << "\"" << name << "\"";
  logStatus(__FILE__, __LINE__, log_entry.str());
}

void setConfigTable(std::string cnf) {
  bool found = false;
  for (auto it = conf_tables.begin(); it != conf_tables.end(); it++) {
    if (it->table_name == cnf) {
      if (found) {
        // conf_tables contains two tables of name 'cnf':
        std::cerr << "ERROR: duplicated table name \"" << cnf << "\" ";
        std::cerr << "in file default_conf.h" << std::endl;
        exit(EXIT_FAILURE);
      }
      cf_tbl = &it->entries;
      found = true;
    }
  }
  if (!found) {
    // requested table does not exist:
    std::cerr << "ERROR: invalid configuration table name \"" << cnf << "\"";
    std::cerr << std::endl;
    exit(EXIT_FAILURE);
  }
  logConfigTableName(cnf);
}

void setDefaultConfigTable() {
  // Select the first entry, make sure that it is unique:
  if (conf_tables.size() == 0) {
    std::cerr << "ERROR: empty configuration table" << std::endl;
    exit(EXIT_FAILURE);
  }
  cf_tbl = &conf_tables[0].entries;
  std::string name = conf_tables[0].table_name;
  for (unsigned int t = 1; t < conf_tables.size(); t++) {
    if (conf_tables[t].table_name == name) {
      std::cerr << "ERROR: duplicated table name \"" << name << "\" ";
      std::cerr << "in file default_conf.h" << std::endl;
      exit(EXIT_FAILURE);
    }
  }
  logConfigTableName(conf_tables[0].table_name);
}

bool lookupConfig(conf_req_msg * request, conf_reply_msg * reply,
           uint32_t * line_nr)
{
  if (cf_tbl == NULL) {
    setDefaultConfigTable();
  }
  // Iterate over all table entries and match channel data with data
  // in 'request':
  uint32_t line = 0;
  for (auto it = cf_tbl->begin(); it != cf_tbl->end(); it++) {
    // Pattern-match all entries of 'request':
    if (match(request->ch_name, it->ch_name) &&
        match(request->ch_dotted_quad, it->ch_dotted_quad) &&
        match(std::to_string(request->ch_pid), it->ch_pid) &&
        match(std::to_string(request->ch_tid), it->ch_tid) &&
        match(std::to_string(request->ch_nr), it->ch_nr)) {
// #ifdef ENABLE_FOX_HANDLER
        if (it->h_type == FOX_START)
        {
            reply->log_f = it->log_f;
            return makeFoxStartReply(request, reply, it, line_nr, line);
        }
        else if (it->h_type == FOX_END)
        {
            reply->log_f = it->log_f;
            return makeFoxEndReply(reply, it, line_nr, line);
        }
        else
// #endif
        {   
            // Set up reply message:
            reply->err = CONF_LOOKUP_SUCCESS;
            reply->h_type = it->h_type;
            reply->log_f = it->log_f;
            reply->nr_parameters = it->nr_parameters;
            // Copy parameter values:
            for (uint32_t par = 0; par < reply->nr_parameters; par++)
            {
                reply->par_value[par] = it->par_value[par];
            }
            *line_nr = line;
            return true;
        }
    }
    line++;
  }
  // Falling through here means that no matching configuration entry was found
  reply->err = ERR_CONF_NOT_FOUND;
  return false;
}

bool match(std::string str, std::string expr) {
  if (expr == ".*") {
    return true;
  } else if (str == expr) {
    return true;
  }
  //std::cout << "no match for " << str << "and" << expr << std::endl;
  return false;
}

/*
#define BOOST_SPIRIT_UNICODE
#include <boost/config/warning_disable.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/classic_file_iterator.hpp>
//#include <boost/spirit/include/classic_core.hpp>


using namespace BOOST_SPIRIT_CLASSIC_NS;

namespace qi = boost::spirit::qi;
namespace ascii = boost::spirit::ascii;

template <typename Iterator>
bool parse_numbers(Iterator first, Iterator last) {
  using qi::double_;
  using qi::phrase_parse;
  using ascii::space;


  using boost::spirit::unicode::char_;
  using boost::spirit::eol;

  bool r = phrase_parse(first, //< start iterator >
                        last,  //< end iterator >
                        '"' > *( +( char_ - ( '"' | eol ) ) ) > '"' >> *(',' >> double_),   //< the parser >
                        space  //< the skip-parser >
                       );
  if (first != last) { // fail if we did not get a full match
    std::cout << "Parse failed\n";
    return false;
  }
  std::cout << "Parse succeeded\n";
  return r;
}
*/


