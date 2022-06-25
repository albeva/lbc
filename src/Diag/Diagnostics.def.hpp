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

ERROR(notAllowedTopLevelStatement,
    "statements are not allowed at the top level")
ERROR(unexpectedToken,
    "expected '{0}' got '{1}'")
ERROR(moduleNotFound,
    "no such module '{0}'")
ERROR(failedToLoadModule,
    "failed to load module '{0}'")
ERROR(expectedDeclarationAfterAttribute,
    "expected declaration after attributes, got '{0}'")
ERROR(unexpectedNestedDeclaration,
    "unexpected nested declaration '{0}'")
ERROR(variadicArgumentNotLast,
    "variadic argument must be last")
ERROR(unexpectedReturn,
    "return not allowed outside main module, SUB or FUNCTION")
ERROR(expectedExpression,
    "expected expression, got '{0}'")
ERROR(unsupportedExternLanguage,
    "Unsupported extern langauge '{0}'")
ERROR(onlyDeclarationsInExtern,
    "Only declarations permitted in EXTERN block")

//----------------------------------------
// Semantic errors
//----------------------------------------
ERROR(functionMustReturnAValue,
    "FUNCTION must return a value")
ERROR(subShouldNotReturnAValue,
    "SUB should not return a value")
ERROR(invalidFunctionReturnType,
    "No viable conversion from returned value of type '{0}' to function return type '{1}'")
ERROR(noViableConversionToType,
    "No viable conversion from '{0}' to '{1}'")
ERROR(invalidTypeOfExpression,
    "Invalid TYPEOF expression")
ERROR(unexpectedTokenInTypeOf,
    "Unexpected token in TYPEOF expression")
ERROR(targetNotAssignable,
    "Non-object type '{0}' is not assignable")

#undef DIAG
#undef ERROR
#undef WARNING
