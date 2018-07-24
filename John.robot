*** Settings ***
Documentation          This example demonstrates executing commands on a remote machine
...                    and getting their output and the return code.
...                    http://robotframework.org/SSHLibrary/SSHLibrary.html#Read
...                    Notice how connections are handled as part of the suite setup and
...                    teardown. This saves some time when executing several test cases.

Library                SSHLibrary	Dialogs
Suite Setup            Open Connection And Log In
Suite Teardown         Shutdown Application And Close All Connections

*** Variables ***


${HOST}                localhost
${SER_PORT}            4000
${VIR_PORT}            6969
${IP_ADD}              172.17.0.6
Create List			

*** Test Cases ***

Start Server
  [Documentation]		...
  Run Keyword     		Compile
  Write					./run_server ${VIR_PORT}
  ${output}=			Read Until			...
  Should End With		${output}			Starting server...

First Time Login Wrong Password Input
  [Documentation]		...
  Switch Connection 	Client_1
  Write					./run_client ${IP_ADD} ${VIR_PORT}
  ${output}				Read Until			
  ${output}=			Read Until			password:
  Should End With		${output}			Please enter new password:
  Write					waffles
  ${output}=			Read Until			password:
  Should End With		${output}			Please confirm your password:
  Write					pancakes
  ${output}=			Read Until			again
  Should End With		${output}			please try again.

First Time Login Success
  Run Keyword 			First Time Login

Single Client Write And Server Read
  Write					Hello World!
  Write					I am human
  Write					Not a robot test!
  Write					Just joking, i'm a robot test writen by Kenrick.
  Write 				Cya ;)
  Switch Connection     Server
  ${output}=			Read Until 			human
  Should End With    	${output}         	Here is the message: Hello World!\n Here is the message: I am human
  ${output}=			Read Until			test!
  Should End With		${output}			Here is the message: Not a robot test!
  ${output}=			Read Until			;)
  Should End With		${output}			Here is the message: Cya ;)

Client Logoff and Login (Proper)
  Switch Connection     Client_1
  Write 				/exit
  Write 				./run_client ${IP_ADD} ${VIR_PORT}
  Read Until			enter username:
  Write 				John
  Read Until			Please enter password:
  Write 				waffles
  Switch Connection 	Server
  ${output}=        	Read Until         New client joined
  Should End With   	${output}          New client joined
  Switch Connection 	Client_1
  Write 				Hi
  Switch Connection     Server
  ${output}=        	Read Until         Hi
  Should End With   	${output}          Here is the message: Hi




Client Join Till Server Capacity
  Switch Connection  Client_2
  Write              ./run_client ${IP_ADD} ${VIR_PORT}
  Switch Connection  Server
  ${output}=         Read Until         joined
  Should End With    ${output}          New client joined
  Switch Connection  Client_3
    Write              ./run_client ${IP_ADD} ${VIR_PORT} 
    Switch Connection  Server
    ${output}=         Read Until         joined
    Should End With    ${output}          New client joined


*** Keywords ***
Open Connection And Login
  [Documentation]		...
  Open Connection     	${HOST}            	port=${SER_PORT}   	alias=Server
  ${USERNAME}=		  	Get Value From User 	Input username		default
  ${PASSWORD}=		  	Get Value From User 	Input password 		hidden=yes
  Login               	${USERNAME}       	${PASSWORD}
  Write               	cd /git
  Open Connection     	${HOST}           	port=3000 alias=Client_1
  Login              	${USERNAME}       	${PASSWORD}
  Write               	cd /git
  Open Connection 	  	${HOST}  		       	port=6000			alias=Client_2

Compile
  [Documentation]		...
  Switch Connection Server
  Write			 	mv -f run_server run_server_backup
  Write				mv -f run_server run_server_backup
  Write             g++ -std=c++11 -pthread server.cpp run_server
  Write 			g++ -std=c++11 -pthread client.cpp run_client
  File Should Exist     run_server
  File Should Exist 	run_client

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








