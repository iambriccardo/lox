#include <stdio.h>
#include <stdlib.h>

struct node *HEAD = NULL;

struct node {
  char *value;
  struct node *next;
};

void insert(char *value) {
  struct node *new_node = malloc(sizeof(struct node));
  new_node->value = value;

  if (HEAD == NULL) {
    HEAD = new_node;
  } else {
    HEAD->next = new_node;
  }
}

void remove_element(int index) {
  struct node *prev = NULL;
  struct node *current = HEAD;

  if (index == 0 && HEAD != NULL) {
    HEAD = current->next;
    free(current);
    return;
  }

  int i = 0;
  while (i <= index && current != NULL) {
    if (i == index) {
      prev->next = current->next;
      free(current);
    } else {
      prev = current;
      current = current->next;
      i++;
    }
  }
}

void print_list(struct node *node) {
  struct node *current = node;

  while (current != NULL) {
    printf("%s ", current->value);
    current = current->next;
  }

  printf("\n");
}

int main() {
  insert("ciao");
  insert("come");
  print_list(HEAD);
  remove_element(0);
  print_list(HEAD);
  remove_element(0);
  print_list(HEAD);
}
