#include <iostream>
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <string>
#include <stdlib.h> 
#include <queue>
#include <list>  
#include <semaphore.h>
using namespace std;

#define NUM_THREADS 5
#define MEMORY_SIZE 20

struct llNode //link list node with constructor, == operator id,size, index and copy constructor
{
	llNode(int id=-1,int size=0,int index=0):id(id),size(size),index(index){}
	llNode(const llNode &n){this->id=n.id; this->size=n.size; this->index=n.index;}
	const bool operator==(llNode l){return(this->id==l.id&&this->index==l.index&&this->size==l.size);}
	int id;
	int size;
	int index;
};

struct reqNode  //node for queue
{
	reqNode(int id=-1,int size=0):id(id),size(size){}
	int id;
	int size;
};


queue<reqNode> myqueue; // shared que
pthread_mutex_t sharedLock = PTHREAD_MUTEX_INITIALIZER; // mutex
pthread_t server; // server thread handle
sem_t semlist[NUM_THREADS]; // thread semaphores

list<llNode> ll; // thread memory information
char  memory[MEMORY_SIZE]; // memory size

bool working=true;

void delNode(llNode &n)
{
	for(list<llNode>::iterator i=ll.begin();i!=ll.end();i++) //iterate list
	{
		if(*i==n) //if searched node found delete it by setting its id to -1 if it's left and/or right are 
						//holes merge them
		{
			//printf("FREE MEM:%d %d %d",n.id,n.size,n.index); //
			i->id=-1;
			list<llNode>::iterator temp1=i;
			list<llNode>::iterator temp2=i;
			temp1++;
			if (temp1->id==-1)
			{
				
				i->size=i->size + temp1->size;
				ll.remove(*temp1);
			}

			temp2--;
		if (temp2->id==-1)
			{
				i->size=i->size + temp2->size;
				i->index=temp2->index;
				ll.remove(*temp2);
		    }
		}
	}  
}

void print_ll(list<llNode> ll) //print the linked list
{
	for(list<llNode>::iterator i=ll.begin();i!=ll.end();i++)
	{
		cout<<"["<<i->id<<"]"<<"["<<i->size<<"]"<<"["<<i->index<<"]";
		if(i==--ll.end())
			cout<<endl;	
		else
			cout<<"---";	
	}
}

void dump_memory()  
{
 // You need to print the whole memory array here.
	printf("List:\n");
	print_ll(ll);

	printf("Memory Dump:\n");
	for(int i=0;i<MEMORY_SIZE;i++)
	{
		printf("%c",memory[i]);	
	}
	cout<<endl;
	cout<<"*********************************"<<endl;
}
void use_mem() //use memory between 1 and 5 seconds
{
	sleep((rand()%5) +1);
}
void free_mem(llNode &l) //free memory after using it by using delNode() function
{
	pthread_mutex_lock( &sharedLock );
	delNode(l);
	for(int j=l.index;j<l.size+l.index;j++)
	{
		memory[j]='X';
	}
	pthread_mutex_unlock( &sharedLock );
}

void release_function() 
{
	//This function will be called
	//after the memory is no longer needed.
	//It will kill all the threads and deallocate the linked list.	
	//printf("Release function\n");
	pthread_mutex_lock( &sharedLock );
	working=false;
	ll.clear();
	

	for (int i = 0; i < MEMORY_SIZE; i++)	//return to init
  	{char x = 'X'; memory[i] = x;}
	pthread_mutex_unlock( &sharedLock );
	pthread_join(server,NULL);
}

bool my_malloc(llNode &l)
{
	//This function will add the struct to the queue
	bool reqGranted=false;
	reqNode n;
	n.id=l.id;
	n.size=l.size;
	pthread_mutex_lock( &sharedLock );
	myqueue.push(n);    //push the request
	pthread_mutex_unlock( &sharedLock );
	//printf("QUEUE PUSHED ID:%d SIZE:%d\n",n.id,n.size); //
	sem_wait(&semlist[l.id]);   //wait on its semaphore for server thread
	//check the ll
	pthread_mutex_lock( &sharedLock );
	for(list<llNode>::iterator i=ll.begin();i!=ll.end();i++)
	{
		if(i->id==n.id && i->size==n.size)    //check that if the request is granted
		{
			reqGranted=true;
			l.index=i->index;
			break;
		}
	}
	pthread_mutex_unlock( &sharedLock );
	return reqGranted;

		
}

