package com.craftinginterpreters.lox.runtime;

import com.craftinginterpreters.lox.Lox;
import com.craftinginterpreters.lox.ast.Expr;
import com.craftinginterpreters.lox.ast.FunctionType;
import com.craftinginterpreters.lox.ast.Stmt;
import com.craftinginterpreters.lox.lexer.Token;
import com.craftinginterpreters.lox.lexer.TokenType;
import com.craftinginterpreters.lox.runtime.constructs.*;
import com.craftinginterpreters.lox.runtime.unwinders.Break;
import com.craftinginterpreters.lox.runtime.unwinders.Return;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import static com.craftinginterpreters.lox.lexer.TokenType.COLON;
import static com.craftinginterpreters.lox.lexer.TokenType.QUESTION_MARK;

public class Interpreter implements Expr.Visitor<Object>, Stmt.Visitor<Object> {

    public final Environment globals = new Environment(null);
    private final Map<Expr, Environment.Location> locals = new HashMap<>();
    private Environment environment = globals;

    public void resolve(Expr expr, int distance, int variableIndex) {
        locals.put(expr, new Environment.Location(distance, variableIndex));
    }

    public void interpret(List<Stmt> statements) {
        try {
            for (Stmt stmt : statements) {
                if (stmt != null) {
                    execute(stmt);
                }
            }
        } catch (RuntimeError error) {
            Lox.runtimeError(error);
        }
    }

    public void interpret(Expr expression) {
        try {
            Object value = evaluate(expression);
            System.out.println(stringify(value));
        } catch (RuntimeError error) {
            Lox.runtimeError(error);
        }
    }

    public void execute(Stmt stmt) {
        stmt.accept(this);
    }

    public Object evaluate(Expr expr) {
        return expr.accept(this);
    }

    private boolean isTruthy(Object object) {
        if (object == null) return false;
        if (object instanceof Boolean) return (boolean) object;

        return true;
    }

    private boolean isEqual(Object a, Object b) {
        if (a == null && b == null) return true;
        if (a == null) return false;

        return a.equals(b);
    }

    private void checkNumberOperand(Token operator, Object operand) {
        if (operand instanceof Double) return;
        throw new RuntimeError(operator, "Operand must be a number.");
    }

    private void checkNumberOperands(Token operator, Object... operands) {
        for (Object operand : operands) {
            checkNumberOperand(operator, operand);
        }
    }

    private String stringify(Object object) {
        if (object == null) return "nil";

        if (object instanceof Double) {
            String text = object.toString();
            if (text.endsWith(".0")) {
                text = text.substring(0, text.length() - 2);
            }
            return text;
        }

        return object.toString();
    }

    private Object lookUpVariable(Expr expr) {
        Environment.Location location = locals.get(expr);
        return environment.getAt(location);
    }

    @Override
    public Object visitExpressionStmt(Stmt.Expression stmt) {
        evaluate(stmt.expression);

        return null;
    }

    @Override
    public Object visitBlockStmt(Stmt.Block stmt) {
        executeBlock(stmt.statements, new Environment(environment));

        return null;
    }

    public void executeBlock(List<Stmt> statements, Environment environment) {
        Environment previous = this.environment;
        try {
            this.environment = new Environment(environment);

            for (Stmt statement : statements) {
                execute(statement);
            }
        } finally {
            this.environment = previous;
        }
    }

    @Override
    public Object visitIfStmt(Stmt.If stmt) {
        if (isTruthy(evaluate(stmt.condition))) {
            execute(stmt.thenBranch);
        } else if (stmt.elseBranch != null) {
            execute(stmt.elseBranch);
        }

        return null;
    }

    @Override
    public Object visitPrintStmt(Stmt.Print stmt) {
        Object value = evaluate(stmt.expression);
        System.out.println(stringify(value));

        return null;
    }

    @Override
    public Object visitVarStmt(Stmt.Var stmt) {
        Object value = null;
        if (stmt.initializer != null) {
            value = evaluate(stmt.initializer);
        }

        environment.define(value);

        return null;
    }

