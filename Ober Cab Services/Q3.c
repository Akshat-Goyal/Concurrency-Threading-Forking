#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h> 
#include <wait.h>
#include <time.h>
#include <unistd.h>

typedef struct Cab{
	int id, cabType, riderId[2];   
}Cab;

typedef struct Rider{
	int id, rideTime, cabType, maxWaitTime, cabId, isWaiting, payment, isDone;
	pthread_t tid;
	pthread_cond_t cond;
}Rider;

typedef struct Server{
	int id;
	pthread_t tid;
}Server;

int N, M, K, *riderState;
Rider *riders;
Cab *Cabs;
Server *servers;
pthread_mutex_t cabLock, serverLock;
sem_t sem;

void BookCab(Rider* rider){
	struct timespec ts;
	pthread_mutex_lock(&cabLock);
	if(rider->cabType == 1){
		for(int i=0; i<N; i++){
			if(Cabs[i].cabType == 0){
				rider->cabId = Cabs[i].id;
				Cabs[i].cabType = 1;
				Cabs[i].riderId[0] = rider->id;
				printf("Cab %d is going to pick up Rider %d\n", rider->cabId+1, rider->id+1); fflush(stdout);
				break;
			}
		}
	}
	else{
		for(int i=0; i<N; i++){
			if(Cabs[i].cabType == 2){
				rider->cabId = Cabs[i].id;
				Cabs[i].cabType = 3;
				Cabs[i].riderId[1] = rider->id;
				printf("Cab %d is going to pick up Rider %d\n", rider->cabId+1, rider->id+1); fflush(stdout);
				break;
			}
		}
		if(rider->cabId == -1){
			for(int i=0; i<N; i++){
				if(Cabs[i].cabType == 0){
					rider->cabId = Cabs[i].id;
					Cabs[i].cabType = 2;
					Cabs[i].riderId[0] = rider->id;
					printf("Cab %d is going to pick up Rider %d\n", rider->cabId+1, rider->id+1); fflush(stdout);
					break;
				}
			}
		}
	}
	if(rider->cabId == -1){
		rider->isWaiting = 1;
		clock_gettime(CLOCK_REALTIME, &ts);
		ts.tv_sec += rider->maxWaitTime;
		pthread_cond_timedwait(&(rider->cond), &(cabLock), &ts);
		rider->isWaiting = 0;
	}
	pthread_mutex_unlock(&cabLock);
	if(rider->cabId != -1){
		if(rider->cabType == 1){	
			printf("Rider %d is riding in Premier Cab %d\n", rider->id+1, rider->cabId+1), fflush(stdout);
		}
		else if(rider->cabType == 2){
			printf("Rider %d is riding in Pool Cab %d\n", rider->id+1, rider->cabId+1), fflush(stdout);
		}
		sleep(rider->rideTime);
		if(rider->cabType == 2)
			printf("Rider %d in Pool Cab %d has reached to the destination\n", rider->id+1, rider->cabId+1), fflush(stdout);
		else
			printf("Rider %d in Premier Cab %d has reached to the destination\n", rider->id+1, rider->cabId+1), fflush(stdout);
	}
	else{
		printf("Rider %d did not get any Cab\n", rider->id+1);
		pthread_mutex_lock(&serverLock);
		rider->isDone = 1;
		pthread_mutex_unlock(&serverLock);
		return;
	}
	pthread_mutex_lock(&cabLock);
	if(Cabs[rider->cabId].cabType == 3){
		printf("Cab %d has one customer and searching for other customer\n", rider->cabId+1), fflush(stdout);
		Cabs[rider->cabId].cabType = 2;
		if(Cabs[rider->cabId].riderId[0] == rider->id) Cabs[rider->cabId].riderId[0] == Cabs[rider->cabId].riderId[1];
		for(int i=0; i<M; i++){
			if(riders[i].cabType == 2 && riders[i].isWaiting == 1){
				riders[i].isWaiting = 0;
				riders[i].cabId = rider->cabId;
				Cabs[rider->cabId].cabType = 3;
				Cabs[rider->cabId].riderId[1] = riders[i].id;
				printf("Cab %d is going to pick up Rider %d\n", riders[i].cabId+1, riders[i].id+1); fflush(stdout);
				pthread_cond_signal(&(riders[i].cond));
				break;
			}
		}
	}
	else{
		printf("Cab %d is idle now and waiting for the customers\n", rider->cabId+1), fflush(stdout);
		Cabs[rider->cabId].cabType = 0;
		for(int i=0; i<M; i++){
			if((riders[i].cabType == 1 || riders[i].cabType == 2) && riders[i].isWaiting == 1){
				riders[i].isWaiting = 0;
				riders[i].cabId = rider->cabId;
				Cabs[rider->cabId].cabType = riders[i].cabType;
				Cabs[rider->cabId].riderId[0] = riders[i].id;
				printf("Cab %d is going to pick up Rider %d\n", riders[i].cabId+1, riders[i].id+1); fflush(stdout);
				pthread_cond_signal(&(riders[i].cond));
				break;
			}
		}
	}
	pthread_mutex_unlock(&cabLock);
}