void * server_function(void *)
{
	//This function should grant or decline a thread depending on memory size.
	while(working)
	{
		pthread_mutex_lock( &sharedLock );
		if(!myqueue.empty())   //if there is a request in the queue
		{
			
			reqNode n=myqueue.front();
                        //printf("Server: node:%d , %d\n",n.id,n.size); //
			
			//memory access algo
			bool memFound=false;
			for(list<llNode>::iterator i=ll.begin();i!=ll.end();i++)
			{
				//printf("Server LOOP ID:%d INDEX=%d SIZE=%d\n",i->id,i->index,i->size);
				if(i->id==-1 && i->size>=n.size) //memory bole found
				{
					//printf("Memory Hole Found\n");
					memFound=true;
					llNode toInsert(n.id,n.size,i->index);
					ll.insert(i,toInsert);  //insert it to the hole
					if(i->size!=n.size)   
					{
						i->index+=n.size;
						i->size-=n.size;
					}
					else    //if hole is fit exactly delete it completely
					{
						//cout<<"Equality Found:"<<i->id<<"\t"<<i->size<<"\t"<<i->index<<endl;
						ll.remove(*i);
					}
					
					for(int j=toInsert.index;j<n.size + toInsert.index;j++)  //assign to memory array
					{
						char c='0'+n.id;
						//cout<<"ASSIGN MEMORY:"<<c<<endl; //
						memory[j]=c;
					}
					break;
				}
			}
			if(memFound)  //if allocated dump mem
			{
				//printf("SERVER\n");
				dump_memory();
			}

			myqueue.pop();
			sem_post(&semlist[n.id]);	  //free the semaphore let thread continue its job
		}
		pthread_mutex_unlock( &sharedLock );		
	}	
}

void * thread_function(void * id) 
{
	while(working)
	{
		//This function will create a random size, and call my_malloc
		//Block
		//Then fill the memory with 1's or give an error prompt
		int* pid=(int*)id;
			//printf("thread %d"\n,*pid);  //
		int memSize=(rand() % (MEMORY_SIZE/2)) +1; //memsize generator
		//printf("Thread %d: %d\n",*pid,memSize); 
		llNode memNode(*pid,memSize,-1);
		if(my_malloc(memNode))  //if thread can be allocated
		{
			use_mem();   
			free_mem(memNode);
		}
		//printf("ORD ID:%d\n",*pid);
	}
	
}

void init()	 
{
	pthread_mutex_lock(&sharedLock);	//lock
	for(int i = 0; i < NUM_THREADS; i++) //initialize semaphores
	{sem_init(&semlist[i],0,0);}
	for (int i = 0; i < MEMORY_SIZE; i++)	//initialize memory 
  	{char x = 'X'; memory[i] = x;}
	
	ll.push_back(llNode(-1,MEMORY_SIZE,0));
	//print_ll(ll);
    pthread_create(&server,NULL,server_function,NULL); //start server 
	pthread_mutex_unlock(&sharedLock); //unlock
}





int main (int argc, char *argv[])
 {

    pthread_t threads[NUM_THREADS];
	int id[NUM_THREADS];
	for(int i=0;i<NUM_THREADS;i++)
	{
		id[i]=i;
	}
      
 	init();	// call init

 	//You need to create threads with using thread ID array, using pthread_create()
	for(int i=0;i<NUM_THREADS;i++)
	{
		//printf("index %d",i); //
		
		pthread_create(&threads[i],NULL,thread_function,(void *)&id[i]);
	}
	sleep(10);  //sleep 10 seconds

 	// join the threads
	release_function();
	for(int i=0;i<NUM_THREADS;i++)
	{
		pthread_join(threads[i],NULL);
		//printf("join %d",i);
	}

 	//dump_memory(); // this will print out the memory
 	printf("\nTerminating...\n");
 }
