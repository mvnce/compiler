#ifndef IC_H
#define IC_H

// The classes in this file store contextual information about the intermediate
// code (IC) output, facilitating either printing or conversion to assembly.
//
// The IC_Argument class holds information about a single argument to an
// instruction.  There are three types of arguments on an IC instruction:
// CONSTANT values, SCALAR variables, or ARRAY variables.
//
// The IC_Entry class holds information about a SINGLE line of intermediate
// code.
//
// The IC_Array class holds an array of IC_Entries that make up the full
// intermediate code program.
//

#include <iostream>
#include <string>
#include <sstream>
#include <vector>

#include "symbol_table.h"

struct IC_Argument {
  std::string str_value;   // The output representation for this argument.
  int var_id;              // The ID for this argument if it is a variable.

  enum Type { ARG_NONE, ARG_CONST, ARG_SCALAR, ARG_ARRAY };
  Type arg_type;

  IC_Argument() : str_value(""), var_id(-1), arg_type(ARG_NONE) { ; }
  IC_Argument(const std::string & in_str, int in_id, Type in_type)
    : str_value(in_str), var_id(in_id), arg_type(in_type) { ; }
  IC_Argument(const IC_Argument &) = default;
  
  bool IsConst() const { return arg_type == ARG_CONST; }
  bool IsScalar() const { return arg_type == ARG_SCALAR; }
  bool IsArray() const { return arg_type == ARG_ARRAY; }
};


struct IC_Entry {
  std::string label;             // Label on this line, if any.
  std::string inst;              // Instruction name on this line, if any.
  std::vector<IC_Argument> args; // Set of arguments for this instruction
  std::string comment;           // Comment on this line, if any.

  // Do we need to load and/or store each of the arguments for this instruction?
  bool load1;   bool load2;   bool load3;
  bool store1;  bool store2;  bool store3;

  IC_Entry(std::string in_inst="", std::string in_label="", std::string in_cmt="");
  IC_Entry(const IC_Entry &) = default;

  void AddArg(tableEntry * arg);  // Add a tableEntry (i.e. variable) as an arg
  void AddArg(const std::string & arg);  // Add a constant string as an arg

  void PrintIC(std::ostream & ofs);
  void PrintTubeCode(std::ostream & ofs);
};


///////////////
//  IC_Array

class IC_Array {
private:
  std::vector<IC_Entry> ic_array;

public:
  IC_Array() { ; }
  ~IC_Array() { ; }

  IC_Entry & AddLabel(std::string label_id, std::string cmt="");

  // Add() adds an instruction to the array; the following parameters are possible:
  //
  //   inst - The name of the instruction being added (std::string)
  //   arg1 - argument 1: variable (tableEntry *) or constant (std::string)
  //   arg2 - argument 2: variable (tableEntry *) or constant (std::string)
  //   arg3 - argument 3: variable (tableEntry *) or constant (std::string)
  //   cmt  - a comment to be included in the output (std::string)
  //
  // All but the first argument are optional and must come in order.
  // Use "" to leave an argument unused.
  
  template <typename T1, typename T2, typename T3>
  IC_Entry & Add(const std::string & inst, T1 arg1, T2 arg2, T3 arg3, std::string cmt="")
  {
    // Create the new intermediate code entry.
    IC_Entry new_entry(inst, "", cmt);
    
    new_entry.AddArg(arg1);
    new_entry.AddArg(arg2);
    new_entry.AddArg(arg3);
    
    ic_array.push_back(new_entry);
    return ic_array.back();
  }

  template <typename T1, typename T2>
  IC_Entry & Add(const std::string & inst, T1 arg1, T2 arg2) { return Add(inst, arg1, arg2, ""); }

  template <typename T1>
  IC_Entry & Add(const std::string & inst, T1 arg1) { return Add(inst, arg1, "", ""); }

  IC_Entry & Add(const std::string & inst) { return Add(inst, "", "", ""); }

  void PrintIC(std::ostream & ofs);
  void PrintTubeCode(std::ostream & ofs);
};

#endif
