package com.craftinginterpreters.lox.runtime;

import com.craftinginterpreters.lox.Lox;
import com.craftinginterpreters.lox.ast.Expr;
import com.craftinginterpreters.lox.ast.FunctionType;
import com.craftinginterpreters.lox.ast.Stmt;
import com.craftinginterpreters.lox.lexer.Token;

import java.util.*;

public class Resolver implements Expr.Visitor<Void>, Stmt.Visitor<Void> {

    private final Stack<List<Boolean>> scopes = new Stack<>();
    private final Stack<Map<String, Integer>> indexes = new Stack<>();
    private final Stack<Set<String>> usages = new Stack<>();
    private final Interpreter interpreter;
    private EnclosingContext currentFunction = EnclosingContext.NONE;
    private EnclosingContext currentWhile = EnclosingContext.NONE;
    private EnclosingContext currentClass = EnclosingContext.NONE;

    public Resolver(Interpreter interpreter) {
        this.interpreter = interpreter;
    }

    public void resolve(List<Stmt> statements) {
        beginScope();
        for (Stmt statement : statements) {
            resolve(statement);
        }
        endScope();
    }

    private void resolve(Stmt stmt) {
        stmt.accept(this);
    }

    private void resolve(Expr expr) {
        expr.accept(this);
    }

    private void beginScope() {
        scopes.push(new ArrayList<>());
        indexes.push(new HashMap<>());
        usages.push(new HashSet<>());
    }

    private void endScope() {
        Map<String, Integer> index = indexes.peek();
        Set<String> usage = usages.peek();

        for (Map.Entry<String, Integer> entry : index.entrySet()) {
            // If the variable is "this", we don't want to raise a warning in case it's unused.
            if (entry.getKey().equals("this")) continue;

            if (!usage.contains(entry.getKey())) {
                Lox.warning("Variable " + entry.getKey() + " is never used.");
            } else {
                // If the variable was used we remove it from the set, in order to compute the set of variables that
                // were used but do not belong to this scope.
                usage.remove(entry.getKey());
            }
        }

        scopes.pop();
        indexes.pop();
        usages.pop();

        if (!usages.isEmpty()) {
            // In case we have an outer usage, we will merge the current usages to the upstream ones.
            Set<String> outerUsage = usages.peek();
            outerUsage.addAll(usage);
        }
    }

    private void declare(Token name) {
        if (scopes.isEmpty()) return;

        Map<String, Integer> index = indexes.peek();
        if (index.containsKey(name.lexeme)) {
            Lox.error(name, "Already a variable with this name in this scope.");
        }

        List<Boolean> scope = scopes.peek();
        // We insert the variable, and we assume it's always appended at the end.
        int insertionIndex = scope.size();
        scope.add(false);
        index.put(name.lexeme, insertionIndex);
    }

    private void defineThis() {
        List<Boolean> scope = scopes.peek();
        int insertionIndex = scope.size();
        scope.add(true);
        indexes.peek().put("this", insertionIndex);
    }

    private void define(Token name) {
        if (scopes.isEmpty()) return;
        Map<String, Integer> index = indexes.peek();
        scopes.peek().set(index.get(name.lexeme), true);
    }

    private void resolveLocal(Expr expr, Token name) {
        for (int i = scopes.size() - 1; i >= 0; i--) {
            Integer variableIndex = indexes.get(i).get(name.lexeme);
            if (variableIndex != null) {
                interpreter.resolve(expr, scopes.size() - 1 - i, variableIndex);
                return;
            }
        }

        if (expr instanceof Expr.Assign) {
            Lox.error(((Expr.Assign) expr).name, "Can't assign an undefined variable.");
        } else if (expr instanceof Expr.Variable) {
            Lox.error(((Expr.Variable) expr).name, "Variable undefined.");
        }
    }

    private void resolveFunction(Stmt.Function function, EnclosingContext context) {
        EnclosingContext enclosingFunction = currentFunction;
        currentFunction = context;
        beginScope();
        for (Token param : function.params) {
            declare(param);
            define(param);
        }
        resolve(function.body);
        endScope();
        currentFunction = enclosingFunction;
    }

    private void resolveLambda(Expr.Lambda lambda) {
        EnclosingContext enclosingFunction = currentFunction;
        currentFunction = EnclosingContext.FUNCTION;
        beginScope();
        for (Token param : lambda.params) {
            declare(param);
            define(param);
        }
        resolve(lambda.body);
        endScope();
        currentFunction = enclosingFunction;
    }

    @Override
    public Void visitAssignExpr(Expr.Assign expr) {
        resolve(expr.value);
        resolveLocal(expr, expr.name);

        return null;
    }

    @Override
    public Void visitBinaryExpr(Expr.Binary expr) {
        resolve(expr.left);
        resolve(expr.right);

        return null;
    }

    @Override
    public Void visitTernaryExpr(Expr.Ternary expr) {
        resolve(expr.expr1);
        resolve(expr.expr2);
        resolve(expr.expr3);

        return null;
    }

    @Override
    public Void visitCallExpr(Expr.Call expr) {
        resolve(expr.callee);

        for (Expr argument : expr.arguments) {
            resolve(argument);
        }

        return null;
    }

    @Override
    public Void visitGetExpr(Expr.Get expr) {
        resolve(expr.object);

        return null;
    }

