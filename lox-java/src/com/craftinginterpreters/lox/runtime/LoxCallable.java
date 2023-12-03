package com.craftinginterpreters.lox.runtime;

import java.util.List;

public interface LoxCallable {

    int arity();

    Object call(Interpreter interpreter, List<Object> arguments);
}
