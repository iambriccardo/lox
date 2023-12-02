package com.craftinginterpreters.lox;

import com.craftinginterpreters.lox.ast.Expr;
import com.craftinginterpreters.lox.ast.Stmt;
import com.craftinginterpreters.lox.lexer.Scanner;
import com.craftinginterpreters.lox.lexer.Token;
import com.craftinginterpreters.lox.lexer.TokenType;
import com.craftinginterpreters.lox.parser.Parser;
import com.craftinginterpreters.lox.runtime.Interpreter;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.nio.charset.Charset;
import java.nio.file.Files;
import java.nio.file.Paths;
import java.util.List;

public class Lox {
    private static final Interpreter interpreter = new Interpreter();
    private static boolean hadCriticalError = false;
    private static boolean hadRuntimeError = false;

    public static void main(String[] args) throws IOException {
        if (args.length > 1) {
            System.out.println("Usage: jlox [script]");
            System.exit(64);
        } else if (args.length == 1) {
            runFile(args[0]);
        } else {
            runPrompt();
        }
    }

    private static void runFile(String path) throws IOException {
        byte[] bytes = Files.readAllBytes(Paths.get(path));
        run(new String(bytes, Charset.defaultCharset()));

        if (hadCriticalError) System.exit(65);
        if (hadRuntimeError) System.exit(70);
    }

    private static void runPrompt() throws IOException {
        InputStreamReader input = new InputStreamReader(System.in);
        BufferedReader reader = new BufferedReader(input);

        for (; ; ) {
            System.out.print("> ");
            String line = reader.readLine();
            if (line == null) break;
            run(line);
            hadCriticalError = false;
        }
    }

    private static void run(String source) {
        Scanner scanner = new Scanner(source);
        List<Token> tokens = scanner.scanTokens();

        Parser parser = new Parser(tokens);
        List<Stmt> statements = parser.parse();

        // Stop if there was a critical error in the parsing process. Non-critical errors can move onto execution
        // under the assumption that the synchronization step successfully found the next synchronization point.
        if (hadCriticalError) return;

        Expr singleExpression = isSingleExpression(statements);
        if (singleExpression != null) {
            interpreter.interpret(singleExpression);
        } else {
            interpreter.interpret(statements);
        }
    }

    private static Expr isSingleExpression(List<Stmt> statements) {
        if (!statements.isEmpty()) {
            Stmt stmt = statements.getFirst();
            if (stmt instanceof Stmt.Expression) {
                return ((Stmt.Expression) stmt).expression;
            }
        }

        return null;
    }

    public static void error(int line, String message, boolean critical) {
        report(line, "", message, critical);
    }

    public static void error(Token token, String message, boolean critical) {
        if (token.type == TokenType.EOF) {
            report(token.line, " at end", message, critical);
        } else {
            report(token.line, " at '" + token.lexeme + "'", message, critical);
        }
    }

    public static void runtimeError(Interpreter.RuntimeError error) {
        System.err.println("[line " + error.token.line + "] RuntimeError: " + error.getMessage());
        hadRuntimeError = true;
    }

    private static void report(int line, String where, String message, boolean critical) {
        System.err.println("[line " + line + "] " + (critical ? "Error" : "Warning") + where + ": " + message);
        if (critical) {
            hadCriticalError = true;
        }
    }
}