    @Override
    public Void visitGroupingExpr(Expr.Grouping expr) {
        resolve(expr.expression);

        return null;
    }

    @Override
    public Void visitLiteralExpr(Expr.Literal expr) {
        return null;
    }

    @Override
    public Void visitLogicalExpr(Expr.Logical expr) {
        resolve(expr.left);
        resolve(expr.right);

        return null;
    }

    @Override
    public Void visitSetExpr(Expr.Set expr) {
        resolve(expr.value);
        resolve(expr.object);

        return null;
    }

    @Override
    public Void visitThisExpr(Expr.This expr) {
        if (currentClass != EnclosingContext.CLASS) {
            Lox.error(expr.keyword, "Can't use 'this' outside of a class.");
            return null;
        }

        if (currentFunction == EnclosingContext.STATIC_METHOD) {
            Lox.error(expr.keyword, "Can't use 'this' inside a static method.");
            return null;
        }

        resolveLocal(expr, expr.keyword);

        return null;
    }

    @Override
    public Void visitUnaryExpr(Expr.Unary expr) {
        resolve(expr.right);

        return null;
    }

    @Override
    public Void visitLambdaExpr(Expr.Lambda expr) {
        resolveLambda(expr);

        return null;
    }

    @Override
    public Void visitVariableExpr(Expr.Variable expr) {
        if (!indexes.isEmpty()) {
            Integer variableIndex = indexes.peek().get(expr.name.lexeme);
            if (variableIndex != null && variableIndex < scopes.peek().size() && scopes.peek().get(variableIndex) == Boolean.FALSE) {
                Lox.error(expr.name, "Can't read local variable in its own initializer.");
            }
        }

        if (!usages.isEmpty()) {
            // We mark that the variable has been used.
            usages.peek().add(expr.name.lexeme);
        }

        resolveLocal(expr, expr.name);

        return null;
    }

    @Override
    public Void visitBlockStmt(Stmt.Block stmt) {
        beginScope();
        resolve(stmt.statements);
        endScope();

        return null;
    }

    @Override
    public Void visitExpressionStmt(Stmt.Expression stmt) {
        resolve(stmt.expression);

        return null;
    }

    @Override
    public Void visitClassStmt(Stmt.Class stmt) {
        EnclosingContext enclosingClass = currentClass;
        currentClass = EnclosingContext.CLASS;

        declare(stmt.name);
        define(stmt.name);

        for (Stmt.Function method : stmt.methods) {
            // Only for non-static methods, we have to define an extra scope which contains the `this`.
            if (method.functionType != FunctionType.STATIC_METHOD) {
                beginScope();
                defineThis();
            }

            EnclosingContext enclosingFunction = EnclosingContext.METHOD;
            if (method.name.lexeme.equals("init")) {
                enclosingFunction = EnclosingContext.INITIALIZER;
            } else if (method.functionType == FunctionType.STATIC_METHOD) {
                enclosingFunction = EnclosingContext.STATIC_METHOD;
            }

            resolveFunction(method, enclosingFunction);

            if (method.functionType != FunctionType.STATIC_METHOD) {
                endScope();
            }
        }

        currentClass = enclosingClass;

        return null;
    }

    @Override
    public Void visitFunctionStmt(Stmt.Function stmt) {
        declare(stmt.name);
        define(stmt.name);

        resolveFunction(stmt, EnclosingContext.FUNCTION);

        return null;
    }

    @Override
    public Void visitIfStmt(Stmt.If stmt) {
        resolve(stmt.condition);
        resolve(stmt.thenBranch);
        if (stmt.elseBranch != null) {
            resolve(stmt.elseBranch);
        }

        return null;
    }

    @Override
    public Void visitPrintStmt(Stmt.Print stmt) {
        resolve(stmt.expression);
        return null;
    }

    @Override
    public Void visitBreakStmt(Stmt.Break stmt) {
        if (currentWhile != EnclosingContext.WHILE) {
            Lox.error(stmt.keyword, "Can't break outside of a while loop.");
        }

        return null;
    }

    @Override
    public Void visitReturnStmt(Stmt.Return stmt) {
        if (currentFunction != EnclosingContext.FUNCTION
                && currentFunction != EnclosingContext.METHOD
                && currentFunction != EnclosingContext.STATIC_METHOD
                && currentFunction != EnclosingContext.INITIALIZER) {
            Lox.error(stmt.keyword, "Can't return from top-level code.");
        }

        if (stmt.value != null) {
            if (currentFunction == EnclosingContext.INITIALIZER) {
                Lox.error(stmt.keyword, "Can't return a value from an initializer.");
            }

            resolve(stmt.value);
        }

        return null;
    }

    @Override
    public Void visitVarStmt(Stmt.Var stmt) {
        declare(stmt.name);
        if (stmt.initializer != null) {
            resolve(stmt.initializer);
        }
        define(stmt.name);

        return null;
    }

    @Override
    public Void visitWhileStmt(Stmt.While stmt) {
        resolve(stmt.condition);

        EnclosingContext enclosingWhile = currentWhile;
        currentWhile = EnclosingContext.WHILE;
        resolve(stmt.body);
        currentWhile = enclosingWhile;

        return null;
    }

    private enum EnclosingContext {
        NONE,
        INITIALIZER,
        FUNCTION,
        METHOD,
        STATIC_METHOD,
        CLASS,
        WHILE,
    }
}
