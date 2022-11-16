// Simple key/value database for parallel analysis demo
// Ole Hansen, 17-Dec-20

#include "Database.h"
#include <fstream>
#include <iostream>
#include <cassert>
#include <algorithm>
#include <boost/algorithm/string/trim.hpp>
#include <boost/tokenizer.hpp>

#ifndef ALL
# define ALL(c) (c).begin(), (c).end()
#endif

using namespace std;
using namespace boost;
using namespace boost::algorithm;

Database database;

Database::Database( const string& filename )
  : m_is_ready{false}
{
  Open(filename);
}

void Database::Item::print( ostream& os ) const
{
  if( !module.empty() )
    os << module << ".";
  os << key << " = " << value << endl;
}

// Read database file, if it exists. Add its key/value pairs to internal cache.
int Database::Append( const string& filename )
{
  ifstream ifs(filename);
  if( ifs ) {
    int lineno = 0;
    string line;
    while( getline(ifs, line) ) {
      ++lineno;
      // Blank line or comment?
      if( string::size_type pos = line.find('#'); pos != string::npos )
        line.erase(pos);
      trim(line);
      if( line.empty() )
        continue;

      // Extract key/value pairs. Each non-comment line in the database file
      // should have the format
      //   module.key = value
      // where "value" must be convertible to double. Trailing garbage
      // results in an error.
      try {
        ParseDBline(line);
      }
      catch( const bad_db_syntax& e ) {
        cerr << "Bad database syntax on line " << lineno << ": " << e.what() << endl;
        return -1;
      }
      catch( const std::exception& e ) {
        cerr << "Bad database line " << lineno << ": " << line << ": " << e.what() << endl;
        return -1;
      }
    }
  }
  m_is_ready = true;
  return static_cast<int>(m_items.size());
}

// Clear database. Deletes all contents.
void Database::Clear()
{
  m_items.clear();
  m_items.shrink_to_fit();
  m_is_ready = false;
}

std::optional<double> Database::Get( const string& key, const string& module,
                                     int search ) const
{
  optional<double> ret;
  auto it = find_if( ALL(m_items), [&key,&module](const Item& e) {
    return (e.key == key && e.module == module);
  });
  if( it == m_items.end() && search != 0 ) {
    it = find_if( ALL(m_items), [&key](const Item& e) {
      return (e.module.empty() && e.key == key);
    });
  }
  if( it != m_items.end() )
    ret = it->value;
  return ret;
}

std::optional<double> Database::Erase( const string& key, const string& module )
{
  optional<double> ret;
  auto it = find_if( ALL(m_items), [&key,&module](const Item& e) {
    return (e.key == key && e.module == module);
  });
  if( it != m_items.end() ) {
    ret = it->value;
    m_items.erase(it);
  }
  return ret;
}

bool Database::Set( const string& key, const string& module, double val )
{
  bool ret = false;
  auto it = find_if( ALL(m_items), [&key,&module](const Item& e) {
    return (e.key == key && e.module == module);
  });
  if( it == m_items.end() ) {
    ret = true;
    m_items.emplace_back(key,module,val);
  } else {
    it->value = val;
  }
  return ret;
}

bool Database::Set( Item& item )
{
  bool ret = false;
  auto it = find_if( ALL(m_items), [&item](const Item& e) {
    return (e.key == item.key && e.module == item.module);
  });
  if( it == m_items.end() ) {
    m_items.push_back(std::move(item));
    ret = true;
  } else {
    swap(*it,item);
  }
  return ret;
}

void Database::Print( ostream& os ) const
{
  for_each( ALL(m_items), [&os](auto& e) {
    os << "DB: ";
    e.print(os);
  });
}

// Parse the full key into module.key parts.
// Returns the number of components found between dots.
int Database::ParseDBkey( const string& fullkey, Item& item )
{
  char_separator<char> dot(".");
  tokenizer<char_separator<char>> parts(fullkey, dot);
  int j = 0;
  for( const auto& p : parts ) {
    if( j == 0 ) {
      item.module = p;
    } else if( j == 1 ) {
      item.key = p;
    } else if( j > 1 ) {
      // The key itself may contain dots
      item.key += ".";
      item.key.append(p);
    }
    ++j;
  }
  // Put a component without a module prefix into the key field
  // instead the module field. Example:
  //   "debug = 1" -> module = "", key = "debug", value = "1.0"
  if( j == 1 ) {
    assert(item.key.empty());  // Else logic error in parser above
    item.key.swap(item.module);
  }
  return j;
}

// Parse database line and put results in member variable 'm_items'
void Database::ParseDBline( const string& line )
{
  Item item;
  char_separator<char> whitespace(" \t");
  tokenizer<char_separator<char>> tokens(line, whitespace);
  int i = 0;
  for( const auto& t : tokens ) {
    if( i == 0 ) {
      if( t == "=" )
        throw bad_db_syntax(line);
      // Parse module.key part
      if( ParseDBkey(t,item) == 0 )
        throw bad_db_syntax(line);
    } else if( i == 1 ) {
      if( t != "=" )
        throw bad_db_syntax(line);
    } else if( i == 2 ) {
      item.value = stod(t);
    }
    ++i;
  }
  if( i != 3 )
    throw bad_db_syntax(line);

  // Check if this item already exists
  bool ins = Set(item);
  if( !ins ) {
    cerr << "Warning: overwriting existing database key: ";
    item.print(cerr);
  }
}
