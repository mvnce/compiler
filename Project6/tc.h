#ifndef TC_H
#define TC_H

#include <iostream>
#include <string>
#include <sstream>
#include <vector>

#include "symbol_table.h"

struct TC_Argument {
  std::string str_value;
  int var_id;

  enum Type { ARG_NONE, ARG_CONST, ARG_SCALAR, ARG_ARRAY };
  Type arg_type;

  TC_Argument() : str_value(""), var_id(-1), arg_type(ARG_NONE) { ; }
  TC_Argument(const std::string & in_str, int in_id, Type in_type)
    : str_value(in_str), var_id(in_id), arg_type(in_type) { ; }
  TC_Argument(const TC_Argument &) = default;
  
  bool IsConst() const { return arg_type == ARG_CONST; }
  bool IsScalar() const { return arg_type == ARG_SCALAR; }
  bool IsArray() const { return arg_type == ARG_ARRAY; }
};


struct TC_Entry {
  std::string label;
  std::string inst;
  std::vector<TC_Argument> args;
  std::string comment;

  bool load1;   bool load2;   bool load3;
  bool store1;  bool store2;  bool store3;

  TC_Entry(std::string in_inst="", std::string in_label="", std::string in_cmt="");
  TC_Entry(const TC_Entry &) = default;

  void AddArg(tableEntry * arg);
  void AddArg(const std::string & arg);

  void PrintIC(std::ostream & ofs);
};

class TC_Array {
private:
  std::vector<TC_Entry> TC_array;

public:
  TC_Array() { ; }
  ~TC_Array() { ; }

  TC_Entry & AddLabel(std::string label_id, std::string cmt="");

  template <typename T1, typename T2, typename T3>
  TC_Entry & Add(const std::string & inst, T1 arg1, T2 arg2, T3 arg3, std::string cmt="")
  {
    // Create the new tube code entry.
    TC_Entry new_entry(inst, "", cmt);
    
    new_entry.AddArg(arg1);
    new_entry.AddArg(arg2);
    new_entry.AddArg(arg3);
    
    TC_array.push_back(new_entry);
    return TC_array.back();
  }

  template <typename T1, typename T2>
  TC_Entry & Add(const std::string & inst, T1 arg1, T2 arg2) { return Add(inst, arg1, arg2, ""); }

  template <typename T1>
  TC_Entry & Add(const std::string & inst, T1 arg1) { return Add(inst, arg1, "", ""); }

  TC_Entry & Add(const std::string & inst) { return Add(inst, "", "", ""); }

  void PrintIC(std::ostream & ofs);
};

#endif