    @Override
    public Object visitWhileStmt(Stmt.While stmt) {
        while (isTruthy(evaluate(stmt.condition))) {
            try {
                execute(stmt.body);
            } catch (Break b) {
                break;
            }
        }

        return null;
    }

    @Override
    public Object visitClassStmt(Stmt.Class stmt) {
        Object superclass = null;
        Environment functionEnvironment = this.environment;
        if (stmt.superclass != null) {
            superclass = evaluate(stmt.superclass);
            if (!(superclass instanceof LoxClass)) {
                throw new RuntimeError(stmt.superclass.name, "Superclass must be a class.");
            }

            functionEnvironment = new Environment(functionEnvironment);
            functionEnvironment.define(superclass);
        }

        Map<String, LoxFunction> methods = new HashMap<>();
        Map<String, LoxFunction> staticMethods = new HashMap<>();
        Map<String, LoxFunction> getterMethods = new HashMap<>();

        for (Stmt.Function method : stmt.methods) {
            boolean isInitializer = method.functionType == FunctionType.METHOD && method.name.lexeme.equals("init");
            boolean isParameterless = method.functionType == FunctionType.GETTER;

            LoxFunction function = new LoxFunction(method, functionEnvironment, isInitializer, isParameterless);

            if (method.functionType == FunctionType.METHOD) {
                methods.put(method.name.lexeme, function);
            } else if (method.functionType == FunctionType.STATIC_METHOD) {
                staticMethods.put(method.name.lexeme, function);
            } else if (method.functionType == FunctionType.GETTER) {
                getterMethods.put(method.name.lexeme, function);
            }
        }

        LoxClass klass = new LoxClass(stmt.name.lexeme, (LoxClass) superclass, methods, staticMethods, getterMethods);
        this.environment.define(klass);

        return null;
    }

    @Override
    public Void visitFunctionStmt(Stmt.Function stmt) {
        LoxFunction function = new LoxFunction(stmt, this.environment, false, false);
        this.environment.define(function);

        return null;
    }

    @Override
    public Object visitBreakStmt(Stmt.Break stmt) {
        throw new Break();
    }

    @Override
    public Void visitReturnStmt(Stmt.Return stmt) {
        Object value = null;
        if (stmt.value != null) value = evaluate(stmt.value);

        throw new Return(value);
    }

    @Override
    public Object visitVariableExpr(Expr.Variable expr) {
        return lookUpVariable(expr);
    }

    @Override
    public Object visitAssignExpr(Expr.Assign expr) {
        Object value = evaluate(expr.value);

        Environment.Location location = locals.get(expr);
        if (location != null) {
            environment.assignAt(location, value);
        }

        return value;
    }

    @Override
    public Object visitBinaryExpr(Expr.Binary expr) {
        Object left = evaluate(expr.left);
        Object right = evaluate(expr.right);

        switch (expr.operator.type) {
            case GREATER:
                checkNumberOperands(expr.operator, left, right);
                return (double) left > (double) right;
            case GREATER_EQUAL:
                checkNumberOperands(expr.operator, left, right);
                return (double) left >= (double) right;
            case LESS:
                checkNumberOperands(expr.operator, left, right);
                return (double) left < (double) right;
            case LESS_EQUAL:
                checkNumberOperands(expr.operator, left, right);
                return (double) left <= (double) right;
            case BANG_EQUAL:
                return !isEqual(left, right);
            case EQUAL_EQUAL:
                return isEqual(left, right);
            case MINUS:
                checkNumberOperands(expr.operator, left, right);
                return (double) left - (double) right;
            case PLUS:
                if (left instanceof Double && right instanceof Double) {
                    return (double) left + (double) right;
                }

                if (left instanceof String && right instanceof String) {
                    return left + (String) right;
                }

                if (left instanceof String || right instanceof String) {
                    return left.toString() + right.toString();
                }

                throw new RuntimeError(expr.operator, "Operand must be a number.");
            case SLASH:
                checkNumberOperands(expr.operator, left, right);
                if ((double) right == 0) {
                    throw new RuntimeError(expr.operator, "Division by zero not allowed.");
                }

                return (double) left / (double) right;
            case STAR:
                checkNumberOperands(expr.operator, left, right);
                return (double) left * (double) right;
        }

        // Unreachable.
        return null;
    }

