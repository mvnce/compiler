#include "tc.h"

TC_Entry::TC_Entry(std::string in_inst, std::string in_label, std::string in_cmt)
  : label(in_label), inst(in_inst), comment(in_cmt)
  , load1(false), load2(false), load3(false)
  , store1(false), store2(false), store3(false) { ; }

void TC_Entry::AddArg(tableEntry * arg)
{
  if (arg == nullptr) return;
  // std::string var_name = arg->IsScalar() ? "s" : "a";
  std::string var_name = "";
  var_name += std::to_string(arg->GetVarID());

  if (arg->IsScalar()) {
    args.emplace_back(var_name, arg->GetVarID(), TC_Argument::ARG_SCALAR);
  } else {
    args.emplace_back(var_name, arg->GetVarID(), TC_Argument::ARG_ARRAY);
  }
}

void TC_Entry::AddArg(const std::string & arg)
{
  if (arg.size() > 0) args.emplace_back(arg, -1, TC_Argument::ARG_CONST);
}


void TC_Entry::PrintIC(std::ostream & ofs)
{
  std::stringstream out_line;

  if (label != "") { out_line << label << ": "; }
  else { out_line << ""; }

  if (inst != "") {
    out_line << inst << " ";
    for (int i = 0; i < (int) args.size(); i++) {
      out_line << args[i].str_value << " ";
    }
  }

  if (comment != "") {
    while (out_line.str().size() < 40) out_line << " ";
    out_line << "# " << comment;
  }

  ofs << out_line.str() << std::endl;
}

TC_Entry & TC_Array::AddLabel(std::string label_id, std::string cmt)
{
  TC_Entry new_entry("", label_id, cmt);
  TC_array.push_back(new_entry);
  return TC_array.back();
}


void TC_Array::PrintIC(std::ostream & ofs)
{
  ofs << "# Ouput from Dr. Charles Ofria's reference code." << std::endl;
  ofs << "  val_copy 23 regH" << std::endl;
  ofs << "  store 10023 0" << std::endl;
  for (int i = 0; i < (int) TC_array.size(); i++) {
    TC_array[i].PrintIC(ofs);
  }
}
