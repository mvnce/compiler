#ifndef TYPE_INFO_H
#define TYPE_INFO_H

#include <string>

namespace Type {
  enum TypeNames { VOID=0, VALUE, CHAR, STRING, VALUE_ARRAY };
  
  std::string AsString(int type); // Convert the internal type to a string like "int"
  int InternalType(int type);     // Determine the internal type for arrays (or return Type::VOID)
  bool IsArray(int type);         // Determine if the type passed in is an array
  inline bool IsScalar(int type) { return !IsArray(type); }
};

#endif