    @Override
    public Object visitLogicalExpr(Expr.Logical expr) {
        Object left = evaluate(expr.left);

        if (expr.operator.type == TokenType.OR) {
            if (isTruthy(left)) return left;
        } else {
            if (!isTruthy(left)) return left;
        }

        return evaluate(expr.right);
    }

    @Override
    public Object visitSetExpr(Expr.Set expr) {
        Object object = evaluate(expr.object);

        if (!(object instanceof LoxInstance)) {
            throw new RuntimeError(expr.name, "Only instances have fields.");
        }

        Object value = evaluate(expr.value);
        ((LoxInstance) object).set(expr.name, value);

        return value;
    }

    @Override
    public Object visitSuperExpr(Expr.Super expr) {
        Environment.Location location = locals.get(expr);

        LoxClass superclass = (LoxClass) environment.getAt(location);
        LoxInstance object = (LoxInstance) environment.getAt(location.decrementDistance());
        LoxFunction method = superclass.findMethod(expr.method.lexeme);

        if (method == null) {
            throw new RuntimeError(expr.method, "Undefined property '" + expr.method.lexeme + "'.");
        }

        return method.bind(object);
    }

    @Override
    public Object visitThisExpr(Expr.This expr) {
        return lookUpVariable(expr);
    }

    @Override
    public Object visitGroupingExpr(Expr.Grouping expr) {
        return evaluate(expr.expression);
    }

    @Override
    public Object visitLiteralExpr(Expr.Literal expr) {
        return expr.value;
    }

    @Override
    public Object visitUnaryExpr(Expr.Unary expr) {
        Object right = evaluate(expr.right);

        return switch (expr.operator.type) {
            case MINUS -> {
                checkNumberOperand(expr.operator, right);
                yield -(double) right;
            }
            case BANG -> !isTruthy(right);
            default -> null;
        };
    }

    @Override
    public Object visitTernaryExpr(Expr.Ternary expr) {
        Object expr1 = evaluate(expr.expr1);
        Object expr2 = evaluate(expr.expr2);
        Object expr3 = evaluate(expr.expr3);

        // We support for now only the "x ? y : z" operator.
        if (expr.operator1.type == QUESTION_MARK && expr.operator2.type == COLON) {
            return isTruthy(expr1) ? expr2 : expr3;
        }

        return null;
    }

    @Override
    public Object visitCallExpr(Expr.Call expr) {
        Object callee = evaluate(expr.callee);

        List<Object> arguments = new ArrayList<>();
        for (Expr argument : expr.arguments) {
            arguments.add(evaluate(argument));
        }

        if (!(callee instanceof LoxCallable function)) {
            throw new RuntimeError(expr.paren, "Can only call functions and classes.");
        }

        if (arguments.size() != function.arity()) {
            throw new RuntimeError(expr.paren, "Expected " +
                    function.arity() + " arguments but got " +
                    arguments.size() + ".");
        }

        return function.call(this, arguments);
    }

    @Override
    public Object visitGetExpr(Expr.Get expr) {
        Object object = evaluate(expr.object);
        if (object instanceof LoxInstance) {
            Object getResult = ((LoxInstance) object).get(expr.name);
            if (getResult instanceof LoxFunction function && function.isParameterless) {
                return function.call(this, null);
            }

            return getResult;
        }

        throw new RuntimeError(expr.name, "Only instances have properties.");
    }

    @Override
    public Object visitLambdaExpr(Expr.Lambda expr) {
        return new LoxLambda(expr, this.environment);
    }

    public static class RuntimeError extends RuntimeException {
        public final Token token;

        public RuntimeError(Token token, String message) {
            super(message);
            this.token = token;
        }
    }
}
