package com.craftinginterpreters.lox.runtime;

import com.craftinginterpreters.lox.lexer.Token;

import java.util.HashMap;
import java.util.Map;

class Environment {
    private final Environment enclosing;
    private final Map<String, Object> values = new HashMap<>();

    public Environment(Environment enclosing) {
        this.enclosing = enclosing;
    }

    public void define(String name, Object value) {
        values.put(name, value);
    }

    public void assign(Token name, Object value) {
        if (values.containsKey(name.lexeme)) {
            values.put(name.lexeme, value);
            return;
        }

        if (enclosing != null) {
            enclosing.assign(name, value);
            return;
        }

        throw new Interpreter.RuntimeError(name, "Undefined variable '" + name.lexeme + "'.");
    }

    public Object get(Token name) {
        if (values.containsKey(name.lexeme)) {
            Object value = values.get(name.lexeme);
            if (value == null) {
                throw new Interpreter.RuntimeError(name, "Uninitialized variable '" + name.lexeme + "'.");
            }

            return value;
        }

        if (enclosing != null) {
            return enclosing.get(name);
        }

        throw new Interpreter.RuntimeError(name, "Undefined variable '" + name.lexeme + "'.");
    }
}