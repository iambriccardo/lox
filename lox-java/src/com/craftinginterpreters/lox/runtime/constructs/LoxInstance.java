package com.craftinginterpreters.lox.runtime.constructs;

import com.craftinginterpreters.lox.lexer.Token;
import com.craftinginterpreters.lox.runtime.Interpreter;

import java.util.HashMap;
import java.util.Map;

public class LoxInstance {
    private final Map<String, Object> fields = new HashMap<>();
    private LoxClass klass;

    public LoxInstance() {
    }

    public LoxInstance(LoxClass klass) {
        this.klass = klass;
    }

    public void set(Token name, Object value) {
        this.fields.put(name.lexeme, value);
    }

    public Object get(Token name) {
        if (this.fields.containsKey(name.lexeme)) {
            return this.fields.get(name.lexeme);
        }

        LoxFunction method = klass.findMethod(name.lexeme);
        if (method != null) return method.bind(this);

        LoxFunction getterMethod = klass.findGetterMethod(name.lexeme);
        if (getterMethod != null) return getterMethod.bind(this);

        throw new Interpreter.RuntimeError(name, "Undefined property '" + name.lexeme + "'.");
    }

    @Override
    public String toString() {
        return klass.getName() + " instance";
    }
}