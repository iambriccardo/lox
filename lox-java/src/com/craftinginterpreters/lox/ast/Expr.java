package com.craftinginterpreters.lox.ast;

import java.util.List;
import com.craftinginterpreters.lox.lexer.Token;

public abstract class Expr {
public interface Visitor<R> {
    public R visitAssignExpr(Assign expr);
    public R visitBinaryExpr(Binary expr);
    public R visitTernaryExpr(Ternary expr);
    public R visitCallExpr(Call expr);
    public R visitGetExpr(Get expr);
    public R visitGroupingExpr(Grouping expr);
    public R visitLiteralExpr(Literal expr);
    public R visitLogicalExpr(Logical expr);
    public R visitSetExpr(Set expr);
    public R visitThisExpr(This expr);
    public R visitUnaryExpr(Unary expr);
    public R visitLambdaExpr(Lambda expr);
    public R visitVariableExpr(Variable expr);
  }
  public static class Assign extends Expr {
    public Assign(Token name, Expr value) {
      this.name = name;
      this.value = value;
    }

    public final Token name;
    public final Expr value;

    @Override
    public <R> R accept(Visitor<R> visitor) {
      return visitor.visitAssignExpr(this);
    }
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
  public static class Call extends Expr {
    public Call(Expr callee, Token paren, List<Expr> arguments) {
      this.callee = callee;
      this.paren = paren;
      this.arguments = arguments;
    }

    public final Expr callee;
    public final Token paren;
    public final List<Expr> arguments;

    @Override
    public <R> R accept(Visitor<R> visitor) {
      return visitor.visitCallExpr(this);
    }
  }
  public static class Get extends Expr {
    public Get(Expr object, Token name) {
      this.object = object;
      this.name = name;
    }

    public final Expr object;
    public final Token name;

    @Override
    public <R> R accept(Visitor<R> visitor) {
      return visitor.visitGetExpr(this);
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
  public static class Logical extends Expr {
    public Logical(Expr left, Token operator, Expr right) {
      this.left = left;
      this.operator = operator;
      this.right = right;
    }

    public final Expr left;
    public final Token operator;
    public final Expr right;

    @Override
    public <R> R accept(Visitor<R> visitor) {
      return visitor.visitLogicalExpr(this);
    }
  }
  public static class Set extends Expr {
    public Set(Expr object, Token name, Expr value) {
      this.object = object;
      this.name = name;
      this.value = value;
    }

    public final Expr object;
    public final Token name;
    public final Expr value;

    @Override
    public <R> R accept(Visitor<R> visitor) {
      return visitor.visitSetExpr(this);
    }
  }
  public static class This extends Expr {
    public This(Token keyword) {
      this.keyword = keyword;
    }

    public final Token keyword;

    @Override
    public <R> R accept(Visitor<R> visitor) {
      return visitor.visitThisExpr(this);
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
  public static class Lambda extends Expr {
    public Lambda(List<Token> params, List<Stmt> body) {
      this.params = params;
      this.body = body;
    }

    public final List<Token> params;
    public final List<Stmt> body;

    @Override
    public <R> R accept(Visitor<R> visitor) {
      return visitor.visitLambdaExpr(this);
    }
  }
  public static class Variable extends Expr {
    public Variable(Token name) {
      this.name = name;
    }

    public final Token name;

    @Override
    public <R> R accept(Visitor<R> visitor) {
      return visitor.visitVariableExpr(this);
    }
  }

  public abstract <R> R accept(Visitor<R> visitor);
}
