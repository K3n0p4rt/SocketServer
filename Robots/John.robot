*** Settings ***
Documentation          This example demonstrates executing commands on a remote machine
...                    and getting their output and the return code.
...                    http://robotframework.org/SSHLibrary/SSHLibrary.html#Read
...                    Notice how connections are handled as part of the suite setup and
...                    teardown. This saves some time when executing several test cases.

Library                SSHLibrary
Suite Setup            Open Connection And Log In
Suite Teardown         Shutdown Application And Close All Connections

*** Variables ***

${USERNAME}            root
${PASSWORD}            screencast
${HOST}                localhost
${SER_PORT}            4000
${VIR_PORT}            6969
${IP_ADD}              172.17.0.5

*** Test Cases ***

Start Server
  [Documentation]		...
  Run Keyword     		Compile
  Write					      ./run_server ${VIR_PORT}
  ${output}=			    Read Until			...
  Should End With		  ${output}			  Starting server...

First Time Login Wrong Password Input
  [Documentation]		...
  Switch Connection 	Client_1
  Write					  ./run_client ${IP_ADD} ${VIR_PORT}
  Read Until			enter username:
  Write           John
  ${output}=			Read Until			password:
  Should End With		${output}			Please enter new password:
  Write					waffles
  ${output}=			Read Until			password:
  Should End With		${output}			Please confirm your password:
  Write					pancakes
  ${output}=			Read Until			again.
  Should End With		${output}			please try again.

First Time Login Success
  Run Keyword 			   First Time Login

Single Client Write And Server Read
  Write					      Hello World!
  Switch Connection   Server
  ${output}=			    Read Until 			!
  Should End With    	${output}       Here is the message: Hello World!

Client Logoff and Login (Proper)
  Switch Connection     Client_1
  Write 				      /exit
  Write 				      ./run_client ${IP_ADD} ${VIR_PORT}
  Read Until			    enter username:
  Write 				      John
  Read Until			    Please enter password:
  Write 				      waffles
  Switch Connection 	Server
  ${output}=        	Read Until         New client joined
  Should End With   	${output}          New client joined
  Switch Connection 	Client_1
  Write 				      A
  Switch Connection   Server
  ${output}=        	Read Until         A
  Should End With   	${output}          Here is the message: A

Client Join Till Server Capacity
  Switch Connection   Client_2
  Write               ./run_client ${IP_ADD} ${VIR_PORT}
  Read Until          enter username:
  Write               Jess
  Run Keyword         First Time Login
  Switch Connection   Client_3
  Write               ./run_client ${IP_ADD} ${VIR_PORT}
  Read Until          enter username:
  Write               Jan
  Run Keyword         First Time Login

Client Join At Max Server Capacity
  Switch Connection  Client_4
  Write              ./run_client ${IP_ADD} ${VIR_PORT}     
  ${output}=         Read Until         later
  Should End With    ${output}          Server full. Please try agian later
  Execute Command    pkill -15

Send And Recieve Message
  Switch Connection  Client_3
  Write              howdy!
  Switch Connection  Client_2
  ${output}=         Read Until         dy!
  Should End With    ${output}          howdy!
  Switch Connection  Client_1
  ${output}=         Read Until         dy!
  Should End With    ${output}          howdy!

  Switch Connection  Client_1
  Write              I am here
  Switch Connection  Client_3
  ${output}=         Read Until         here
  Should End With    ${output}          I am here
  Switch Connection  Client_2
  ${output}=         Read Until         here
  Should End With    ${output}          I am here

Using Online And All_Users Commands
  Switch Connection   Client_1
  Write               /online
  ${output}           Read                delay=0.2s
  Should Contain      ${output}           John
  Should Contain      ${output}           Jan
  Should Contain      ${output}           Jess
  Write               /all_users
  ${output}           Read                delay=0.2s
  Should Contain      ${output}           John
  Should Contain      ${output}           Jan
  Should Contain      ${output}           Jess
  Write               /exit
  Switch Connection   Client_2
  Write               /online
  ${output}           Read                delay=0.2s
  Should Not Contain  ${output}           John
  Switch Connection   Client_1
  Write               ./run_client ${IP_ADD} ${VIR_PORT} 
  Write               John
  Write               waffles

Using Add_Friend and Freind Commands
  Switch Connection   Client_1
  Write               /friends
  ${output}           Read                 delay=0.2s
  Should Not Contain  ${output}           John
  Should Not Contain  ${output}           Jan
  Should Not ontain   ${output}           Jess
  Write               /add_friend Jess


*** Keywords ***
Open Connection And Login
  [Documentation]		...
  Open Connection     	${HOST}            	port=${SER_PORT}   	alias=Server
  Login               ${USERNAME}       	${PASSWORD}
  Write               	cd /git
  Open Connection     	${HOST}           	port=2000     alias=Client_1
  Login              	${USERNAME}       	${PASSWORD}
  Write               	cd /git
  Open Connection       ${HOST}             port=3000     alias=Client_2
  Login               ${USERNAME}         ${PASSWORD}
   Write                cd /git
  Open Connection       ${HOST}             port=5000     alias=Client_3
  Login               ${USERNAME}         ${PASSWORD}
   Write                cd /git
  Open Connection       ${HOST}             port=6000     alias=Client_4
  Login               ${USERNAME}         ${PASSWORD}
   Write                cd /git

Compile
  [Documentation]		...
  Switch Connection     Server
  Write			 	          mv -f run_server run_server_backup
  Write				          mv -f run_client run_client_backup
  Write                 g++ -std=c++11 -pthread server.cpp -o run_server
  Write 			          g++ -std=c++11 -pthread client.cpp -o run_client


First Time Login
  Read Until			new password:
  Write					waffles
  Read Until			your password:
  Write					waffles
  Read Until			confirmed!
  Read Until			enter password:
  Write					waffles
  Read Until			chatroom!
  

Shutdown Application And Close All Connections
   Switch Connection   Client_4
   Execute Command     pkill -15 run          

   Switch Connection   Client_3
   Execute Command     pkill -15 run  
  
   Switch Connection   Client_2
   Execute Command     pkill -15 run  
  
   Switch Connection   Client_1
   Execute Command     pkill -15 run  
  
   Switch Connection   Server
   Execute Command     pkill -15 run  

   Close All Connections







