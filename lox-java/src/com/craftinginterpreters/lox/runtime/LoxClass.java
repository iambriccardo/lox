package com.craftinginterpreters.lox.runtime;

import java.util.List;

class LoxClass implements LoxCallable {
    private final String name;

    public LoxClass(String name) {
        this.name = name;
    }

    public String getName() {
        return name;
    }

    @Override
    public String toString() {
        return name;
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
