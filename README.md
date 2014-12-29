Author
======
Sowmiya Narayan Srinath <br>
September 2014

TCP socket client / server application using I/O multiplexing, child processes and threads similiar to the
inetd superserver daemon. <br>
The server is multi threaded and the client spawns a new process for every request made. <br>
The application is built to be robust against various forms of crashes, invalid user inputs etc.
