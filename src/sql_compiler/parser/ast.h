//ast.h
#ifndef MINIBASE_AST_H
#define MINIBASE_AST_H

#include <string>
#include <vector>
#include <memory>

// 前向声明
class ASTVisitor;

// AST节点基类
class ASTNode {
public:
    virtual ~ASTNode() = default;
    virtual void accept(ASTVisitor& visitor) = 0;
};

// 表达式节点基类
class Expression : public ASTNode {
public:
    virtual ~Expression() = default;
};

// 语句节点基类
class Statement : public ASTNode {
public:
    virtual ~Statement() = default;
};

// 字面量表达式
class LiteralExpression : public Expression {
public:
    enum class LiteralType {
        INTEGER,
        STRING
    };

    LiteralExpression(LiteralType type, const std::string& value)
        : type(type), value(value) {}

    LiteralType getType() const { return type; }
    const std::string& getValue() const { return value; }

    void accept(ASTVisitor& visitor) override;

private:
    LiteralType type;
    std::string value;
};

// 标识符表达式
class IdentifierExpression : public Expression {
public:
    IdentifierExpression(const std::string& name) : name(name) {}

    const std::string& getName() const { return name; }

    void accept(ASTVisitor& visitor) override;

private:
    std::string name;
};

// 二元操作表达式
class BinaryExpression : public Expression {
public:
    enum class Operator {
        EQUALS,      // =
        LESS_THAN,   // <
        GREATER_THAN,// >
        LESS_EQUAL,  // <=
        GREATER_EQUAL,// >=
        NOT_EQUAL,   // !=
        PLUS,        // +
        MINUS,       // -
        MULTIPLY,    // *
        DIVIDE       // /
    };

    BinaryExpression(std::unique_ptr<Expression> left, Operator op, std::unique_ptr<Expression> right)
        : left(std::move(left)), op(op), right(std::move(right)) {}

    Expression* getLeft() const { return left.get(); }
    Operator getOperator() const { return op; }
    Expression* getRight() const { return right.get(); }

    void accept(ASTVisitor& visitor) override;

private:
    std::unique_ptr<Expression> left;
    Operator op;
    std::unique_ptr<Expression> right;
};

// 列定义
class ColumnDefinition {
public:
    enum class DataType {
        INT,
        VARCHAR
    };

    ColumnDefinition(const std::string& name, DataType type)
        : name(name), type(type) {}

    const std::string& getName() const { return name; }
    DataType getType() const { return type; }

private:
    std::string name;
    DataType type;
};

// CREATE TABLE语句
class CreateTableStatement : public Statement {
public:
    CreateTableStatement(const std::string& tableName, std::vector<ColumnDefinition> columns)
        : tableName(tableName), columns(std::move(columns)) {}

    const std::string& getTableName() const { return tableName; }
    const std::vector<ColumnDefinition>& getColumns() const { return columns; }

    void accept(ASTVisitor& visitor) override;

private:
    std::string tableName;
    std::vector<ColumnDefinition> columns;
};

// INSERT语句的值列表
class ValueList {
public:
    ValueList(std::vector<std::unique_ptr<Expression>> values)
        : values(std::move(values)) {}

    const std::vector<std::unique_ptr<Expression>>& getValues() const { return values; }

private:
    std::vector<std::unique_ptr<Expression>> values;
};

// INSERT语句
class InsertStatement : public Statement {
public:
    InsertStatement(const std::string& tableName, 
                   std::vector<std::string> columnNames,
                   std::vector<ValueList> valueLists)
        : tableName(tableName), columnNames(std::move(columnNames)), valueLists(std::move(valueLists)) {}

    const std::string& getTableName() const { return tableName; }
    const std::vector<std::string>& getColumnNames() const { return columnNames; }
    const std::vector<ValueList>& getValueLists() const { return valueLists; }

    void accept(ASTVisitor& visitor) override;

private:
    std::string tableName;
    std::vector<std::string> columnNames;
    std::vector<ValueList> valueLists;
};

// SELECT语句
class SelectStatement : public Statement {
public:
    SelectStatement(std::vector<std::string> columns, 
                   const std::string& tableName,
                   std::unique_ptr<Expression> whereClause = nullptr)
        : columns(std::move(columns)), tableName(tableName), whereClause(std::move(whereClause)) {}

    const std::vector<std::string>& getColumns() const { return columns; }
    const std::string& getTableName() const { return tableName; }
    Expression* getWhereClause() const { return whereClause.get(); }

    void accept(ASTVisitor& visitor) override;

private:
    std::vector<std::string> columns;
    std::string tableName;
    std::unique_ptr<Expression> whereClause;
};

// DELETE语句
class DeleteStatement : public Statement {
public:
    DeleteStatement(const std::string& tableName, std::unique_ptr<Expression> whereClause = nullptr)
        : tableName(tableName), whereClause(std::move(whereClause)) {}

    const std::string& getTableName() const { return tableName; }
    Expression* getWhereClause() const { return whereClause.get(); }

    void accept(ASTVisitor& visitor) override;

private:
    std::string tableName;
    std::unique_ptr<Expression> whereClause;
};

//UPDATE语句
class UpdateStatement : public Statement {
public:
    UpdateStatement(const std::string& tableName,
                    std::vector<std::pair<std::string, std::unique_ptr<Expression>>> assignments,
                    std::unique_ptr<Expression> whereClause = nullptr)
            : tableName_(std::move(tableName)),
                assignments_(std::move(assignments)), // ✅ move
              whereClause_(std::move(whereClause)) {}
    const std::string& getTableName() const { return tableName_; }
    const std::vector<std::pair<std::string, std::unique_ptr<Expression>>>& getAssignments() const { return assignments_; }
    Expression* getWhereClause() const { return whereClause_.get(); }

    void accept(ASTVisitor& visitor) override;
private:
    std::string tableName_;
    std::vector<std::pair<std::string, std::unique_ptr<Expression>>> assignments_;
    std::unique_ptr<Expression> whereClause_;
};

// AST访问者接口
class ASTVisitor {
public:
    virtual ~ASTVisitor() = default;

    virtual void visit(LiteralExpression& expr) = 0;
    virtual void visit(IdentifierExpression& expr) = 0;
    virtual void visit(BinaryExpression& expr) = 0;
    virtual void visit(CreateTableStatement& stmt) = 0;
    virtual void visit(InsertStatement& stmt) = 0;
    virtual void visit(SelectStatement& stmt) = 0;
    virtual void visit(DeleteStatement& stmt) = 0;
    virtual void visit(UpdateStatement& stmt) = 0;
};

#endif // MINIBASE_AST_H