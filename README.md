In-Memory Redis-like Database | C++, Networking, Concurrent Data Structures 

* Developed a Redis-like high-performance key-value store built from scratch using +1500 lines of C++ code.
* Crafted an asynchronous networking layer with ASIO for high-performance, non-blocking I/O.
* Built a master-slave replication system using sync protocols, ensuring fault tolerance across distributed nodes.
* Designed a persistence mechanism to save in-memory data to disk, balancing performance and data durability.
* Leveraged smart pointers for resource management, preventing memory leaks in a multi-threaded environment.
* Created a stream data type to enable sequential data storage and retrieval for real-time applications.
* Built an atomic transaction engine with a transaction manager for queuing and executing commands as a single
unit, including rollback capabilities
