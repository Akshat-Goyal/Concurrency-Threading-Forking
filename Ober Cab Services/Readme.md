PROBLEM STATEMENT:
==================

+ Ober is a new cab service, which needs your help to implement the simple system.The requirements of the system are as follows given N cabs, M riders and K payment servers you need to implement a working system which ensures correctness and idempotency. Each cab has one driver. In Ober, payments by the riders should be done in Payment Servers only. Ober provides two types of cab services namely pool and premier ride. In pool ride maximum of two riders can share cab whereas in premier ride only one rider. There are four states for a cab namely:
  waitState ​ (no rider in cab), ​ onRidePremier​ , ​ onRidePoolFull ​ (pool ride with two riders), ​ onRidePoolOne​ (pool ride with only one rider).
+ As a part of this system you need to implement the following functionalities.
+ #### Riders​ :
  1. BookCab: This function takes three inputs namely ​ cabType, maxWaitTime, ​RideTime . If the rider doesn’t find any cab (all cabs are in usage) until ​ maxWaitTime ​ he exits from the system with ​ TimeOut message.
  2. MakePayment: This function should be called by rider only after the end of the ride. If all the K payment servers are busy, then the rider should wait for the payment servers to get free. After payment is done rider can exit from the system.
+ #### Drivers​ :
  1. AcceptRide: If ride is premier cab in ​ waitState ​ should accept ride and change its state to ​ onRidePremier . ​ If the ride is pool and there is a cab with state ​ onRidePoolOne ​ then that cab should accept the ride and changes its state to ​ onRidePoolFull ​ . If the ride is pool and there is no cab with the state ​ onRidePoolOne ​ then cab in wait state should accept ride and change its state to ​ onRidePoolOne . ​ Assume that there is no time gap between accept rider and pickup rider, i.e., ride starts immediately after accepting ride. If there are multiple cabs available with required state take any random cab.
  2. OnRide: When pool cab is on ride and state is ​ onRidePoolOne ​ then driver can accept another pool ride. No new rider is accepted when cab is on premier ride.
  3. EndRide: The driver ends the ride after completion of the ​ RideTime ​ of the rider. If the ride is a premier it goes to ​ waitState ​ (ready to accept new rides) once ride is done. If cab’s state is ​ onRidePoolOne ​ then driver ends the ride and goes to ​ waitState . ​ If cab’s state is onRidePoolFull ​ then it goes to ​ onRidePoolOne ​ state (Notably, it can accept another pool rider). Driver or cab need not wait for the payment to be done by the rider.

+ #### Payment Servers:
1. Accept payment: Payment servers accepts a payment and assume the payment time is constant for all the riders (assume 2sec).

+ #### Instructions​ :
1. Your code must not result in busy-waiting (deadlocks).
2. Use appropriate times for RideTime and the maxWaitTime.
3. You need not follow above mentioned functions you can create functions appropriately.
4. Use Semaphores and mutex accordingly.
5. Use separate threads for each of the riders and payment servers.
6. Use only four states of Cab as mentioned above.


SOLUTION EXPLAINED:
===================

1. Cab: Has id of Cab, cabType (0 for ​ waitState ​, 1 for onRidePremier ​, 2 for onRidePoolOne, 3 for ​ onRidePoolFull) and a riderId[2] to store the ids of the customers.
2. Rider: Has id of Rider,tid is thread id corresponding to that rider, cabType(1 for Premiun ride, 2 for Pool ride), maxWaitTime is max. time rider waits for cab, cabId is id of cab in which it is riding, isWaiting (1 if waiting for the cab), payment (1 if payment pending), isDone (1 if rider's maxWaitTime ends or ride completes), cond type is pthread_cond_t corresponding to it's tid.
3. Server: Has id of Server,tid id thread id corresponding to that server.
4. One semaphore 
  + sem - For servers (initialized to Zero)
5. Two Mutex
  + cabLock - for cabs
  + serverLock - for servers
6. Threads for riders , servers and structs for each of them are created and joined in the main function.

IMPORTANT FUNCTIONS:
----------------------------------------------------------

### initialize_riders
+ Waits for the arrival of the rider, sets rideTime, maxWaitTime, cabType, calls BookCab and MakePayment 

### BookCab
+ It acquires cabLock mutex, and checks if any cab has the desired cabType, releases mutex. Now sleeps for rideTime if ride was found, otherwise waits (pthread_cond_timedwait) for maxWaitTime for cab's signal. If any cab sends signal, then it goes to sleep for rideTime.     
+ If no cab is found till maxWaitTime, it returns.
+ When cab gets free, then it acquires cabLock mutex and checks if any cab is waiting for this cabType and sends signal to that rider and releases mutex.
+ After completing the ride, MakePayment is called.


### MakePayment
+ Here rider calls the sem_post and releases the server to accept the payment.


### AcceptPayment
+ This function is called by the server threads initially.
+ This functions waits in sem_wait until any rider releases it to accept payment
+ It acquires serverLock mutex, if all riders are done, then it returns after releasing mutex.


RUNNING IT:
-----------
1. gcc -pthread Q3.c
2. ./a.out
