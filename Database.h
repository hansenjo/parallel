// Simple key/value database for parallel analysis demo
// Ole Hansen, 17-Dec-20

#ifndef PPODD_DATABASE_H
#define PPODD_DATABASE_H

#include <string>
#include <vector>
#include <optional>

class Database {
public:
  // Exception that can be thrown during reading
  class bad_db_syntax : public std::runtime_error {
  public:
    explicit bad_db_syntax( const std::string& what_arg )
            : std::runtime_error(what_arg) {}
  };

  // Construct empty database
  Database() = default;

  // Construct new database and open and parse filename
  explicit Database( const std::string& filename );

  // Open and parse filename. Deletes any previous values read.
  // Returns number of keys found.
  size_t Open( const std::string& filename ) {
    Close();
    return Append(filename);
  }

  // Open and parse filename. Append to existing values read.
  // Returns number of keys found.
  size_t Append( const std::string& filename );

  // Get value for key and module. The module is essentially a key namespace.
  // If search != 0, retry with module="" if not found (global fallback).
  std::optional<double> Get( const std::string& key, const std::string& module, int search = 0 );

  // Get value for global key
  std::optional<double> Get( const std::string& key ) {
    return Get(key,std::string(),0);
  }

  void Close() {
    m_items.clear();
    m_is_ready = false;
  }

  [[nodiscard]] bool   IsReady() const { return m_is_ready; }
  [[nodiscard]] size_t GetSize() const { return m_items.size(); }

private:
  // A key/value pair read from the database
  struct Item {
    std::string module;
    std::string key;
    double value{};
  } __attribute__((aligned(64)));

  // Collection of key/value pairs from read from the database.
  // Derived classes may extract their parameters from these items.
  std::vector<Item> m_items;

  bool m_is_ready{false};  // True if database successfully read

  int  ParseDBkey( const std::string& line, Item& item );
  void ParseDBline( const std::string& line );
};

extern Database database;

#endif //PPODD_DATABASE_H
