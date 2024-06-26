//
// Created by Albert Varaksin on 15/06/2021.
//
// clang-format off

#ifndef DIAG
#    define DIAG(LEVEL, ID, MSG)
#endif

#ifndef ERROR
#   define ERROR(ID, MSG) DIAG(Error, ID, MSG)
#endif

#ifndef WARNING
#   define WARNING(ID, MSG) DIAG(Warning, ID, MSG)
#endif

//----------------------------------------
// Parse errors
//----------------------------------------

ERROR(notAllowedTopLevelStatement,  "statements are not allowed at the top level")
ERROR(unexpectedToken,              "expected '{0}' got '{1}'")
ERROR(moduleNotFound,               "no such module '{0}'")
ERROR(failedToLoadModule,           "failed to load module '{0}'")
ERROR(expectedDeclarationAfterAttribute, "expected declaration after attributes, got '{0}'")
ERROR(unexpectedNestedDeclaration,  "unexpected nested declaration '{0}'")
ERROR(variadicArgumentNotLast,      "variadic argument must be last")
ERROR(unexpectedReturn,             "return not allowed outside main module, SUB or FUNCTION")
ERROR(expectedExpression,           "expected expression, got '{0}'")
ERROR(unsupportedExternLanguage,    "Unsupported extern langauge '{0}'")
ERROR(onlyDeclarationsInExtern,     "Only declarations permitted in EXTERN block")

//----------------------------------------
// Semantic errors
//----------------------------------------
ERROR(functionMustReturnAValue,     "FUNCTION must return a value")
ERROR(subShouldNotReturnAValue,     "SUB should not return a value")
ERROR(invalidFunctionReturnType,    "No viable conversion from returned value of type '{0}' to function return type '{1}'")
ERROR(noViableConversionToType,     "No viable conversion from '{0}' to '{1}'")
ERROR(invalidTypeOfExpression,      "Invalid TYPEOF expression")
ERROR(unexpectedTokenInTypeOf,      "Unexpected token in TYPEOF expression")
ERROR(targetNotAssignable,          "Non-object type '{0}' is not assignable")
ERROR(targetNotCallable,            "Type '{0}' is not a sub or a function")
ERROR(noMatchingSubOrFunction,      "No matching SUB or FUNCTION to call")
ERROR(unknownIdentifier,            "Unknown identifier '{0}'")
ERROR(useBeforeDefinition,          "Use of variable '{0}' before definition")
ERROR(cannotUseTypeAsBoolean,       "Cannot use '{0}' as boolean")
ERROR(unaryOperatorAppledToType,    "Unary operator '{0}' cannot be applied to type '{1}'")
ERROR(dereferencingNonPointerType,  "Dereferencing a non pointer type '{0}'")
ERROR(unexpectedContinuation,       "{0} not allowed outside FOR or DO loops")
ERROR(unexpectedContinuationTarget, "Unexpected {0} target '{1}'")
ERROR(accessingMemberOnNonUDTType,  "Accessing a member on '{0}' which is not a user defined type")
ERROR(invalidBinaryExprOperands,    "Binary operator '{0}' cannot be applied to operands of type '{1}' and '{2}'")
ERROR(invalidCompareExprOperands,   "Comparison operator '{0}' cannot be applied to operands of type '{1}' and '{2}'")
ERROR(cannotConvertOperandToType,   "Cannot convert operand of type '{0}' to {1}")
ERROR(invalidCast,                  "Invalid cast from '{0}' to '{1}'")
ERROR(invalidImplicitConversion,    "Invalid implicit conversion '{0}' to '{1}'")
ERROR(mismatchingIfExprBranchTypes, "Mismatching types in IF expression branches '{0}' and '{1}'")
ERROR(circularTypeDependency,       "Circular type dependency detected")
ERROR(undefinedType,                "Undefined type '{0}'")
ERROR(notAType,                     "'{0}' is not a type")
ERROR(symbolAlreadyDefined,         "Symbol '{0}' is already defined")
ERROR(forIteratorMustBeNumeric,     "FOR iterator type must be numeric, got {0}")

#undef DIAG
#undef ERROR
#undef WARNING
