#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <unistd.h>
#include <time.h>
#define from_master 1
#define from_worker 2

int main(int argc, char *argv[])
{
	//declare variables
	int myrank;
	int nprocs;
	
	//MPI SET UP
	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
	MPI_Status status;
	MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
	
	//===========================================================================================
	//			MASTER PROCESS
	//===========================================================================================
	if(myrank == 0){
		//CODE SUBMITTED BY:
		printf("Code Submitted By:\nAleezeh Usman : 18I-0529\n--------------------------------\n");
	
		
		printf("------MASTER PROCESS HAS BEGUN------\n");
		int sizeofarray, return_var;
		
		//Take input from user and initialise dynamic array
		printf("Enter size of array:");
		scanf("%d", &sizeofarray);
		while(sizeofarray % (nprocs-1) != 0){
			printf("ERROR::Please enter a number divisible by %d\n", nprocs-1);
			scanf("%d", &sizeofarray);	
		}
		int* arr = (int*) calloc(sizeofarray, sizeof(int));
		//Randomly fill array (random numbers within range of size of array)
		int i = 0;
		srand(time(0));
		printf("ARRAY -> ");
		for(; i < sizeofarray; i++){
			int num = (rand() % sizeofarray + 1);
			arr[i] = num;
			printf("%d ", num);
		}
		printf("\n");
		//Take value to find as input
		int search_value;
		printf("Enter value to search:");
		scanf("%d", &search_value);
		
		//Send Data to worker processes to begin performing task
		printf("------SENDING DATA TO WORKER PROCESSES------\n");
		int division_size = sizeofarray/(nprocs-1);									//calculate how much data will be sent to each node
		printf("NOTE :: Each worker process will recieve %d elements to search from\n", division_size);
		int ind = 0;	//index to divide array 
		int dest = 1;
		for(; dest < nprocs ; dest++){
			//printf("SEND TO -> %d\n",dest );
			MPI_Send(&division_size, 1, MPI_INT, dest ,from_master, MPI_COMM_WORLD);				//send size being sent first
			MPI_Send(&arr[ind], 1*division_size, MPI_INT, dest,from_master, MPI_COMM_WORLD);			//then send data
			MPI_Send(&search_value, 1, MPI_INT, dest ,from_master, MPI_COMM_WORLD);
			ind += division_size;
		}
			
		//wait for workers to tell if they have ended their tasks
		int notfoundcount = 0;
		while(1){
			int abort_or_not = 0;           //will be used ad boolean to let other processes know if value found yet
			MPI_Recv(&return_var, 1, MPI_INT, MPI_ANY_SOURCE,from_worker, MPI_COMM_WORLD, &status);
			int src = status.MPI_SOURCE;    //where the signal has come from
			//*****VALUE NOT FOUND YET BUT STILL LOOKING*****
            		if(return_var == 0 ){           
				MPI_Send(&abort_or_not, 1, MPI_INT, src ,from_master, MPI_COMM_WORLD);
			}
			//****IF VALUE WAS FOUND*****
			if (return_var == 1){
				abort_or_not = 2;		//letting worker know that you found the correct searach value and can now terminate
				MPI_Send(&abort_or_not, 1, MPI_INT, src ,from_master, MPI_COMM_WORLD);
				MPI_Recv(&return_var, 1, MPI_INT, src ,from_worker, MPI_COMM_WORLD, &status);	//for synchronisation
				printf("---------------------\nMASTER : PROCESS %d FOUND VALUE\n---------------------\n", src);
				
				abort_or_not = 1;		//letting all other processes know that value is found time to abort
				i = 1;
                		//letting all OTHER processes know that value has been found
				printf("\nMASTER --> ABORTING ALL PROCESSES  ...\n");
				for(;i < nprocs; i++){
					if(i != src){   
						MPI_Recv(&return_var, 1, MPI_INT, i,from_worker, MPI_COMM_WORLD, &status);
						MPI_Send(&abort_or_not, 1, MPI_INT, i ,from_master, MPI_COMM_WORLD);
						MPI_Recv(&return_var, 1, MPI_INT, i,from_worker, MPI_COMM_WORLD, &status);
					}
				}
				break;
			}
			//****IF VALUE NOT FOUND*****
			if(return_var == 2){    
				MPI_Send(&abort_or_not, 1, MPI_INT, src ,from_master, MPI_COMM_WORLD);
				notfoundcount += 1;

                		//If none of the processes could find values then abort all programs 
				if(notfoundcount == nprocs-1){
					printf("<<<< ERROR :: VALUE WAS NOT FOUND >>>>\n");
					abort_or_not = 1;
					i = 1;
					printf("\nMASTER --> ABORTING ALL PROCESSES  ...\n");
					for(;i < nprocs; i++){
							MPI_Recv(&return_var, 1, MPI_INT, i,from_worker, MPI_COMM_WORLD, &status);
							MPI_Send(&abort_or_not, 1, MPI_INT, i ,from_master, MPI_COMM_WORLD);
							MPI_Recv(&return_var, 1, MPI_INT, i,from_worker, MPI_COMM_WORLD, &status);
					
					}
					break;
				}
			}
		}
		
		//free up allocated memory			
		free(arr);
		//end program
	    	printf("\n*****END PROGRAM*****\n");
	}

	//===========================================================================================
	//			 	WORKER PROCESSES
	//===========================================================================================	
	else if( myrank > 0){
		//GET DATA FROM MASTER NODE
		int sizeofarray = 0;
		MPI_Recv(&sizeofarray, 1, MPI_INT, 0,from_master, MPI_COMM_WORLD, &status);
		int* arr = (int*) calloc(sizeofarray, sizeof(int));		//dynamically allocate the array of specified size
		MPI_Recv(&arr[0], 1*sizeofarray, MPI_INT, 0,from_master, MPI_COMM_WORLD, &status);     
		int search_value;
		MPI_Recv(&search_value, 1, MPI_INT, 0,from_master, MPI_COMM_WORLD, &status);
		
		//print data that each worker process gets
		#pragma omp critical
		{
			printf("DATA RECIEVED BY PROCESS %d ---> ", myrank);
			int i = 0;
			for(; i < sizeofarray ; i++){
				printf("%d ", arr[i]);
			}
			printf("\n");
		}
		
		//Wait for all processes to begin task at the same time
		#pragma omp barrier				//to keep asking master whether it should abort or not
		{	
			int return_var;     			//used as signal; 0 - still looking, 2 - not found, 1 - found
			int abort_or_not = 0;			//get signal from master to continue or not			
			int i = 0;
			for(; i < sizeofarray ; i++){
				return_var = 0;
				if(arr[i] == search_value){ 	//if value is found
					return_var = 1;
				}
				#pragma omp critical
				{
					MPI_Send(&return_var, 1, MPI_INT, 0,from_worker, MPI_COMM_WORLD);
					MPI_Recv(&abort_or_not, 1, MPI_INT, 0,from_master, MPI_COMM_WORLD, &status);
				}
				if(return_var == 1 && abort_or_not == 2){	//master tells process to stop if itself found value
					printf("\n::PROCESS %d : I FOUND THE VALUE!\n", myrank);
					MPI_Send(&return_var, 1, MPI_INT, 0,from_worker, MPI_COMM_WORLD);	//for synchronisation	
					break;
				}
				if(abort_or_not == 1){  			//master sends signal to stop if some other process finds value
					printf("--> PROCESS %d ABORTING....\n", myrank);
					MPI_Send(&return_var, 1, MPI_INT, 0,from_worker, MPI_COMM_WORLD);	//for synchronisation	
					break;
				}
				
			}
	
			if(return_var == 0 && abort_or_not == 0)//if value still not found by process itself or anyother process
				return_var = 2;			//2 will signify that value was not found
			
			
			if(return_var == 2){   			//let master know you don't have value
				MPI_Send(&return_var, 1, MPI_INT, 0,from_worker, MPI_COMM_WORLD);
				MPI_Recv(&abort_or_not, 1, MPI_INT, 0,from_master, MPI_COMM_WORLD, &status);
                		//Even if process does not have value keep waiting for signal from master before aborting
				return_var = 0;
				while(abort_or_not != 1){
					MPI_Send(&return_var, 1, MPI_INT, 0,from_worker, MPI_COMM_WORLD);
					MPI_Recv(&abort_or_not, 1, MPI_INT, 0,from_master, MPI_COMM_WORLD, &status);
					if(abort_or_not == 1){
						printf("--> PROCESS %d ABORTING....\n", myrank);
					}
					MPI_Send(&return_var, 1, MPI_INT, 0,from_worker, MPI_COMM_WORLD);	//for synchronisation			
				}	
			}
		
		}
        	
		free(arr); //free allocated space
	}
	
	//Finish up the processes---------------------------------------------------
	MPI_Finalize();
    	return 0;
	
}