void MakePayment(int riderId){
	printf("Rider %d is waiting for the server to make payment\n", riderId+1); fflush(stdout);
	riders[riderId].payment = 1;
	sem_post(&sem);
}

void* initialize_riders(void* arg){
	int arrivalTime = rand()%10;
	sleep(arrivalTime);
	Rider *rider = (Rider*) arg;
	rider->rideTime = rand()%5+1;
	rider->maxWaitTime = rand()%3+1;
	rider->cabType = rand()%2+1;
	rider->cabId = -1;
	if(rider->cabType == 1)
		printf("Rider %d is waiting for a Premier Cab\n", rider->id+1), fflush(stdout);
	else
		printf("Rider %d is waiting for a Pool Cab\n", rider->id+1), fflush(stdout);
	BookCab(rider);
	if(rider->cabId != -1) MakePayment(rider->id);
}

void* acceptPayment(void* arg){
	Server *server = (Server*) arg;
	int done;
	while(1){
		sem_wait(&sem);
		sleep(2);
		done = 0;
		pthread_mutex_lock(&serverLock);
		for(int i=0; i<M; i++){
			if(riders[i].payment == 1){
				riders[i].payment = 0;
				riders[i].isDone = 1;
				done++;
				printf("Server %d has accepted payment from Rider %d\n", server->id+1, riders[i].id+1); fflush(stdout);
				break;
			}
			else if(riders[i].isDone == 1) done++;
		}
		pthread_mutex_unlock(&serverLock);
		if(done == M) break;
	}
}

int main(){

	srand(time(0));
	printf("Enter no. of Cabs\n");
	scanf("%d", &N);
	printf("Enter no. of Riders\n");
	scanf("%d", &M);
	printf("Enter no. of Payment Servers\n");
	scanf("%d", &K);

	sem_init(&sem, 0, 0); 
	pthread_mutex_init(&(cabLock), NULL);
	pthread_mutex_init(&(serverLock), NULL);
	Cabs = (Cab*) malloc(sizeof(Cab)*N);
	for(int i=0; i<N; i++){
		Cabs[i].id = i;
		Cabs[i].cabType = 0;
	}

	riders = (Rider*) malloc(sizeof(Rider)*M);
	for(int i=0; i<M; i++){
		riders[i].id = i;
		riders[i].isWaiting = 0;
		riders[i].payment = 0;
		riders[i].isDone = 0;
		pthread_cond_init(&(riders[i].cond), NULL);
		int status = pthread_create(&(riders[i].tid), NULL, initialize_riders, &riders[i]);
		if(status){
			perror("Error in creating Rider thread");
		}
	}

	servers = (Server*) malloc(sizeof(Server)*K);
	for(int i=0; i<K; i++){
		servers[i].id = i;
		int status = pthread_create(&(servers[i].tid), NULL, acceptPayment, &servers[i]);
		if(status){
			perror("Error in creating Server thread");
		}
	}

	for(int i=0; i<M; i++){
		pthread_join(riders[i].tid, NULL);
	}
	for(int i=0; i<K; i++){
		sem_post(&sem);
	}
	for(int i=0; i<K; i++){
		pthread_join(servers[i].tid, NULL);
	}
	sem_destroy(&sem);
	printf("Simulation Over\n"); fflush(stdout);
	return 0;
}