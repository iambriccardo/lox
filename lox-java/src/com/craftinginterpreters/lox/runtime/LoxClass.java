package com.craftinginterpreters.lox.runtime;

import java.util.List;
import java.util.Map;

public class LoxClass implements LoxCallable {

    private final String name;
    private final Map<String, LoxFunction> methods;

    public LoxClass(String name, Map<String, LoxFunction> methods) {
        this.name = name;
        this.methods = methods;
    }

    public String getName() {
        return name;
    }

    @Override
    public String toString() {
        return name;
    }

    public LoxFunction findMethod(String name) {
        if (methods.containsKey(name)) {
            return methods.get(name);
        }

        return null;
    }

    @Override
    public int arity() {
        return 0;
    }

    @Override
    public Object call(Interpreter interpreter, List<Object> arguments) {
        return new LoxInstance(this);
    }
}
