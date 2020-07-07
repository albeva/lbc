//
// Created by Albert on 07/07/2020.
//
#pragma once
#include "pch.h"

namespace lbc {

enum class TypeKind {
    Void,               // Void, lack of type
    Any,                // any ptr, null
    Pointer,            // Ptr to another type

    Number,             // A number
    Bool,               // true / false
    Integer,            // signed / unsigned integer 8, 16, 32, ... bits
    FloatingPoint,      // single, double
    NumberLast,         // end of numeric declaredTypes

    Function,           // function
    ZString,            // nil terminated string, byte ptr / char*
};

class Type;
class TypePointer;
class TypeNumber;
class TypeBool;
class TypeInteger;
class TypeFloatingPoint;
class TypeFunction;
class TypeZString;

/**
 * Base type is root for all lb types
 *
 * It uses llvm custom rtti system
 */
class TypeRoot: noncopyable {
protected:
    TypeRoot(TypeKind kind, size_t hash)
    : m_kind{kind},
      m_hash{hash} {}

public:
    virtual ~TypeRoot();

    TypeKind kind() const { return m_kind; }
    size_t hash() const { return m_hash; }
    virtual llvm::Type* llvm() = 0;

protected:
    llvm::Type* m_llvm = nullptr;

private:
    const TypeKind m_kind;
    const size_t m_hash;
};

/**
 * Void, lack of type. Cannot be used for C style `void*`
 * Use `Any Ptr` for this
 */
class TypeVoid final: public TypeRoot {
public:
    TypeVoid(size_t hash) : TypeRoot{TypeKind::Void, hash} {}
    static const TypeVoid* get();

    static bool classof(const TypeRoot *type) {
        return type->kind() == TypeKind::Void;
    }

    virtual llvm::Type* llvm();
};

/**
 * Void, lack of type. Cannot be used for C style `void*`
 * Use `Any Ptr` for this
 */
class TypeAny final: public TypeRoot {
public:
    TypeAny(size_t hash) : TypeRoot{TypeKind::Any, hash} {}
    static const TypeAny* get();

    static bool classof(const TypeRoot *type) {
        return type->kind() == TypeKind::Any;
    }

    virtual llvm::Type* llvm();
};

/**
 * Pointer to another type
 */
class TypePointer final: public TypeRoot {
public:
    TypePointer(size_t hash, const TypeRoot* base)
    : TypeRoot{TypeKind::Pointer, hash}, m_base{base} {}

    static const TypePointer* get(const TypeRoot* base);

    static bool classof(const TypeRoot *type) {
        return type->kind() == TypeKind::Pointer;
    }

    virtual llvm::Type* llvm();

    const TypeRoot * base() const { return m_base; }

private:
    const TypeRoot* m_base;
};

/**
 * Base for all numeric declaredTypes
 * Bool while conforming, is special kind
 */
class TypeNumber: public TypeRoot {
protected:
    using TypeRoot::TypeRoot;

public:
    static bool classof(const TypeRoot *type) {
        return type->kind() >= TypeKind::Number &&
               type->kind() < TypeKind::NumberLast;
    }

    virtual int bits() const = 0;
    virtual bool isSigned() const = 0;
};

/**
 * Boolean true / false, result of comparison operators
 */
class TypeBool final: public TypeNumber {
public:
    TypeBool(size_t hash) : TypeNumber{TypeKind::Bool, hash} {}

    static const TypeBool* get();

    static bool classof(const TypeRoot *type) {
        return type->kind() == TypeKind::Bool;
    }

    virtual llvm::Type *llvm();

    virtual int bits() const { return 1; }
    virtual bool isSigned() const { return false; }
};

/**
 * Fixed width integer declaredTypes.
 */
class TypeInteger final: public TypeNumber {
public:
    TypeInteger(size_t hash, int bits, bool isSigned)
    : TypeNumber{TypeKind::Integer, hash},
      m_bits{ bits },
      m_isSigned{ isSigned } {}

    static const TypeInteger* get(int bits, bool isSigned);

    static bool classof(const TypeRoot *type) {
        return type->kind() == TypeKind::Integer;
    }

    virtual llvm::Type *llvm();

    virtual int bits() const { return m_bits; }
    virtual bool isSigned() const { return m_isSigned; }

private:
    const int m_bits;
    const bool m_isSigned;
};

/**
 * Floating point declaredTypes
 */
class TypeFloatingPoint final: public TypeNumber {
public:
    TypeFloatingPoint(size_t hash, int bits)
    : TypeNumber{TypeKind::FloatingPoint, hash},
      m_bits{bits} {}

    static const TypeFloatingPoint* get(int bits);

    static bool classof(const TypeRoot *type) {
        return type->kind() == TypeKind::FloatingPoint;
    }

    virtual llvm::Type *llvm();

    virtual int bits() const { return m_bits; }
    virtual bool isSigned() const { return false; }

private:
    const int m_bits;
};

/**
 * Function type
 */
class TypeFunction final: public TypeRoot {
public:
    TypeFunction(size_t hash, const TypeRoot* retType, vector<const TypeRoot*>&& paramTypes)
    : TypeRoot{TypeKind::Function, hash},
      m_retType{retType},
      m_paramTypes{std::move(paramTypes)}
    {}

    static const TypeFunction* get(const TypeRoot* retType, vector<const TypeRoot*>&& paramTypes);

    static bool classof(const TypeRoot *type) {
        return type->kind() == TypeKind::Function;
    }

    virtual llvm::Type *llvm();

    const TypeRoot* retType() const { return m_retType; }
    const vector<const TypeRoot*>& paramTypes() const { return m_paramTypes; }

private:
    const TypeRoot* m_retType;
    const vector<const TypeRoot*> m_paramTypes;
};

/**
 * ZString zero terminated string literal.
 * Equivelant to C `char*`
 */
class TypeZString final: public TypeRoot {
public:
    TypeZString(size_t hash): TypeRoot{TypeKind::ZString, hash} {}

    static const TypeZString* get();

    static bool classof(const TypeRoot *type) {
        return type->kind() == TypeKind::ZString;
    }

    virtual llvm::Type *llvm();
};


} // namespace lbc
