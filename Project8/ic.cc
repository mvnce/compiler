#include "ic.h"

IC_Entry::IC_Entry(std::string in_inst, std::string in_label, std::string in_cmt)
  : label(in_label), inst(in_inst), comment(in_cmt)
  , load1(false), load2(false), load3(false)
  , store1(false), store2(false), store3(false)
{
  block = -1;
  local_arr = std::vector<bool>(3,false);

  if (inst == "") { ; }
  else if (inst == "val_copy")   { load1 = true; store2 = true; }
  else if (inst == "add")        { load1 = true; load2 = true; store3 = true; }
  else if (inst == "sub")        { load1 = true; load2 = true; store3 = true; }
  else if (inst == "mult")       { load1 = true; load2 = true; store3 = true; }
  else if (inst == "div")        { load1 = true; load2 = true; store3 = true; }
  else if (inst == "test_less")  { load1 = true; load2 = true; store3 = true; }
  else if (inst == "test_gtr")   { load1 = true; load2 = true; store3 = true; }
  else if (inst == "test_equ")   { load1 = true; load2 = true; store3 = true; }
  else if (inst == "test_nequ")  { load1 = true; load2 = true; store3 = true; }
  else if (inst == "test_gte")   { load1 = true; load2 = true; store3 = true; }
  else if (inst == "test_lte")   { load1 = true; load2 = true; store3 = true; }
  else if (inst == "jump")       { load1 = true; }
  else if (inst == "jump_if_0")  { load1 = true; load2 = true; }
  else if (inst == "jump_if_n0") { load1 = true; load2 = true; }
  else if (inst == "nop")        { ; }
  else if (inst == "random")     { load1 = true; store2 = true; }
  else if (inst == "out_val")    { load1 = true; }
  else if (inst == "out_float")  { load1 = true; }
  else if (inst == "out_char")   { load1 = true; }
  else if (inst == "push")       { load1 = true; }
  else if (inst == "pop")        { store1 = true; }
  
  else if (inst == "ar_get_idx") { load1 = true; load2 = true;  store3 = true; }
  else if (inst == "ar_set_idx") { load1 = true; load2 = true;  load3 = true; }
  else if (inst == "ar_get_siz") { load1 = true; store2 = true; }
  else if (inst == "ar_set_siz") { load1 = true; load2 = true;  }
  else if (inst == "ar_push")    { load1 = true; }
  else if (inst == "ar_pop")     { store1 = true; }
  else if (inst == "ar_copy")    { load1 = true; store2 = true; }
  else {
    std::cerr << "Internal Compiler Error! Unknown instruction '"
         << inst
         << "' in IC_Entry::IC_Entry(): " << inst << std::endl;
  }
}


// Add a tableEntry (i.e. variable) as an argument to this entry.
void IC_Entry::AddArg(tableEntry * arg)
{
  if (arg == nullptr) return;   // If this is not a real arg, stop here.
  std::string var_name = arg->IsScalar() ? "s" : "a";  // array or scalar?
  var_name += std::to_string(arg->GetVarID());         // Whats it's id number?
  
  if (arg->IsScalar()) {
    args.emplace_back(var_name, arg->GetVarID(), IC_Argument::ARG_SCALAR);
  } else {
    args.emplace_back(var_name, arg->GetVarID(), IC_Argument::ARG_ARRAY);
  }
}


// Add a tableFunction (i.e. return variable) as an argument to this entry.
void IC_Entry::AddArg(tableFunction * arg)
{
  if (arg == nullptr) return;   // If this is not a real arg, stop here.
  std::string return_name = arg->ReturnIsScalar() ? "s" : "a";  // array or scalar?
  return_name += std::to_string(arg->GetReturnID());         // Whats it's id number?
  
  if (arg->ReturnIsScalar()) {
    args.emplace_back(return_name, arg->GetReturnID(), IC_Argument::ARG_SCALAR);
  } else {
    args.emplace_back(return_name, arg->GetReturnID(), IC_Argument::ARG_ARRAY);
  }
}


// Add a constant string as an argument to this entry.
void IC_Entry::AddArg(const std::string & arg)
{
  if (arg.size() > 0) args.emplace_back(arg, -1, IC_Argument::ARG_CONST);
}


