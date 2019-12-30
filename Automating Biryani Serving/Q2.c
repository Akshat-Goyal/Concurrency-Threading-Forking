#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>


typedef struct Chef{
	int id, vessels, p, isready;                 // Each vessel has the capacity to feed p students
	pthread_t tid;
	pthread_cond_t ccond;
	pthread_mutex_t lock;
}Chef;

typedef struct Table{
	int id, slots, p, *st, top; 
	pthread_t tid;
	pthread_cond_t tcond;
	pthread_mutex_t lock;
}Table;

typedef struct Student{
	int id, tableid, slot;
	pthread_t tid;
}Student;

int M, N, K, studentsToBeServed;
Chef *chefs;
Table *tables;
Student *students;

void push(int* st, int top, int y){
	if(top >= 9){
		printf("Table is full\n");
		return;
	}
	st[++top] = y;
}

int pop(int *st, int top){
	if(top > -1){
		return st[top];
	}
	printf("Table is empty\n");
	return 0;
}

void biryani_ready(Chef *chef){
	pthread_mutex_t *lock = &(chef->lock);
	pthread_mutex_lock(lock);
	printf("Robot Chef %d has prepared %d vessels of Biryani. Waiting for all the vessels to be emptied to resume cooking\n", chef->id+1, chef->vessels); fflush(stdout);
	chef->isready = 1;
	if(chef->vessels && studentsToBeServed) pthread_cond_wait(&(chef->ccond), lock);
	chef->isready = 0;
	pthread_mutex_unlock(lock);
}

void* cook_biryani(void* arg){
	Chef *chef = (Chef*) arg;
	pthread_mutex_t * lock = &(chef->lock);
	while(studentsToBeServed){
		int w = rand()%4 + 2;
		chef->vessels = rand()%10 + 1;
		chef->p = rand()%26 + 25;
		printf("Robot Chef %d is preparing %d vessels of Biryani\n", chef->id+1, chef->vessels); fflush(stdout);
		sleep(w);
		biryani_ready(chef);
		if(!chef->vessels) printf("All the vessels prepared by Robot Chef %d are emptied. Resuming cooking now\n", chef->id+1), fflush(stdout);
	}
}

void load_biryani(Table *table){
	int status = 1;
	pthread_mutex_t *clock, *tlock;
	while(status && studentsToBeServed){
		for(int i=0; i<M; i++){
			clock = &(chefs[i].lock);
			pthread_mutex_lock(clock);
			if(chefs[i].isready){
				tlock = &(table->lock);
				pthread_mutex_lock(tlock);
				table->p = chefs[i].p;
				printf("Robot Chef %d is refilling Serving Container of Serving Table %d\n", chefs[i].id+1, table->id+1); fflush(stdout);
				printf("Serving Container of Table %d is refilled by Robot Chef %d; Table %d is resuming serving now\n", table->id+1, chefs[i].id+1, table->id+1); fflush(stdout);
				pthread_mutex_unlock(tlock);
				chefs[i].vessels--;
				if(chefs[i].vessels == 0){
					pthread_cond_broadcast(&(chefs[i].ccond));
				}
				pthread_mutex_unlock(clock);
				status = 0;
				break;
			}
			pthread_mutex_unlock(clock);
		}
	}
}

void ready_to_serve_table(Table *table){
	pthread_mutex_t * lock = &(table->lock);
	pthread_mutex_lock(lock);
	if(table->slots && studentsToBeServed){
		pthread_cond_wait(&(table->tcond), lock);
	}
	pthread_mutex_unlock(lock);
}

void* serve_biryani(void* arg){
	Table *table = (Table*) arg;
	pthread_mutex_t * lock = &(table->lock);
	while(studentsToBeServed){
		printf("Serving Container of Table %d is empty, waiting for refill\n", table->id+1); fflush(stdout);
		load_biryani(table);
		pthread_mutex_lock(lock);
		while(studentsToBeServed && table->p){
			table->slots = rand()%10%table->p + 1;
			table->p -= table->slots;
			printf("Serving Table %d is ready to serve with %d slots\n", table->id+1, table->slots); fflush(stdout);
			pthread_mutex_unlock(lock);
			ready_to_serve_table(table);
			pthread_mutex_lock(lock);
		}
		pthread_mutex_unlock(lock);
	}
}

