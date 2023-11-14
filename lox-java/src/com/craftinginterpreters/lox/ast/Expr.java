package com.craftinginterpreters.lox.ast;

import java.util.List;
import com.craftinginterpreters.lox.lexer.Token;

public abstract class Expr {
public interface Visitor<R> {
    public R visitBinaryExpr(Binary expr);
    public R visitTernaryExpr(Ternary expr);
    public R visitGroupingExpr(Grouping expr);
    public R visitLiteralExpr(Literal expr);
    public R visitUnaryExpr(Unary expr);
  }
  public static class Binary extends Expr {
    public Binary(Expr left, Token operator, Expr right) {
      this.left = left;
      this.operator = operator;
      this.right = right;
    }

    public final Expr left;
    public final Token operator;
    public final Expr right;

    @Override
    public <R> R accept(Visitor<R> visitor) {
      return visitor.visitBinaryExpr(this);
    }
  }
  public static class Ternary extends Expr {
    public Ternary(Expr expr1, Token operator1, Expr expr2, Token operator2, Expr expr3) {
      this.expr1 = expr1;
      this.operator1 = operator1;
      this.expr2 = expr2;
      this.operator2 = operator2;
      this.expr3 = expr3;
    }

    public final Expr expr1;
    public final Token operator1;
    public final Expr expr2;
    public final Token operator2;
    public final Expr expr3;

    @Override
    public <R> R accept(Visitor<R> visitor) {
      return visitor.visitTernaryExpr(this);
    }
  }
  public static class Grouping extends Expr {
    public Grouping(Expr expression) {
      this.expression = expression;
    }

    public final Expr expression;

    @Override
    public <R> R accept(Visitor<R> visitor) {
      return visitor.visitGroupingExpr(this);
    }
  }
  public static class Literal extends Expr {
    public Literal(Object value) {
      this.value = value;
    }

    public final Object value;

    @Override
    public <R> R accept(Visitor<R> visitor) {
      return visitor.visitLiteralExpr(this);
    }
  }
  public static class Unary extends Expr {
    public Unary(Token operator, Expr right) {
      this.operator = operator;
      this.right = right;
    }

    public final Token operator;
    public final Expr right;

    @Override
    public <R> R accept(Visitor<R> visitor) {
      return visitor.visitUnaryExpr(this);
    }
  }

  public abstract <R> R accept(Visitor<R> visitor);
}
