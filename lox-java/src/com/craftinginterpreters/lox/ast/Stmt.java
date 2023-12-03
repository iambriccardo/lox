package com.craftinginterpreters.lox.ast;

import java.util.List;
import com.craftinginterpreters.lox.lexer.Token;

public abstract class Stmt {
public interface Visitor<R> {
    public R visitBlockStmt(Block stmt);
    public R visitExpressionStmt(Expression stmt);
    public R visitFunctionStmt(Function stmt);
    public R visitIfStmt(If stmt);
    public R visitPrintStmt(Print stmt);
    public R visitReturnStmt(Return stmt);
    public R visitVarStmt(Var stmt);
    public R visitWhileStmt(While stmt);
  }
  public static class Block extends Stmt {
    public Block(List<Stmt> statements) {
      this.statements = statements;
    }

    public final List<Stmt> statements;

    @Override
    public <R> R accept(Visitor<R> visitor) {
      return visitor.visitBlockStmt(this);
    }
  }
  public static class Expression extends Stmt {
    public Expression(Expr expression) {
      this.expression = expression;
    }

    public final Expr expression;

    @Override
    public <R> R accept(Visitor<R> visitor) {
      return visitor.visitExpressionStmt(this);
    }
  }
  public static class Function extends Stmt {
    public Function(Token name, List<Token> params, List<Stmt> body) {
      this.name = name;
      this.params = params;
      this.body = body;
    }

    public final Token name;
    public final List<Token> params;
    public final List<Stmt> body;

    @Override
    public <R> R accept(Visitor<R> visitor) {
      return visitor.visitFunctionStmt(this);
    }
  }
  public static class If extends Stmt {
    public If(Expr condition, Stmt thenBranch, Stmt elseBranch) {
      this.condition = condition;
      this.thenBranch = thenBranch;
      this.elseBranch = elseBranch;
    }

    public final Expr condition;
    public final Stmt thenBranch;
    public final Stmt elseBranch;

    @Override
    public <R> R accept(Visitor<R> visitor) {
      return visitor.visitIfStmt(this);
    }
  }
  public static class Print extends Stmt {
    public Print(Expr expression) {
      this.expression = expression;
    }

    public final Expr expression;

    @Override
    public <R> R accept(Visitor<R> visitor) {
      return visitor.visitPrintStmt(this);
    }
  }
  public static class Return extends Stmt {
    public Return(Token keyword, Expr value) {
      this.keyword = keyword;
      this.value = value;
    }

    public final Token keyword;
    public final Expr value;

    @Override
    public <R> R accept(Visitor<R> visitor) {
      return visitor.visitReturnStmt(this);
    }
  }
  public static class Var extends Stmt {
    public Var(Token name, Expr initializer) {
      this.name = name;
      this.initializer = initializer;
    }

    public final Token name;
    public final Expr initializer;

    @Override
    public <R> R accept(Visitor<R> visitor) {
      return visitor.visitVarStmt(this);
    }
  }
  public static class While extends Stmt {
    public While(Expr condition, Stmt body) {
      this.condition = condition;
      this.body = body;
    }

    public final Expr condition;
    public final Stmt body;

    @Override
    public <R> R accept(Visitor<R> visitor) {
      return visitor.visitWhileStmt(this);
    }
  }

  public abstract <R> R accept(Visitor<R> visitor);
}
