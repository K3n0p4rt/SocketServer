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

${HOST}                localhost
${USERNAME}            root
${PASSWORD}            screencast
${SER_PORT}            4000
@{CLI_PORTS}           Create List        3000  5000  7000  8000
${VIR_PORT}            9991
${IP_ADD}              172.17.0.6

*** Test Cases ***
Execute Command And Verify Output
    [Documentation]    Execute Command can be used to run commands on the remote machine.
    ...                The keyword returns the standard output by default.
    ${output}=         Execute Command    echo Hello SSHLibrary!
    Should Be Equal    ${output}          Hello SSHLibrary!

Start Server
    [Documentation]    ...
    Switch Connection  Server
    ${IP_ADD}=          Execute Command    hostname -I             
    Write              ./run ${VIR_PORT}
    ${output}=         Read Until         ...
    Should End With    ${output}          Starting server...

Join With One Client
    [Documentation]    ...
    Switch Connection  Client_1
    Write              ./run ${IP_ADD} ${VIR_PORT} 
    ${output}=         Read Until         name:
    Should End With    ${output}          Please enter your name:
    Write              Kenrick
    Switch Connection  Server
    ${output}=         Read Until         joined
    Should End With    ${output}          New client joined

Client Join Till Server Capacity 
    [Documentation]    ...
    Switch Connection  Client_2
    Write              ./run ${IP_ADD} ${VIR_PORT}
    ${output}=         Read Until         name:
    Should End With    ${output}          Please enter your name:
    Write              Konrac
    Switch Connection  Server
    ${output}=         Read Until         joined
    Should End With    ${output}          New client joined
    Switch Connection  Client_3
    Write              ./run ${IP_ADD} ${VIR_PORT} 
    ${output}=         Read Until         name:
    Should End With    ${output}          Please enter your name:
    Write              That One
    Switch Connection  Server
    ${output}=         Read Until         joined
    Should End With    ${output}          New client joined

Client Join At Max Server Capacity
    [Documentation]    ...
    Switch Connection  Client_4
    Write              ./run ${IP_ADD} ${VIR_PORT}     
    ${output}=         Read Until         later
    Should End With    ${output}          Server full. Please try agian later
    Execute Command    pkill -15 run


Client Leave and Join
    [Documentation]    ...
    Switch Connection  Client_3
    Execute Command    pkill -15 run  
    Write              ./run ${IP_ADD} ${VIR_PORT}
    Switch Connection  Server
    ${output}=         Read Until         joined
    Should End With    ${output}          New client joined

Send And Recieve Message (Client_3)
    [Documentation]
    Switch Connection  Client_3
    Write              howdy!
    Switch Connection  Server
    ${output}=         Read Until         !
    Should End With    ${output}          howdy!
    Switch Connection  Client_2
    ${output}=         Read Until         !
    Should End With    ${output}          howdy!
    Switch Connection  Client_1
    ${output}=         Read Until         !
    Should End With    ${output}          howdy!

Send And Recieve Message (Client_1)
    Switch Connection  Client_1
    Write              I am here
    Switch Connection  Client_3
    ${output}=         Read Until         here
    Should End With    ${output}          I am here
    Switch Connection  Client_2
    ${output}=         Read Until         here
    Should End With    ${output}          I am here

Send And Recieve Message (Client_2)
    Switch Connection  Client_2
    Write              cry ;(
    Switch Connection  Client_1
    ${output}=         Read Until         ;(
    Should End With    ${output}          cry ;(
    Switch Connection  Client_3
    ${output}=         Read Until         ;(
    Should End With    ${output}          cry ;(



*** Keywords ***
Open Connection And Login 
   Open Connection     ${HOST}            port=${SER_PORT}   alias=Server  
   Login               ${USERNAME}        ${PASSWORD}
   Write               cd /git
   Write               g++ std=c++11 -pthread server.cpp -o run 
   Open Connection     ${HOST}            port=3000          alias=Client_1
   Login               ${USERNAME}        ${PASSWORD}
   Write               cd /git
   Write               g++ std=c++11 -pthread client.cpp -o run 
   Open Connection     ${HOST}            port=5000          alias=Client_2
   Login               ${USERNAME}        ${PASSWORD}
   Write               cd /git
   Open Connection     ${HOST}            port=7000          alias=Client_3
   Login               ${USERNAME}        ${PASSWORD}
   Write               cd /git
   Open Connection     ${HOST}            port=8000          alias=Client_4
   Login               ${USERNAME}        ${PASSWORD}
   Write               cd /git

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








