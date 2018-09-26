libkmp.so  
&emsp;core module which protect other software.  
libtest.so  
&emsp;a test case which want to be protected.  
kmp  
&emsp;test client.  
kmp_server  
&emsp;test server.  

**basic steps**  
1. c -> s connect
2. c -> s @
3. s -> c 4B is_c  + 4B delta + seed
4. c -> s 4Bchecksum + fun_name + param + ...
5. c s&ensp; &ensp; call fun(param, ...)
6. c -> s

if report 'cannot open shared object file: No such file or directory' when run kmp_server or kmp,
you can add current path to search path:  
`export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:.`

**test separately ptp**  

imitate ptp:  
c -> s	t1 send()  t2 recv   	c:t1  			s:t2  
c <- s  t4 recv  t3 send(t2)  	c:t1,t2,t4 		s:t2,t3  
c -> s  send(t1,t4)				c:t1,t2,t4		s:t1,t2,t3,t4  
c <- s  send(t3)				c:t1,t2,t3,t4	s:t1,t2,t3,t4  
```
gcc -DPTP_SERVER ptp.cpp -o ptpd
gcc -DPTP_CLIENT ptp.cpp -o ptp
```