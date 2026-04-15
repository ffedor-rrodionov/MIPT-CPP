#include <iostream>
#include <cstring>

char* InputString(int* length) {
  int capacity = 8;
  char* string = new char[8];
  char c;
  int index = 0;
  c = std::cin.get();
  string[index] = c;
  while (c != '\n') {
    if (index == capacity - 1) {
      capacity *= 2;
      char* new_memory = new char[capacity];
      memcpy(new_memory, string, index);
      delete[] string;
      string = new_memory;
    }
    string[index] = c;
    c = std::cin.get();
    index++;
  }
  *length = index;
  string[index] = '\0';
  return string;
}

void PrintOK() {
  std::cout << "ok\n";
}

void PrintError() {
  std::cout << "error\n";
}

bool IsExit(const char* input) {
  return std::strncmp(input, "exit", 4) == 0;
}

bool IsPush(const char* input) {
  return std::strncmp(input, "push", 4) == 0;
}

bool IsPop(const char* input) {
  return std::strncmp(input, "pop", 3) == 0;
}

bool IsBack(const char* input) {
  return std::strncmp(input, "back", 4) == 0;
}

bool IsSize(const char* input) {
  return std::strncmp(input, "size", 4) == 0;
}

bool IsClear(const char* input) {
  return std::strncmp(input, "clear", 5) == 0;
}

char** CreateBigStack(char** old_stack, int* const capacity) {
  char** new_stack = new char*[*capacity*2];
  for (int i = 0; i < *capacity; i++) {
    new_stack[i] = old_stack[i];
  }
  *capacity = *capacity*2;
  delete[] old_stack;
  return new_stack;
}

char** CreateSmallStack(char** old_stack, int* const capacity) {
  char** new_stack = new char*[*capacity/2];
  for (int i = 0; i < *capacity/2; i++) {
    new_stack[i] = old_stack[i];
  }
  *capacity = *capacity/2;
  delete[] old_stack;
  return new_stack;
}

int main() {
  char** stack = new char*[1];
  int stack_capacity = 1;
  int size = 0;
  int length;
  char* input = InputString(&length);
  while (!IsExit(input)) {
    if (IsSize(input)) { std::cout << size << "\n"; }
    else if (IsBack(input)) {
      if (size > 0) { std::cout << stack[size - 1] << "\n"; }
      else { PrintError(); }
    }
    else if (IsClear(input)) {
      for (int i = 0; i < size; ++i) { delete[] stack[i]; }
      size = 0;
      PrintOK();
    }
    else if (IsPop(input)) {
      if (4 * size == stack_capacity) {
        stack = CreateSmallStack(stack, &stack_capacity);
      }
      if (size > 0) {
        std::cout << stack[size - 1] << "\n";
        delete[] stack[size - 1];
        --size;
      } else { PrintError(); }
    }
    else if (IsPush(input)) {
      if (size == stack_capacity) {
        stack = CreateBigStack(stack, &stack_capacity);
      }
      ++size;
      stack[size - 1] = new char[length - 4];
      strcpy(stack[size - 1], input + 5);
      PrintOK();
    }
    delete[] input;
    input = InputString(&length);
  }
  std::cout << "bye\n";
  for (int i = 0; i < size; ++i) { delete[] stack[i]; }
  delete[] stack;
  delete[] input;
}