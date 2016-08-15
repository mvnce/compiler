#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include <iostream>
#include <map>
#include <stdio.h>
#include <vector>

using namespace std;

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
  std::vector<std::map<std::string, tableEntry *> > tbl_l;
  int curr_scope;
  std::vector<int> while_l;

public:
  symbolTable() {
    curr_scope = 0;
    std::map<std::string, tableEntry *> tbl_map; // initial table map
    tbl_l.push_back(tbl_map);
  }
  ~symbolTable() { ; }

  int GetSize() { return tbl_l[curr_scope].size(); }

  // Lookup will find an entry and return it.  If that entry is not in the
  // table, it will build it!
  tableEntry * Lookup(const std::string & in_name) {
    // See if we can find this name in the table.
    
    if (tbl_l[curr_scope].find(in_name) != tbl_l[curr_scope].end()) {
        return tbl_l[curr_scope].find(in_name)->second;
      }
    
    // Not found.  Return NULL!
    return NULL;
  }

  tableEntry * DeepLookup(const std::string & in_name) {
    if (curr_scope > 0) {
      for (int i = curr_scope-1; i >= 0; i--) {
        if (tbl_l[i].find(in_name) != tbl_l[i].end()) {
          return tbl_l[i].find(in_name)->second;
        }
      }
    }

    return NULL;
  }

  void IncreaseScope() {     std::map<std::string, tableEntry *> tbl_map;
    tbl_l.push_back(tbl_map);
    curr_scope++;
  }

  void DecreaseScope() {
    tbl_l.pop_back();
    curr_scope--; 
  }

  // Insert an entry into the symbol table.
  tableEntry * Insert(int in_type, const std::string & in_name) {
    tableEntry * new_entry = new tableEntry(in_type, in_name);
    tbl_l[curr_scope][in_name] = new_entry;
    return new_entry;
  }

  void PushWhile (int num) { while_l.push_back(num); }

  int GetWhileSize() { return while_l.size(); }

  int GetWhileId() { return while_l[while_l.size()]; }

  int PopWhile () {
    int tmp = while_l[while_l.size()-1];
    while_l.pop_back();
    return tmp;
  }

};

#endif
