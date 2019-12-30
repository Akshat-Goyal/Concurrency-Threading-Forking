PROBLEM STATEMENT:
==================
Implement a Concurrent and Threaded version of Quicksort algorithm and compare the performance of Concurrent, Threaded and Normal Quicksort.

SOLUTION EXPLAINED:
===================
+ Randomized Quicksort is used to sort the array in increasing order in all versions.
+ #### runsorts
	+ runs all the sorts and stores the running time of each version of sort.
+ #### threadedQuicksort
	+ Two threads are made recursively, one of which will sort the low subarray and the other will sort the high subarray.
	+ pthread_create and pthread_join functions are used.
+ #### concurrentQuicksort
	+ Two child is made recursively, one of which will sort the low subarray and the other will sort the high subarray.
	+ Shmget and Shmat functions are for accessing the shared memory.
+ #### insertionSort
	+ When the number of elements in the array for a process is less than 5, insertion sort is performed.

Performance Analysis:
===================
N			NormalSort			ThreadedSort	        ConcurrentSort
200     	0.000019      	  	0.004738            	0.006313
2000     	0.000196			0.040387				0.070635
8000		0.000885			0.118254				0.386283
10000		0.001064			0.150484				0.501118
50000		0.007547			0.900363				4.228456
1000000		0.013843			Seg Fault				6.022700

+ The Worst Case Complexity is taken as array was sorted in decreasing order.
+ Randomized Quicksort with threading takes much more time due to creation of too many threads. The SegFault at large input therefore occurs due to the creation of too mamny threads (O(log(n)) number of threads).
+ Similarly, concurrent sort takes more time than normal sort due to the creation of a lot of processes which increase the number of page faults, context switches and CPU migration.
+ Threaded Sort takes less time than Concurrent Sort as threads share memory and caches but for concurrent sort, we are creating two processes which require the os to make virtual memory and Shared Memory region and accessing data in that region takes time.