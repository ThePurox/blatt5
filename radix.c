/*
 * group: Nico Stuhlmüller, Tobias Eckert
 * compile: gcc radixpara.c -lm -lpthread
 * run: ./a.out <num-threads>
 */

/* one might not want to use bubblesort, but it is working so far */

#include <pthread.h>
#include<stdio.h>
#include<stdlib.h>
#include<time.h>
#include <math.h>

#define BASE 10

typedef struct chunk_t
{
  int* numbers;
  int* chunk;
  int digits;
  int* hist;
  int index;
} chunk_t;



int max(int* numbers, int size){
  int m = -1;
  int i;
  for(i=0; i<size; i++){
    if(numbers[i]>m)
      m=numbers[i];
  }
  return m;
}

int digits(int n){
  int d = 0;
  while(n != 0){
    n /= BASE;
    d++;
  }
  return d;
}

int intpow(int base, int exp){
  int i;
  int res = 1;
  for(i=0; i<exp; i++)
    res *= base;
  return res;
}

void printNum(int* numbers, int size){
  int i;
  for(i=0; i<size; i++){
    printf("%d ", numbers[i]);
  }
  printf("\n");
}

void bubblesort(int *numbers,int digit, int size){
  int* sorted = malloc(sizeof(int) * size);
  int d,i;
  int factor = intpow(BASE,digit);
  int count = 0;
  for(d=0; d<BASE; d++){
    for(i=0; i<size; i++){
      if((numbers[i] / factor) % BASE == d){
        sorted[count] = numbers[i];
        count++;
      }
    }
  }
  for(i=0; i<size; i++){
    numbers[i] = sorted[i];
  }
  free(sorted);
}

void radix(int *numbers, int digits, int size){
  int d;
  for(d=0; d<digits; d++){

    bubblesort(numbers, d, size);
  }
}

void* thread_sort(void* arg){
  chunk_t* c = (chunk_t*) arg;
  radix(c->chunk, c->digits, c->hist[c->index]);

  // combine results
  int start = 0,i;
  for(i = 0; i<c->index; i++)
    start += c->hist[i];
  for(i=0; i<c->hist[c->index]; i++){
    c->numbers[start + i] = c->chunk[i];
  }
  return NULL;
}

int* numbers_random(int n){
  srandom(time(NULL));
  int* numbers = malloc(sizeof(int) * n);
  int i;
  for(i=0; i<n; i++)
    numbers[i] = abs((int)random());
  return numbers;
}

int* numbers_file(int* SIZE){
  FILE * f = fopen("numbers.txt","r");
  if(f == NULL){
    printf("opening file failed");
  }

  *SIZE = 0;
  int dump;
  while(fscanf(f, "%d ", &dump) == 1)
    (*SIZE)++;

  rewind(f);
  int* numbers = malloc(sizeof(int) * *SIZE);
  int i;
  for(i=0; i<*SIZE; i++)
    fscanf(f, "%d ", &numbers[i]);
  fclose(f);

  return numbers;
}

int main(int argc, char **argv){
  if(argc != 2)
    return -1;

  const int NUM = atoi(argv[1]);
  int SIZE;
  int* numbers;

  if(1){ // from file or not
    numbers = numbers_file(&SIZE);
  }
  else{
    SIZE = 2018;
    numbers = numbers_random(SIZE);
  }
  printf("%d\n", SIZE);

  clock_t start = clock();

  int m = max(numbers, SIZE);
  int d = digits(m);

  //profits from uniformly distributed numbers
  pthread_t threads[NUM];

  int* hist = calloc(NUM,sizeof(int));
  int* hist_temp = calloc(NUM,sizeof(int));
  int i;
  for(i=0; i<SIZE; i++)
    hist[(int)((long)numbers[i]*NUM/(m+1))]++; // int division

  for(i = 0; i < NUM; i++)
    hist_temp[i] = hist[i];

  int** chunks = (int**) malloc(sizeof(int*) * NUM);
  for(i = 0; i < NUM; i++)
    chunks[i] = malloc(sizeof(int) * hist[i]);

  for(i=0; i<SIZE; i++){
    int index = (int)((long)numbers[i]*NUM/(m+1)); // multiplication might overflow integer
    hist_temp[index]--;
    chunks[index][hist_temp[index]] = numbers[i];
  }
  free(hist_temp);

  for(i = 0; i < NUM; i++){
    chunk_t* c;
    c = (chunk_t*) malloc(sizeof(chunk_t));
    c->hist = hist;
    c->index = i;
    c->digits = d;
    c->numbers = numbers;
    c->chunk = chunks[i];

    pthread_create(&threads[i], NULL, thread_sort, c);
  }

  for(i = 0; i < NUM; i++)
    pthread_join(threads[i],NULL);

  clock_t end = clock();
  float seconds = (float)(end - start) / CLOCKS_PER_SEC;
  printf("Time: %f sec\n", seconds);

  //printNum(numbers, SIZE);
  free(numbers);

  return 0;
}
