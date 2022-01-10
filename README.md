# Async Server Socket

Wrote a single-threaded server that can monitor multiple sockets for new connections using an event-driven approach, and perform the same 3-way handshake with many concurrent clients. Used the `select()` function to monitor multiple
sockets and an array to maintain state information for different clients.

To compile the code,
```bash
make
```

Run the server using,
```bash
./async-tcpserver <port no>
```
Execute the following command in another terminal(same directory)
```bash
python3 concurrent-requests.py <port no>
```