// void IC_Entry::PrintIC(std::ostream & ofs)
// {
//   std::stringstream out_line;

//   // If there is a label, include it in the output.
//   if (label != "") { out_line << label << ": "; }
//   else { out_line << "  "; }

//   // If there is an instruction, print it and all its arguments.
//   if (inst != "") {
//     out_line << inst << " ";
//     for (int i = 0; i < (int) args.size(); i++) {
//       out_line << args[i].str_value << " ";
//     }
//   }

//   // If there is a comment, print it!
//   if (comment != "") {
//     while (out_line.str().size() < 40) out_line << " "; // Align comments for easy reading.
//     out_line << "# " << comment;
//   }

//   ofs << out_line.str() << std::endl;
// }

void IC_Entry::PrintIC(std::ostream & ofs)
{
  std::stringstream out_line;

  // If there is a label, include it in the output.
  if (label != "") { out_line << label << ": "; }
  else { out_line << "  "; }

  // If there is an instruction, print it and all its arguments.
  if (inst != "") {
    out_line << inst << " ";
    for (int i = 0; i < (int) args.size(); i++) {
      out_line << args[i].str_value << " ";
    }
  }

  if (comment == "") {
    while (out_line.str().size() < 40) out_line << " ";
    out_line << "# block#: " << block << "\tlocal: "<< LocalString();
  }
  // If there is a comment, print it!
  if (comment != "") {
    while (out_line.str().size() < 40) out_line << " "; // Align comments for easy reading.
    out_line << "# block#: " << block << "\tlocal: " << LocalString() << "\t" << comment;
  }

  ofs << out_line.str() << std::endl;
}


