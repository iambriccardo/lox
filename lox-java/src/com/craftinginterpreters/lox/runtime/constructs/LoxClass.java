package com.craftinginterpreters.lox.runtime.constructs;

import com.craftinginterpreters.lox.lexer.Token;
import com.craftinginterpreters.lox.runtime.Interpreter;

import java.util.List;
import java.util.Map;

public class LoxClass extends LoxInstance implements LoxCallable {

    private final String name;
    private final Map<String, LoxFunction> methods;
    private final Map<String, LoxFunction> staticMethods;
    private final Map<String, LoxFunction> getterMethods;

    public LoxClass(String name, Map<String, LoxFunction> methods, Map<String, LoxFunction> staticMethods, Map<String, LoxFunction> getterMethods) {
        this.name = name;
        this.methods = methods;
        this.staticMethods = staticMethods;
        this.getterMethods = getterMethods;
    }

    @Override
    public void set(Token name, Object value) {
        throw new Interpreter.RuntimeError(name, "Can't set static property '" + name.lexeme + "' on " + this + ".");
    }

    @Override
    public Object get(Token name) {
        // For now, we support only static methods and not static fields.
        LoxFunction staticMethod = this.findStaticMethod(name.lexeme);
        if (staticMethod != null) return staticMethod;

        throw new Interpreter.RuntimeError(name, "Undefined static method '" + name.lexeme + "'.");
    }

    @Override
    public String toString() {
        return this.getName() + " class";
    }

    public String getName() {
        return name;
    }

    public LoxFunction findMethod(String name) {
        return find(this.methods, name);
    }

    public LoxFunction findStaticMethod(String name) {
        return find(this.staticMethods, name);
    }

    public LoxFunction findGetterMethod(String name) {
        return find(this.getterMethods, name);
    }

    private LoxFunction find(Map<String, LoxFunction> functions, String name) {
        if (functions.containsKey(name)) {
            return functions.get(name);
        }

        return null;
    }

    @Override
    public int arity() {
        LoxFunction initializer = findMethod("init");
        if (initializer == null) return 0;
        return initializer.arity();
    }

    @Override
    public Object call(Interpreter interpreter, List<Object> arguments) {
        LoxInstance instance = new LoxInstance(this);
        LoxFunction initializer = findMethod("init");
        if (initializer != null) {
            initializer.bind(instance).call(interpreter, arguments);
        }

        return instance;
    }
}
