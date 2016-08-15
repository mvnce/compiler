#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include <iostream>
#include <map>
#include <stdio.h>

// NOTE: some of these types won't be needed until projects 4 or 5.

namespace Type {
  enum Typenames { VOID=0, VAL, CHAR, STRING, VAL_ARRAY };
}


class tableEntry {
private:
  int type_id;
  std::string name;
  int s_id;
public:
  tableEntry(int in_type) {
    type_id = in_type;
  }
  tableEntry(int in_type, const std::string & in_name) {
    type_id = in_type;
    name = in_name;
    s_id = 0;
  }
  ~tableEntry() { ; }

  int GetType() { return type_id; }
  std::string GetName() { return name; }
  int GetSID() { return s_id; }
  void SetSID(int ID) { s_id = ID; }
  void SetName(std::string in_name) { name = in_name; }
};


class symbolTable {
private:
  std::map<std::string, tableEntry *> tbl_map;
public:
  symbolTable() { ; }
  ~symbolTable() { ; }

  int GetSize() { return tbl_map.size(); }

  // Lookup will find an entry and return it.  If that entry is not in the
  // table, it will build it!
  tableEntry * Lookup(const std::string & in_name) {
    // See if we can find this name in the table.
    if (tbl_map.find(in_name) != tbl_map.end()) {
      return tbl_map.find(in_name)->second;
    }
    
    // Not found.  Return NULL!
    return NULL;
  }

  // Insert an entry into the symbol table.
  tableEntry * Insert(int in_type, const std::string & in_name) {
    tableEntry * new_entry = new tableEntry(in_type, in_name);
    tbl_map[in_name] = new_entry;
    return new_entry;
  }

};

#endif
