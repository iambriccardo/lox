package com.craftinginterpreters.tool;

public class Benchmark {

    public static int fib(int n) {
        if (n <= 1) {
            return n;
        }
        return fib(n - 2) + fib(n - 1);
    }

    public static void main(String[] args) {
        for (int i = 0; i < 1000; i++) {
            System.out.println(fib(i));
        }
    }
}