void student_in_slot(Student* student){
	printf("Student %d is assigned a slot on the serving table %d and waiting to be served\n", student->id+1, student->tableid+1);
	fflush(stdout);
	pthread_mutex_t *lock = &(tables[student->tableid].lock);
	pthread_mutex_lock(lock);
	studentsToBeServed--;
	if(tables[student->tableid].slots == 0){
		printf("Serving table %d entering Serving Phase\n", student->tableid+1); fflush(stdout);
		while(tables[student->tableid].top > -1){
			printf("Student %d on Serving Table %d has been served\n", pop(tables[student->tableid].st, tables[student->tableid].top)+1, tables[student->tableid].id+1);
			tables[student->tableid].top--;
			fflush(stdout);
		}
		pthread_cond_broadcast(&(tables[student->tableid].tcond));
	}
	pthread_mutex_unlock(lock);			
}

void *wait_for_slot(void* arg){
	Student *student = (Student*) arg;
	int arrivalTime = rand()%10;
	sleep(arrivalTime);
	printf("Student %d has arrived\n", student->id+1); fflush(stdout);
	printf("Student %d is waiting to be allocated a slot on the serving table\n", student->id+1); fflush(stdout);
	int status = 1;
	pthread_mutex_t *lock;
	while(status){
		for(int i=0; i<N; i++){
			lock = &(tables[i].lock);
			pthread_mutex_lock(lock);
			if(tables[i].slots != 0){
				student->tableid = tables[i].id;
				student->slot = tables[i].slots;
				tables[i].slots--;
				push(tables[i].st, tables[i].top, student->id);
				tables[i].top++;
				status = 0;
				pthread_mutex_unlock(lock);
				break;
			}
			pthread_mutex_unlock(lock);
		}
	}
	student_in_slot(student);
}

int main(){

	srand(time(0));
	printf("Enter the number of Robot Chefs\n");
	scanf("%d", &M);
	printf("Enter the number of Serving Tables\n");
	scanf("%d", &N);
	printf("Enter the number of Registered Students\n");
	scanf("%d", &K);
	
	studentsToBeServed = K;
	chefs = (Chef*) malloc(sizeof(Chef)*M);
	for(int i=0, status; i<M; i++){
		chefs[i].id = i;
		chefs[i].isready = 0;
		pthread_mutex_init(&(chefs[i].lock), NULL);
		pthread_cond_init(&(chefs[i].ccond), NULL);
		status = pthread_create(&(chefs[i].tid), NULL, cook_biryani, &chefs[i]);
		if(status){
			perror("Error in creating Chef thread");
		}
	}

	tables = (Table*) malloc(sizeof(Table)*N);
	for(int i=0, status; i<N; i++){
		tables[i].id = i;
		tables[i].slots = 0;
		tables[i].st = (int*) malloc(sizeof(int)*10);
		tables[i].top = -1;
		pthread_mutex_init(&(tables[i].lock), NULL);
		pthread_cond_init(&(tables[i].tcond), NULL);
		status = pthread_create(&(tables[i].tid), NULL, serve_biryani, &tables[i]);
		if(status){
			perror("Error in creating Serving table thread");
		}
	}

	students = (Student*) malloc(sizeof(Student)*K);
	for(int i=0, status; i<K; i++){
		students[i].id = i;
		students[i].tableid = -1;
		status = pthread_create(&(students[i].tid), NULL, wait_for_slot, &students[i]);
		if(status){
			perror("Error in creating Student thread");
		}
	}

	for(int i=0; i<K; i++){
		pthread_join(students[i].tid, NULL);
	}
	studentsToBeServed = 0;
	for(int i=0; i<N; i++){
		if(tables[i].top > -1) printf("Serving table %d entering Serving Phase\n", tables[i].id+1), fflush(stdout);
		while(tables[i].top > -1){
			printf("Student %d on Serving Table %d has been served\n", pop(tables[i].st, tables[i].top)+1, i+1);
			fflush(stdout);
			tables[i].top--;
		}
	}
	printf("Simulation Over\n"); fflush(stdout);
	return 0;
}