void IC_Entry::PrintTubeCode(std::ostream & ofs, std::vector<TC_Reg> & registers)
{
  // If this entry has a label, print it!
  if (label != "") ofs << label << ":" << std::endl;

  // If we have an instruction, load any values it needs into registers.
  if (inst != "") {
    ofs << "### Converting: " << inst;
    for (int i = 0; i < (int) args.size(); i++) ofs << " " << args[i].str_value;
    ofs << std::endl;

    // Setup Loads
    if (load1  && !args[0].IsConst()) {
      ofs << "  load " << args[0].var_id << " regA" << std::endl;
    }
    if (load2  && !args[1].IsConst()) {
      ofs << "  load " << args[1].var_id << " regB" << std::endl;
    }
    if (load3  && !args[2].IsConst()) {
      ofs << "  load " << args[2].var_id << " regC" << std::endl;
    }
  }

  // If there is an instruction, print it and all its arguments.
  std::stringstream out_line;
  if (inst == "ar_get_idx") {            // *******************************************************
    ofs << "  add regA 1 regD" << std::endl;
    if (args[1].IsConst()) ofs << "  add regD " << args[1].str_value << " regD" << std::endl;
    else ofs << "  add regD regB regD" << std::endl;
    ofs << "  load regD regC";

  } else if (inst == "ar_set_idx") {     // *******************************************************
    ofs << "  add regA 1 regD" << std::endl;
    if (args[1].IsConst()) ofs << "  add regD " << args[1].str_value << " regD" << std::endl;
    else ofs << "  add regD regB regD" << std::endl;
    if (args[2].IsConst()) ofs << "  store " << args[2].str_value << " regD";
    else ofs << "  store regC regD";
  } else if (inst == "ar_get_siz") {     // *******************************************************
    ofs << "  load regA regB";

  } else if (inst == "ar_set_siz") {     // *******************************************************
    static int label_id = 0;
    std::stringstream do_copy_label, start_label, end_label;
    do_copy_label << "ar_resize_do_copy_" << label_id++;
    start_label << "ar_resize_start_" << label_id++;
    end_label << "ar_resize_end_" << label_id++;

    std::string size_in("regB");
    if (args[1].IsConst()) size_in = args[1].str_value;

    // Start by calculating old_array_size in "regC"
    ofs << "  val_copy 0 regC                       # Default old array size to 0 if uninitialized." << std::endl;
    ofs << "  jump_if_0 regA " << do_copy_label.str() << "    # Leave 0 size (nothing to copy) for uninitialized arrays." << std::endl;  // Jump if original array is uninitialized
    ofs << "  load regA regC                        # Load old array size into regC" << std::endl;

    // Test if old_array_size ("regC") >= new_array_size (size_in)
    ofs << "  test_gtr " << size_in << " regC regD               # regD = new_size > old_size?" << std::endl;
    ofs << "  jump_if_n0 regD " << do_copy_label.str() << "   # Jump to array copy if new size is bigger than old size." << std::endl;  // If not, proceed to move...
    ofs << "  store " << size_in << " regA                       # Otherwise, replace old size w/ new size.  Done." << std::endl;
    ofs << "  jump " << end_label.str() << "                  # Skip copying contents." << std::endl;

    // If we made it here, we need to copy the array and have original size in "regC" and new size in size_in
    ofs << do_copy_label.str() << ":" << std::endl;

    // Set up memory for the new array.
    ofs << "  load 0 regD                           # Set regD = free mem position" << std::endl;
    ofs << "  store regD " << args[0].var_id << "                          # Set indirect pointer to new mem pos." << std::endl;
    ofs << "  store " << size_in << " regD                       # Store new size at new array start" << std::endl;
    ofs << "  add regD 1 regE                       # Set regE = first pos. in new array" << std::endl;
    ofs << "  add regE " << size_in << " regE                    # Set regE = new free mem position" << std::endl;
    ofs << "  store regE 0                          # Store new free memory at pos. zero" << std::endl;

    // Figure out where to stop copying in E
    ofs << "  add regA regC regE                    # Set regE = the last index to be copied" << std::endl;

    // Copy the array over from A to D.
    ofs << start_label.str() << ":" << std::endl;
    ofs << "  add regA 1 regA                       # Increment pointer for FROM array" << std::endl;
    ofs << "  add regD 1 regD                       # Increment pointer for TO array" << std::endl;
    ofs << "  test_gtr regA regE regF               # If we are done copying, jump to end of loop" << std::endl;
    ofs << "  jump_if_n0 regF " << end_label.str() << std::endl;
    ofs << "  mem_copy regA regD                    # Copy the current index." << std::endl;
    ofs << "  jump " << start_label.str() << std::endl;
    ofs << end_label.str() << ":" << std::endl;

  } else if (inst == "ar_copy") {     // *******************************************************
    static int label_id = 0;
    std::stringstream do_copy_label, start_label, end_label;
    do_copy_label << "ar_do_copy_" << label_id++;
    start_label << "ar_copy_start_" << label_id++;
    end_label << "ar_copy_end_" << label_id++;

    // "regA" holds the pointer the array to copy from.  If it's zero, set array2 to zero and stop.
    ofs << "  jump_if_n0 regA " << do_copy_label.str() << "          # Jump if we actually have something to copy." << std::endl;
    ofs << "  val_copy 0 regB                             # Set indirect pointer to new mem pos." << std::endl;
    ofs << "  jump " << end_label.str() << std::endl;

    // If we made it here, we need to copy the array.
    ofs << do_copy_label.str() << ":" << std::endl;

    // Set up memory for the new array.
    ofs << "  load 0 regD                           # Set regD = free mem position" << std::endl;
    ofs << "  val_copy regD regB                    # Set indirect pointer to new mem pos." << std::endl;
    ofs << "  load regA regE                        # Set regE = Array size." << std::endl;
    ofs << "  add regD 1 regF                       # Set regF = first pos. in new array" << std::endl;
    ofs << "  add regF regE regF                    # Set regF = new free mem position" << std::endl;
    ofs << "  store regF 0                          # Store new free memory at pos. zero" << std::endl;

    // Copy the array over from A to B; increment until B==D
    ofs << start_label.str() << ":" << std::endl;
    ofs << "  test_equ regD regF regG               # If we are done copying, jump to end of loop" << std::endl;
    ofs << "  jump_if_n0 regG " << end_label.str() << std::endl;
    ofs << "  mem_copy regA regD                    # Copy the current index." << std::endl;
    ofs << "  add regA 1 regA                       # Increment pointer for FROM array" << std::endl;
    ofs << "  add regD 1 regD                       # Increment pointer for TO array" << std::endl;
    ofs << "  jump " << start_label.str() << std::endl;
    ofs << end_label.str() << ":" << std::endl;

  } else if (inst == "push") {        // *******************************************************
    // Assume that regH points to the top of the stack.
    if (args[0].IsConst()) ofs << "  store " << args[0].str_value << " regH";
    else ofs << "  store regA regH";
    ofs << "                       # Save loaded value onto the stack." << std::endl;
    ofs << "  add regH 1 regH                       # Increment stack to next mem position" << std::endl;

  } else if (inst == "pop") {         // *******************************************************
    // Assume that regH points to the top of the stack.
    ofs << "  sub regH 1 regH                       # Decrement stack to prev mem position" << std::endl;
    ofs << "  load regH regA                        # Load stored value from the stack." << std::endl;
  } else if (inst == "ar_push") {        // *******************************************************
    // Assume that regH points to the top of the stack.
    if (args[0].IsConst()) ofs << "  store " << args[0].str_value << " regH";
    else ofs << "  store regA regH";
    ofs << "                       # Save loaded value onto the stack." << std::endl;
    ofs << "  add regH 1 regH                       # Increment stack to next mem position" << std::endl;

  } else if (inst == "ar_pop") {         // *******************************************************
    // Assume that regH points to the top of the stack.
    ofs << "  sub regH 1 regH                       # Decrement stack to prev mem position" << std::endl;
    ofs << "  load regH regA                        # Load stored value from the stack." << std::endl;
  }

  // All other instructions are converted in the same way.
  else if (inst != "") {
    out_line << "  " << inst << " ";
    if (args.size() >= 1) {
      if (args[0].IsConst()) out_line << args[0].str_value << " ";
      else out_line << "regA ";
    }
    if (args.size() >= 2) {
      if (args[1].IsConst()) out_line << args[1].str_value << " ";
      else out_line << "regB ";
    }
    if (args.size() >= 3) {
      if (args[2].IsConst()) out_line << args[2].str_value << " ";
      else out_line << "regC ";
    }
  }

  // If there is a comment, print it!
  if (comment != "") {
    while (out_line.str().size() < 40) out_line << " ";   // Align comments for easy reading.
    out_line << "# " << comment;
  }

  // Print out the main instrution information if there is any...
  if (inst != "" || comment != "") ofs << out_line.str() << std::endl;

  // if (store1) {
  //   if (registers[0].val == "") { registers[0].val = args[0].var_id; }
  //   else { ofs << "  store regA " << args[0].var_id << std::endl; }
    
  // }
  // if (store2) {
  //   if (registers[1].val == "") { registers[1].val = args[1].var_id; }
  //   else { ofs << "  store regB " << args[1].var_id << std::endl; }
  // }
  // if (store3) { 
  //   if (registers[2].val == "") { registers[2].val = args[2].var_id; }
  //   else { ofs << "  store regC " << args[2].var_id << std::endl; }
  // }

  if (store1) { ofs << "  store regA " << args[0].var_id << std::endl; }
  if (store2) { ofs << "  store regB " << args[1].var_id << std::endl; }
  if (store3) { ofs << "  store regC " << args[2].var_id << std::endl; }
}



