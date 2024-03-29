package com.craftinginterpreters.lox.runtime.constructs;

import com.craftinginterpreters.lox.runtime.Interpreter;

import java.util.List;

public interface LoxCallable {

    int arity();

    Object call(Interpreter interpreter, List<Object> arguments);
}
