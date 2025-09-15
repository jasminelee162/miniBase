//ast.h
#ifndef MINIBASE_AST_H
#define MINIBASE_AST_H

#include <string>
#include <vector>
#include <memory>

// 前向声明
class ASTVisitor;
struct JoinClause {
    std::string joinType;     // "INNER", "LEFT", "RIGHT", "FULL"
    std::string tableName;    // 要连接的表名
    std::string condition;    // 连接条件，如 "teachers.subject = departments.dept_name"
    
    JoinClause(const std::string& type, const std::string& table, const std::string& cond)
        : joinType(type), tableName(table), condition(cond) {}
};
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

//聚合表达式
class AggregateExpression : public Expression {
    private:
    std::string function;  // SUM, COUNT, AVG, etc.
    std::string column;
    std::string alias;     // AS alias
    
public:
    AggregateExpression(const std::string& func, const std::string& col, const std::string& as = "")
        : function(func), column(col), alias(as) {}
    
    const std::string& getFunction() const { return function; }
    const std::string& getColumn() const { return column; }
    const std::string& getAlias() const { return alias; }
    
    void accept(ASTVisitor& visitor) override;
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
    SelectStatement(std::vector<std::string> cols, 
                   std::vector<std::unique_ptr<AggregateExpression>> aggs,
                   const std::string& mainTable,
                   std::vector<JoinClause> joinList = {},
                   std::unique_ptr<Expression> where = nullptr,
                   std::vector<std::string> groupBy = {},
                   std::unique_ptr<Expression> having = nullptr,
                   std::vector<std::string> orderBy = {},
                   bool orderDesc = false)
        : columns(std::move(cols)), 
          aggregates(std::move(aggs)), 
          mainTableName(mainTable),
          joins(std::move(joinList)),
          whereClause(std::move(where)), 
          groupByColumns(std::move(groupBy)), 
          havingClause(std::move(having)),
          orderByColumns(std::move(orderBy)),
          orderByDesc(orderDesc) 
    {
        // 构建完整的表列表
        fromTables.push_back(mainTableName);
        for (const auto& join : joins) {
            fromTables.push_back(join.tableName);
        }
    }
    
    const std::vector<std::string>& getColumns() const { return columns; }
    const std::string& getMainTableName() const { return mainTableName; }
    const std::string& getTableName() const { return mainTableName; } // 向后兼容
    
    Expression* getWhereClause() const { return whereClause.get(); }
    //Group by
    const std::vector<std::unique_ptr<AggregateExpression>>& getAggregates() const { return aggregates; }
    const std::vector<std::string>& getGroupByColumns() const { return groupByColumns; }
    Expression* getHavingClause() const { return havingClause.get(); }

    // Order by
    const std::vector<std::string>& getOrderByColumns() const { return orderByColumns; }
    bool isOrderByDesc() const { return orderByDesc; }

    //JOIN
    const std::vector<std::string>& getFromTables() const { return fromTables; }
    const std::vector<JoinClause>& getJoins() const { return joins; }
        // 判断是否包含 JOIN
    bool hasJoins() const { return !joins.empty(); }

    void accept(ASTVisitor& visitor) override;

private:
    std::vector<std::string> columns;
    // std::string tableName;
    std::unique_ptr<Expression> whereClause;

    //Group by
    std::vector<std::unique_ptr<AggregateExpression>> aggregates;
    std::vector<std::string> groupByColumns;
    std::unique_ptr<Expression> havingClause;

    // Order by
    std::vector<std::string> orderByColumns;
    bool orderByDesc;

    //join
    // FROM 部分
    std::string mainTableName;                    // 主表名，等同于tableName
    std::vector<std::string> fromTables;          // 所有涉及的表（包括主表和 JOIN 表）
    std::vector<JoinClause> joins;                // JOIN 子句列表
    
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

//JOIN(暂时不使用，后续可删)
class JoinStatement : public Statement {
private:
    std::vector<std::string> columns;
    std::vector<std::string> fromTables;
    std::string joinType;  // "INNER", "LEFT", "RIGHT"
    std::string joinCondition;
    std::unique_ptr<Expression> whereClause;

public:
    JoinStatement(std::vector<std::string> cols,
                  std::vector<std::string> tables,
                  const std::string& joinType,
                  const std::string& condition,
                  std::unique_ptr<Expression> where = nullptr)
        : columns(std::move(cols)), 
          fromTables(std::move(tables)), 
          joinType(joinType),
          joinCondition(condition),
          whereClause(std::move(where)) {}

    const std::vector<std::string>& getColumns() const { return columns; }
    const std::vector<std::string>& getFromTables() const { return fromTables; }
    const std::string& getJoinType() const { return joinType; }
    const std::string& getJoinCondition() const { return joinCondition; }
    const Expression* getWhereClause() const { return whereClause.get(); }

    void accept(ASTVisitor& visitor) override;
};

//SHOW
class ShowTablesStatement : public Statement {


public:
    ShowTablesStatement(){};

    void accept(ASTVisitor& visitor) override;
};

//DORP
class DropStatement : public Statement {
private:
    std::string tableName;
public:
    DropStatement(const std::string& table) : tableName(table) {}
    const std::string& getTableName() const { return tableName; }
    
    void accept(ASTVisitor& visitor) override;
};
// AST访问者接口
class ASTVisitor {
public:
    virtual ~ASTVisitor() = default;

    virtual void visit(LiteralExpression& expr) = 0;
    virtual void visit(IdentifierExpression& expr) = 0;
    virtual void visit(BinaryExpression& expr) = 0;
    virtual void visit(AggregateExpression& expr) = 0;
    virtual void visit(CreateTableStatement& stmt) = 0;
    virtual void visit(InsertStatement& stmt) = 0;
    virtual void visit(SelectStatement& stmt) = 0;
    virtual void visit(DeleteStatement& stmt) = 0;
    virtual void visit(UpdateStatement& stmt) = 0;
    virtual void visit(ShowTablesStatement& stmt) = 0;
    virtual void visit(DropStatement& stmt) = 0;
};

#endif // MINIBASE_AST_H