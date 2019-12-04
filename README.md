# Mem_Alloc_Sim
This is a simulation on console demonstrating memory allocation system in a Simple Operating System it will create an memory array, number
of threads and a server thread. For 10 seconds till program termination ordinary threads will query for memory allocation from memory array
for a random ammount of bytes. Server thread using first fit algorithm, find a fit for the memory query. If found, ordinary thread will 
allocate it, if not, server will reject the query.
