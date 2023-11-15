package com.craftinginterpreters.lox.runtime;

import com.craftinginterpreters.lox.lexer.Token;

import java.util.HashMap;
import java.util.Map;

class Environment {
    private final Map<String, Object> values = new HashMap<>();

    public void define(String name, Object value) {
        values.put(name, value);
    }

    public Object get(Token name) {
        if (values.containsKey(name.lexeme)) {
            return values.get(name.lexeme);
        }

        throw new Interpreter.RuntimeError(name, "Undefined variable '" + name.lexeme + "'.");
    }
}