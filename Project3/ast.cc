#include "ast.h"
#include <cstdlib>

extern void yyerror(std::string err_string);

/////////////////////
//  ASTNode_Base 

void ASTNode_Base::Debug(int indent)
{
  // Print this class' name
  for (int i = 0; i < indent; i++) std::cout << "  ";
  std::cout << GetName()
            << " [" << children.size() << " children]"
            << std::endl;

  // Then run Debug on the children if there are any...
  for (int i = 0; i < children.size(); i++) {
    children[i]->Debug(indent+1);
  }
}

void ASTNode_Base::TransferChildren(ASTNode_Base * in_node)
{
  for (int i = 0; i < in_node->GetNumChildren(); i++) {
    AddChild(in_node->GetChild(i));
  }
  in_node->children.resize(0);
}



/////////////////////
// ASTNode_Math2

ASTNode_Math2::ASTNode_Math2(ASTNode_Base * in1, ASTNode_Base * in2, char op)
  : ASTNode_Base(Type::VAL), math_op(op)
{
  children.push_back(in1);
  children.push_back(in2);
  if (in1->GetType() != Type::VAL || in2->GetType() != Type::VAL){
    yyerror("expected math args to both be of type int.");
    exit(1);
  }
}
