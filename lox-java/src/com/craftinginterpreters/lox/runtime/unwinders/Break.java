package com.craftinginterpreters.lox.runtime.unwinders;

public class Break extends RuntimeException {

    public Break() {
        super(null, null, false, false);
    }
}