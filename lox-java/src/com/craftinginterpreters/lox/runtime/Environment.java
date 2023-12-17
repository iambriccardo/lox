package com.craftinginterpreters.lox.runtime;

import java.util.ArrayList;
import java.util.List;

public class Environment {

    private final Environment enclosing;
    private final List<Object> values = new ArrayList<>();

    public Environment(Environment enclosing) {
        this.enclosing = enclosing;
    }

    public void define(Object value) {
        values.add(value);
    }

    public void assignAt(Location location, Object value) {
        ancestor(location.getDistance()).values.set(location.getVariableIndex(), value);
    }

    public Object getAt(Location location) {
        return ancestor(location.distance).values.get(location.getVariableIndex());
    }

    private Environment ancestor(int distance) {
        Environment environment = this;
        for (int i = 0; i < distance; i++) {
            environment = environment.enclosing;
        }

        return environment;
    }

    public static class Location {
        private final int distance;
        private final int variableIndex;

        public Location(int distance, int variableIndex) {
            this.distance = distance;
            this.variableIndex = variableIndex;
        }

        public int getDistance() {
            return distance;
        }

        public int getVariableIndex() {
            return variableIndex;
        }
    }
}