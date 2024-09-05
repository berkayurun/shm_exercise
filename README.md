## General Description
This project demonstrates shared memory communication between a multithreaded server and possibly multiple clients.

There are two client implementations. The standard client sends random queries to the server in an infinite loop. On the other hand, the single request client takes command line parameters, sends a single query, and exits.

This second client can be controlled by using the following flags:
```
Usage:
-i [key] [value]         Insert
-r [key]                 Remove
-s [key]                 Search
-p                       Print table
-b [index]               Print bucket by index
```

## Scalability Analysis
There are two possible bottlenecks in the system. Concurrency on the hash table and the single shared memory between the server and multiple clients.

### Hash Table
In the current implementation, each bucket has a mutex. This granularity is fine as long as the hash table is relatively big and the hash function is successful. Nevertheless, when this is not the case, many keys will be placed in a single bucket. This is an issue because a single mutex protects the bucket, and many threads are trying to access it. As a result, threads wait for each other. Combining this with the overhead multithreading brings, it is possible that a single thread execution is faster.

A more coarse approach, such as locking the whole hash table, would lead to worse multithreading results.

In the end, scalability is connected to the hash table's size, the hash function's quality, and how the collision is handled.

### Shared Memory
The shared memory is being used by the server and the client(s). In the current version, both sides lock the shared memory before accessing it. This is not ideal since there is only one communication resource, and when it is locked, everyone else must wait. Especially when there are massive amounts of clients writing and multiple server threads reading, shared memory becomes the bottleneck, and the execution becomes sequential.

As an improvement, we can eliminate the server-side lock for the shared memory since the server only reads and gets notified when there is a new query. Also, data will not be overridden after it is written. To implement this, `consumer_index`, which shows the next reading location, must be tracked in the server and not in the shared memory.

To achieve a overall better scalability, the most naive solution would be dividing shared memory into blocks and only locking the accessed block. In this case, we would not have the queries ordered by their creation time. So, one major drawback would be managing the order of the queries, especially insert and delete, since they change the table. All other operations like search (find) and print can be executed out-of-order in between these Remove/Insert ops. However, this brings an important overhead, especially if Remove/Insert queries are frequently expected.

The processing order of queries seems to be the biggest problem for scalability.

## References
- https://www.softprayog.in/programming/interprocess-communication-using-posix-shared-memory-in-linux
- man pages