//////////////
// IC_Array

IC_Entry & IC_Array::AddLabel(std::string label_id, std::string cmt)
{
  IC_Entry new_entry("", label_id, cmt);
  ic_array.push_back(new_entry);
  return ic_array.back();
}


void IC_Array::PrintIC(std::ostream & ofs)
{
  ofs << "# Ouput from Dr. Charles Ofria's reference code." << std::endl;
  for (int i = 0; i < (int) ic_array.size(); i++) {
    ic_array[i].PrintIC(ofs);
  }
}

void IC_Array::PrintTubeCode(std::ostream & ofs)
{
  const int stack_start = 10000;
  const int stack_size = 10000;
  const int free_start = stack_start + stack_size;

  std::vector<TC_Reg> registers;
  TC_Reg reg0;
  reg0.name ="regA";
  TC_Reg reg1;
  reg1.name ="regB";
  TC_Reg reg2;
  reg2.name ="regC";
  TC_Reg reg3;
  reg3.name ="regD";

  registers.push_back(reg0);
  registers.push_back(reg1);
  registers.push_back(reg2);
  registers.push_back(reg3);

  ofs << "#=-=-= Ouput from Dr. Charles Ofria's sample compiler." << std::endl
      << "  val_copy " << stack_start << " regH                      # Setup regH to point to start of stack." << std::endl
      << "  store " << free_start << " 0                            # Store next free memory at 0" << std::endl;

  // Convert each line of intermediate code, one at a time.
  for (int i = 0; i < (int) ic_array.size(); i++) {
    ic_array[i].PrintTubeCode(ofs, registers);
  }
}


