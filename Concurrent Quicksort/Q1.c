#include <stdio.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <unistd.h>
#include <wait.h>
#include <pthread.h>


struct thread_data{
	int l, r, *arr;
};

int * shareMem(size_t size){
    key_t mem_key = IPC_PRIVATE;
    int shm_id = shmget(mem_key, size, IPC_CREAT | 0666);
    return (int*)shmat(shm_id, NULL, 0);
}

void swap(int *a, int *b){
	int temp = *a;
	*a = *b;
	*b = temp;
}

void insertionSort(int *arr, int l, int r){

	for(int i=l+1, j, x;i<=r;i++){

		x = arr[i];
		j = i-1;

		while(j>=l && arr[j]>x){
			arr[j+1] = arr[j];
            j--;
        }

		arr[j+1] = x;
	}
}

void normalQuicksort(int *arr, int l,int r){
	if(l >= r) return;

	if(r-l+1<5){
		insertionSort(arr, l, r);
	}
	else{
		int pivot = rand()%(r-l+1) + l, j=l-1;
		swap(arr+pivot, arr+r); pivot = r;
		for(int i=l; i<r; i++){
			if(arr[i] <= arr[pivot]){
				j++;
				swap(arr+i, arr+j);
			}
		}
		j++;
		swap(arr+j, arr+pivot);
		normalQuicksort(arr, l, j-1);
		normalQuicksort(arr, j+1, r);
	}
	return;
}

void concurrentQuicksort(int *arr, int l, int r){
	if(l >= r) return;

	if(r-l+1 < 5){
		insertionSort(arr, l, r);
	}
	else{
		int pivot = rand()%(r-l+1) + l, j=l-1;
		swap(arr+pivot, arr+r); pivot = r;
		for(int i=l; i<r; i++){
			if(arr[i] <= arr[pivot]){
				j++;
				swap(arr+i, arr+j);
			}
		}
		j++;
		swap(arr+j, arr+pivot);
		int pid1 = fork();
		if(pid1 == 0){
			concurrentQuicksort(arr, l, j-1);
			_exit(0);
		}
		else if(pid1 > 0){
			int pid2 = fork();
			if(pid2 == 0){
				concurrentQuicksort(arr, j+1, r);
				_exit(0);
			}
			else if(pid2 > 0){
				int status;
				waitpid(pid1, &status, 0);
				waitpid(pid2, &status, 0);
			}
		}
	}
	return;
}

void *threadedQuicksort(void *ar){

	struct thread_data *a = (struct thread_data*) ar;

	int l = a->l;
    int r = a->r;
    int *arr = a->arr;
    if(l>=r) return NULL;

	if(r-l+1<5){
		insertionSort(arr, l, r);
		return NULL;
	}

	int pivot = rand()%(r-l+1) + l, j=l-1;
	swap(arr+pivot, arr+r); pivot = r;
	for(int i=l; i<r; i++){
		if(arr[i] <= arr[pivot]){
			j++;
			swap(arr+i, arr+j);
		}
	}
	j++;
	swap(arr+j, arr+pivot);
	struct thread_data a1;
    a1.l = l;
    a1.r = j-1;
    a1.arr = arr;
    pthread_t tid1;
    pthread_create(&tid1, NULL, threadedQuicksort, &a1);

	struct thread_data a2;
    a2.l = j+1;
    a2.r = r;
    a2.arr = arr;
    pthread_t tid2;
    pthread_create(&tid2, NULL, threadedQuicksort, &a2);

	pthread_join(tid1, NULL);
    pthread_join(tid2, NULL);
	return NULL;
}

void runsorts(int n){

	struct timespec ts;

	int *arr = shareMem(sizeof(int)*n);
	int *brr = (int*) malloc(sizeof(int)*n);
	int *crr = (int*) malloc(sizeof(int)*n);
    for(int i=0;i<n;i++){
		scanf("%d", arr+i);
		brr[i] = arr[i];
		crr[i] = arr[i];
	}

    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    long double st = ts.tv_nsec/(1e9)+ts.tv_sec;
	concurrentQuicksort(arr, 0, n-1);
	clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    long double en = ts.tv_nsec/(1e9)+ts.tv_sec;
	printf("\nConcurrent Quicksort\n");
	for(int i=0; i<n; i++){
		printf("%d ", arr[i]);
	}
	printf("\n");
	shmdt(arr);	
    long double t1 = en-st;
    printf("\nTime taken by Concurrent Quicksort = %Lf\n", t1);

	pthread_t tid;
    struct thread_data a;
    a.l = 0;
    a.r = n-1;
    a.arr = brr;

    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    st = ts.tv_nsec/(1e9)+ts.tv_sec;
    pthread_create(&tid,NULL,threadedQuicksort,&a);
    pthread_join(tid,NULL);
	clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
	en = ts.tv_nsec/(1e9)+ts.tv_sec;
	printf("\nThreaded Quicksort\n");
	for(int i=0; i<n; i++){
		printf("%d ", brr[i]);
	}
	printf("\n");
	free(brr);
	long double t2 = en-st;
    printf("\nTime of Threaded Quicksort = %Lf\n", t2);

	clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    st = ts.tv_nsec/(1e9)+ts.tv_sec;
	normalQuicksort(crr, 0, n-1);
	clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    en = ts.tv_nsec/(1e9)+ts.tv_sec;
	printf("\nNormal Quicksort\n");
	for(int i=0; i<n; i++){
		printf("%d ", crr[i]);
	}
	printf("\n");
	free(crr);
    long double t3 = en - st;
    printf("\nTime of Normal Quicksort = %Lf\n", t3);

	printf("\nNormal Quicksort ran:\n\t[ %Lf ] times faster than Concurrent Quicksort\n\t[ %Lf ] times faster than Threaded Concurrent Quicksort\n\n", t1/t3, t2/t3);
    return;
}

int main(){
	int n;
	scanf("%d", &n);
	runsorts(n);
	return 0;
}