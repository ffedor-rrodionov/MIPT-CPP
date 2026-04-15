#include <iostream>

bool IsUnique(int* permutation, int size) {
  for (int i = 0; i < size; i++) {
    for (int j = 0; j < size; j++) {
      if (i != j && permutation[i] == permutation[j]) {
        return false;
      }
    }
  }
  return true;
}

void Permutations(long long* answer, int placed,
                  int stop, int* permutation, int** arrays,
                  int* sizes) {
  if (placed == stop) {
    if (IsUnique(permutation, stop)) {
      long long sum = 1;
      for (int i = 0; i < stop; i++) {
        sum = sum * arrays[i][permutation[i]];
      }
      *answer += sum;
    }
    return;
  }
  for (int i = 0; i < sizes[placed]; i++) {
    permutation[placed] = i;
    Permutations(answer, placed + 1, stop, permutation, arrays, sizes);
  }
}

int main(int argc, char *argv[]) {
  long long answer = 0;
  int k = argc - 1;
  int** arrays = new int*[k];
  int* sizes = new int[k];
  int size_i;
  int* permutation = new int[k];
  for (int i = 0; i < k; i++) {
    sscanf(argv[i + 1], "%d", &size_i);
    arrays[i] = new int[size_i];
    sizes[i] = size_i;
  }
  for (int i = 0; i < k; i++) {
    for (int j = 0; j < sizes[i]; j++) {
      std::cin >> arrays[i][j];
    }
  }
  Permutations(&answer, 0, k, permutation, arrays, sizes);
  std::cout << answer;

  for (int i = 0; i < k; i++) {
    delete[] arrays[i];
  }
  delete[] arrays;
  delete[] sizes;
  delete[] permutation;
}