//=================== new function =======================

std::string IC_Entry::LocalString() {
  std::string outStr = "";
  for (int i = 0; i < (int) local_arr.size(); i++) { outStr += std::to_string(local_arr[i]) + " "; }
  return outStr;
}


bool IC_Entry::Find(std::string str_val) {
  for (int i = 0; i < (int) args.size(); i++) {
    if (args[i].str_value == str_val) { return true; }
  }
  return false;
}


void IC_Array::AddBlock() {
  int cnt = 1;
  for (int i = 0; i < (int) ic_array.size(); i++) {
    ic_array[i].block = cnt;
    if (ic_array[i].inst == "jump") { cnt++; }
  }
}


void IC_Array::AddLocal() {
  // 1. itrate ic_array
  // 2. iterate current entry args
  // 3. find last used

  for (int i = ic_array.size()-1; i >= 0; i--) {
    int block1 = ic_array[i].block;
    std::vector<IC_Argument> args1 = ic_array[i].args;

    for(int n = 0; n < (int) args1.size(); n++) {
      std::string str_val2 = args1[n].str_value;
      for(int j = i-1; j >=0; j--) {
        int block2 = ic_array[j].block;

        if (block1 != block2) { break; }
        if (ic_array[j].Find(str_val2)) { ic_array[i].local_arr[n] = true; }
      }
    }
  }
}


// bool IC_Array::IsConstantOpt() {

//   for (int i = ic_array.size()-1; i >= 0; i--)  {
//     int block1 = ic_array[i].block;
//     std::vector<IC_Argument> args1 = ic_array[i].args;
//     std::string inst1 = ic_array[i].inst;

//     // // make sure the inst is val_copy and the 1st arg is a number!
//     // if (inst1 == "val_copy" && args1[0].str_value[0]!='s') {
//     //   //make sure the number will not be rewrite and does not involve any push or push inst
//     //   for (int n = 0; n < ic_array.size(); n ++) {
//     //     std::cout << ic_array[n].args[0] << std::endl;
//     //     std::cout << args1[0].str_value[1] << std::endl;
//     //     if (ic_array[n].inst != "pop" && 
//     //         ic_array[n].inst != "push" &&
//     //         ic_array[n].args[0] != args1[0].str_value[1]
//     //       ) {}
//     //   }
//     // }
//   }
//   return false;
// }


// bool IC_Array::IsAlgebraicOpt() {
//   for (int i = ic_array.size()-1; i >= 0; i--)  {
//     int block1 = ic_array[i].block;
//     std::vector<IC_Argument> args1 = ic_array[i].args;
//   }
//   return false;
// }


