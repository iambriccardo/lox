package com.craftinginterpreters.lox.runtime;

public class Break extends RuntimeException {

    public Break() {
        super(null, null, false, false);
    }
}