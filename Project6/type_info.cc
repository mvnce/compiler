#include "type_info.h"

namespace Type {
  std::string AsString(int type) {
    switch (type) {
    case VOID: return "void";
    case VALUE: return "val";
    case CHAR: return "char";
    case STRING: return "array(char)";
    case VALUE_ARRAY: return "array(val)";
    };
    return "unknown";
  }

  int InternalType(int type) {
    if (type == STRING) return CHAR;
    if (type == VALUE_ARRAY) return VALUE;
    return VOID;
  }

  bool IsArray(int type) {
    if (type == STRING || type == VALUE_ARRAY) return true;
    return false;
  }
};
