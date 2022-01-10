all: tcpclient async-tcpserver 

tcpclient: tcpclient.c
	gcc -o tcpclient tcpclient.c

# timeclient: timetcpclient.c
# 	gcc -o timeclient timetcpclient.c
	
async-tcpserver: async-tcpserver.c
	gcc -o async-tcpserver async-tcpserver.c
	
clean: 
	rm -f *.o tcpclient async-tcpserver timeclient