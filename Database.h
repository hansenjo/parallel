// Simple key/value database for parallel analysis demo
// Ole Hansen, 17-Dec-20

#ifndef PPODD_DATABASE_H
#define PPODD_DATABASE_H

#include <string>
#include <vector>
#include <optional>
#include <iostream>

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
    Clear();
    return Append(filename);
  }

  // Open and parse filename. Append to existing values read.
  // Returns number of keys found.
  size_t Append( const std::string& filename );

  // Get value for key and module. The module is essentially a key namespace.
  // If search != 0, retry with module="" if not found (global fallback).
  [[nodiscard]] std::optional<double> Get( const std::string& key,
                                           const std::string& module,
                                           int search = 0 ) const;

  // Get value for global key
  [[nodiscard]] std::optional<double> Get( const std::string& key ) const {
    return Get(key,std::string(),0);
  }

  // Clear database. Deletes all contents.
  void Clear() {
    m_items.clear();
    m_is_ready = false;
  }

  // Remove given key. Returns the key's value if it existed
  std::optional<double> Erase( const std::string& key,
                               const std::string& module );

  // Set given key to given value. Returns true if a new key was created.
  bool Set(const std::string& key, const std::string& module, double val );

  [[nodiscard]] bool   IsReady() const { return m_is_ready; }
  [[nodiscard]] size_t GetSize() const { return m_items.size(); }
  void Print( std::ostream& os = std::cout ) const;

private:
  // A key/value pair read from the database
  struct Item {
    Item() = default;
    Item( std::string _key, std::string _module, double _value )
            : key{std::move(_key)}, module{std::move(_module)}, value{_value} {}
    void print( std::ostream& os = std::cout ) const;
    std::string key;
    std::string module;
    double value{};
  } __attribute__((aligned(64)));

  // Collection of key/value pairs read from the database.
  std::vector<Item> m_items;

  bool m_is_ready{false};  // True if database successfully read

  int  ParseDBkey( const std::string& line, Item& item ) const;
  void ParseDBline( const std::string& line );
  bool Set( Item& item );
};

extern Database database;

#endif //PPODD_DATABASE_H
