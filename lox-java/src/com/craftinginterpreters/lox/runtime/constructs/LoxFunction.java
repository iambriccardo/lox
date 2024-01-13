package com.craftinginterpreters.lox.runtime.constructs;

import com.craftinginterpreters.lox.ast.Stmt;
import com.craftinginterpreters.lox.runtime.Environment;
import com.craftinginterpreters.lox.runtime.Interpreter;
import com.craftinginterpreters.lox.runtime.unwinders.Return;

import java.util.List;

public class LoxFunction implements LoxCallable {
    public final boolean isInitializer;
    public final boolean isParameterless;
    private final Stmt.Function declaration;
    private final Environment closure;

    public LoxFunction(Stmt.Function declaration, Environment closure, boolean isInitializer, boolean isParameterless) {
        this.declaration = declaration;
        this.closure = closure;
        this.isInitializer = isInitializer;
        this.isParameterless = isParameterless;
    }

    public LoxFunction bind(LoxInstance instance) {
        Environment environment = new Environment(closure);
        environment.define(instance);

        return new LoxFunction(declaration, environment, isInitializer, isParameterless);
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
            if (isInitializer) return closure.getAt(new Environment.Location(0, 0));
            return returnValue.value;
        }

        // TODO: check if the positioning of the variable is correct.
        if (isInitializer) return closure.getAt(new Environment.Location(0, 0));
        return null;
    }

    @Override
    public String toString() {
        return "<fn " + declaration.name.lexeme + ">";
    }
}