void IC_Array::AlgebraicOpt() {
  // according to test file #1
  for (int i = 0; i <= (int) ic_array.size() -1; i++) {
    // eliminating mathematical calculation
    if (ic_array[i].inst == "val_copy" && (ic_array[i].args[0].str_value[0]!='s') &&
         (ic_array[i+1].inst == "add" ||
          ic_array[i+1].inst == "div" ||
          ic_array[i+1].inst == "sub" ||
          ic_array[i+1].inst == "mult" ||
          ic_array[i+1].inst == "test_less" ||
          ic_array[i+1].inst == "test_gtr" ||
          ic_array[i+1].inst == "test_equ" ||
          ic_array[i+1].inst == "test_nequ" ||
          ic_array[i+1].inst == "test_gte" ||
          ic_array[i+1].inst == "test_lte")) {
      ic_array[i+1].args[1].str_value = ic_array[i].args[0].str_value;
      // std::cout << ic_array[i].args[0].str_value[0] << std::endl;
      ic_array[i+1].args[1].arg_type = ic_array[i].args[0].arg_type;
      ic_array[i].inst = "";
      ic_array[i].store2 = false;
      ic_array[i].args.clear();
    }

    // elmination random calculation
    if (ic_array[i].inst == "val_copy" && (ic_array[i].args[0].str_value[0]!='s') &&
         (ic_array[i+1].inst == "random")) {

      ic_array[i+1].args[0].str_value = ic_array[i].args[0].str_value;
      // std::cout << ic_array[i].args[0].str_value[0] << std::endl;
      ic_array[i+1].args[0].arg_type = ic_array[i].args[0].arg_type;
      ic_array[i].inst = "";
      // ic_array[i].load1 = false;
      ic_array[i].store2 = false;
      ic_array[i].args.clear();
    }

    // elmination
    // val_copy 10 s4
    // ar_set_siz a1 s4
    if (ic_array[i].inst == "val_copy" && (ic_array[i].args[0].str_value[0]!='s') && (ic_array[i+1].inst == "ar_set_siz")) {
      ic_array[i+1].args[1] = ic_array[i].args[0];
      ic_array[i].inst = "";
      ic_array[i].args.clear();
      ic_array[i].load1 = false;
      ic_array[i].store2 = false;
    }
  }

  // according to test file #4 
  for (int i = 0; i < (int) ic_array.size(); i++) {
    // change add 0 to val_copy
    if (ic_array[i].inst == "add" && ic_array[i].args[1].str_value == "0") {
      ic_array[i].inst = "val_copy";
      ic_array[i].load2 = false;
      ic_array[i].store3 = false;
      ic_array[i].store2 = true;

      ic_array[i].args[1] = ic_array[i].args[2];
      ic_array[i].args.pop_back();
    }
    // change mult 1 to valcopy
    if (ic_array[i].inst == "mult" && ic_array[i].args[1].str_value == "1") {
      ic_array[i].inst = "val_copy";
      ic_array[i].load2 = false;
      ic_array[i].store3 = false;
      ic_array[i].store2 = true;

      ic_array[i].args[1] = ic_array[i].args[2];
      ic_array[i].args.pop_back();
    }

    if (ic_array[i].inst == "mult" && ic_array[i].args[1].str_value == "0") {
      ic_array[i].inst = "val_copy";
      ic_array[i].load2 = false;
      ic_array[i].store3 = false;
      ic_array[i].store2 = true;

      ic_array[i].args[0] = ic_array[i].args[1];
      ic_array[i].args[1] = ic_array[i].args[2];
      ic_array[i].args.pop_back();
    }
  }
  // eliminating useless val_copy
  for (int i = 0; i < (int) ic_array.size(); i++) {

    if (ic_array[i].inst == "val_copy") {

      for (int j = i+1; j < (int) ic_array.size(); j++) {
        if (ic_array[j].inst == "") { continue; }

        else if (ic_array[j].inst != "val_copy") { break; }

        else if (ic_array[j].inst == "val_copy" && (ic_array[j].args[0].str_value == ic_array[i].args[1].str_value)) {

          ic_array[j].args[0] = ic_array[i].args[0];
          ic_array[j].store2 = true;
          ic_array[i].inst = "";
          ic_array[i].store2 = false;
          ic_array[i].args.clear();
        }
      }
    }
  }

  // for (int i = 0; i < (int) ic_array.size() -1; i++) {
  //   if (ic_array[i].inst == "val_copy" && (ic_array[i+1].inst == "out_char")) {
  //     ic_array[i+1].args[0] = ic_array[i].args[0];
  //     ic_array[i].inst = "";
  //     ic_array[i].store2 = false;
  //     ic_array[i].args.clear();
  //   }
  // }

  // eliminating
  //  add s1 1 s16
  //  val_copy s16 s1
  for (int i = 0; i < (int) ic_array.size()-1; i++) {
    if ((ic_array[i].inst == "add" ||
         ic_array[i+1].inst == "div" ||
         ic_array[i+1].inst == "sub" ||
         ic_array[i+1].inst == "mult") &&
      (ic_array[i].args[1].str_value[0] != 's') && (ic_array[i+1].inst == "val_copy")) {
      ic_array[i].args[2] = ic_array[i+1].args[1];
      ic_array[i+1].inst = "";
      ic_array[i+1].args.clear();
      ic_array[i+1].load1 = false;
      ic_array[i+1].store2 = false;

    }
  }
}

