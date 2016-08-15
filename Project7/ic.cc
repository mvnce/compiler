#include "ic.h"

IC_Entry::IC_Entry(std::string in_inst, std::string in_label, std::string in_cmt)
  : label(in_label), inst(in_inst), comment(in_cmt)
  , load1(false), load2(false), load3(false)
  , store1(false), store2(false), store3(false)
{
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

  else if (inst == "ar_get_idx") { load1 = true; load2 = true;  store3 = true; }
  else if (inst == "ar_set_idx") { load1 = true; load2 = true;  load3 = true; }
  else if (inst == "ar_get_siz") { load1 = true; store2 = true; }
  else if (inst == "ar_set_siz") { load1 = true; load2 = true;  }
  else if (inst == "ar_copy")    { load1 = true; store2 = true; }
  else if (inst == "ar_push")    {  }
  else if (inst == "ar_pop")     {  }
  else if (inst == "push")       {  }
  else if (inst == "pop")        {  }
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


// Add a constant string as an argument to this entry.
void IC_Entry::AddArg(const std::string & arg)
{
  if (arg.size() > 0) args.emplace_back(arg, -1, IC_Argument::ARG_CONST);
}


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

  // If there is a comment, print it!
  if (comment != "") {
    while (out_line.str().size() < 40) out_line << " "; // Align comments for easy reading.
    out_line << "# " << comment;
  }

  ofs << out_line.str() << std::endl;
}


void IC_Entry::PrintTubeCode(std::ostream & ofs)
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
    else if (load1  && args[0].IsScalar())
    {
      ofs << "  load " << args[0].var_id << " regA" << std::endl;
      ofs << "  val_copy " << "regA regB" << std::endl;
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

  }
  else if (inst == "push") {
    if (args[0].IsScalar())
   {
     ofs << "  load " << args[0].var_id << " regA" << std::endl;
     ofs << "  store regA regH" << std::endl;
   }
   else if (args[0].IsConst())
   {
     ofs << "  store " << args[0].str_value << " regH" << std::endl;
   }
   
   ofs << "  add regH 1 regH" << std::endl;
  }
  else if (inst == "pop") {
    ofs << "  sub regH 1 regH                       # Decrement stack to prev mem position" << std::endl;
    ofs << "  load regH regA                        # Load stored value from the stack." << std::endl;
    ofs << "  store regA " << args[0].var_id << std::endl;
  }

  else if (inst == "ar_pop") {
    ofs << "  sub regH 1 regH                       # Decrement stack to prev mem position" << std::endl;
    ofs << "  load regH regA                        # Load stored value from the stack." << std::endl;
    ofs << "" << std::endl;
    ofs << "  store regA " << args[0].var_id << std::endl;
  }

  else if (inst == "ar_push") {
    ofs << "  load " << args[0].var_id << " regA" << std::endl;
    ofs << "  store regA regH" << std::endl;
    ofs << "  add regH 1 regH" << std::endl;
  }
  
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
  const int free_start = 10100;

  ofs << "#=-=-= Ouput from Dr. Charles Ofria's sample compiler." << std::endl;
  ofs << "  val_copy " << "100" << " regH" << std::endl;
  ofs << "  store " << free_start << " 0"
      << "                         # Store next free memory at 0" << std::endl;

  // Convert each line of intermediate code, one at a time.
  for (int i = 0; i < (int) ic_array.size(); i++) {
    ic_array[i].PrintTubeCode(ofs);
  }
}
