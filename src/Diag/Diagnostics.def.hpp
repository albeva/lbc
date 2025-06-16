//
// Created by Albert Varaksin on 15/06/2021.
//
// clang-format off
#pragma once

//----------------------------------------
// Parse errors
//----------------------------------------

#define PARSE_ERRORS(_) \
    /* ID                                   Message                                                  */ \
    _( notAllowedTopLevelStatement,         "statements are not allowed at the top level"             ) \
    _( unexpectedToken,                     "expected '{0}' got '{1}'"                                ) \
    _( moduleNotFound,                      "no such module '{0}'"                                    ) \
    _( failedToLoadModule,                  "failed to load module '{0}'"                             ) \
    _( expectedDeclration,                  "expected declaration, got '{0}'"                         ) \
    _( unexpectedNestedDeclaration,         "unexpected nested declaration '{0}'"                     ) \
    _( variadicArgumentNotLast,             "variadic argument must be last"                          ) \
    _( unexpectedReturn,                    "return not allowed outside main module, SUB or FUNCTION" ) \
    _( expectedExpression,                  "expected expression, got '{0}'"                          ) \
    _( unsupportedExternLanguage,           "Unsupported extern langauge '{0}'"                       ) \
    _( onlyDeclarationsInExtern,            "Only declarations permitted in EXTERN block"             ) \
    _( expectedTypeExpression,              "expected type expression, got '{0}'"                     ) \
    _( procTypesMustHaveAPtr,               "{0} type missing a trailing PTR"                         ) \
    _( invalidPointerToReference,           "Cannot have a pointer to a reference"                    ) \
    _( invalidReferenceToReference,         "Cannot have a reference to a reference"                  )

//----------------------------------------
// Semantic errors
//----------------------------------------

#define SEMANTIC_ERRORS(_) \
    /* ID                                   Message                                                      */ \
    _( functionMustReturnAValue,            "FUNCTION must return a value"                                ) \
    _( subShouldNotReturnAValue,            "SUB should not return a value"                               ) \
    _( invalidFunctionReturnType,           "No viable conversion from returned value of type '{0}' to function return type '{1}'") \
    _( noViableConversionToType,            "No viable conversion from '{0}' to '{1}'"                    ) \
    _( invalidTypeOfExpression,             "Invalid TYPEOF expression"                                   ) \
    _( unexpectedTokenInTypeOf,             "Unexpected token in TYPEOF expression"                       ) \
    _( targetNotAssignable,                 "Non-object type '{0}' is not assignable"                     ) \
    _( targetNotCallable,                   "Type '{0}' is not a sub or a function"                       ) \
    _( noMatchingSubOrFunction,             "No matching SUB or FUNCTION to call"                         ) \
    _( unknownIdentifier,                   "Unknown identifier '{0}'"                                    ) \
    _( useBeforeDefinition,                 "Use of variable '{0}' before definition"                     ) \
    _( cannotUseTypeAsBoolean,              "Cannot use '{0}' as boolean"                                 ) \
    _( unaryOperatorAppledToType,           "Unary operator '{0}' cannot be applied to type '{1}'"        ) \
    _( dereferencingNonPointerType,         "Dereferencing a non pointer type '{0}'"                      ) \
    _( unexpectedContinuation,              "{0} not allowed outside FOR or DO loops"                     ) \
    _( unexpectedContinuationTarget,        "Unexpected {0} target '{1}'"                                 ) \
    _( accessingMemberOnNonUDTType,         "Accessing a member on '{0}' which is not a user defined type") \
    _( invalidBinaryExprOperands,           "Binary operator '{0}' cannot be applied to operands of type '{1}' and '{2}'") \
    _( invalidCompareExprOperands,          "Comparison operator '{0}' cannot be applied to operands of type '{1}' and '{2}'") \
    _( cannotConvertOperandToType,          "Cannot convert operand of type '{0}' to {1}"                 ) \
    _( invalidCast,                         "Invalid cast from '{0}' to '{1}'"                            ) \
    _( invalidImplicitConversion,           "Invalid implicit conversion '{0}' to '{1}'"                  ) \
    _( mismatchingIfExprBranchTypes,        "Mismatching types in IF expression branches '{0}' and '{1}'" ) \
    _( circularTypeDependency,              "Circular type dependency detected on '{0}'"                  ) \
    _( undefinedType,                       "Undefined type '{0}'"                                        ) \
    _( notAType,                            "'{0}' is not a type"                                         ) \
    _( symbolAlreadyDefined,                "Symbol '{0}' is already defined"                             ) \
    _( forIteratorMustBeNumeric,            "FOR iterator type must be numeric, got {0}"                  ) \
    _( constantRequiresAConstantExpr,       "Expected a constant expression when initialising CONST variable" ) \
    _( mustBeConstantExpr,                  "Expression must be constant"                                 ) \
    _( cannotTakeAddressOf,                 "Cannot take the address of value of type '{0}'"              ) \
    _( referenceReqioresAnInitializer,      "Declaration of reference variable '{0}' requires an initializer" ) \
    _( assignNonAddresValueToReference,     "Assigning non-addressable expression to reference variable '{0}'")


//----------------------------------------
// All errors
//----------------------------------------

#define ALL_ERRORS(_) \
    PARSE_ERRORS(_)    \
    SEMANTIC_ERRORS(_)
