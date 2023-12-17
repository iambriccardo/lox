package com.craftinginterpreters.lox.runtime;

import com.craftinginterpreters.lox.ast.Expr;

import java.util.List;

public class LoxLambda implements LoxCallable {
    private final Expr.Lambda declaration;
    private final Environment closure;

    public LoxLambda(Expr.Lambda declaration, Environment closure) {
        this.declaration = declaration;
        this.closure = closure;
    }

    @Override
    public int arity() {
        return this.declaration.params.size();
    }

    @Override
    public Object call(Interpreter interpreter, List<Object> arguments) {
        Environment environment = new Environment(this.closure);
        for (int i = 0; i < declaration.params.size(); i++) {
            environment.define(arguments.get(i));
        }

        try {
            interpreter.executeBlock(declaration.body, environment);
        } catch (Return returnValue) {
            return returnValue.value;
        }
        return null;
    }

    @Override
    public String toString() {
        return "<fn lambda>";
    }
}

