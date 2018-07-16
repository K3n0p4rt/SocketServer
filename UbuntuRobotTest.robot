*** Settings ***
Documentation          This example demonstrates executing commands on a remote machine
...                    and getting their output and the return code.
...                    http://robotframework.org/SSHLibrary/SSHLibrary.html#Read
...                    Notice how connections are handled as part of the suite setup and
...                    teardown. This saves some time when executing several test cases.

Library                SSHLibrary
Suite Setup            Open Connection And Log In
Suite Teardown         Close All Connections

*** Test Cases ***
Execute Command And Verify Output
    [Documentation]    Execute Command can be used to ran commands on the remote machine.
    ...                The keyword returns the standard output by default.
    ${output}=         Execute Command  echo Hello SSHLibrary!
    Should Be Equal    ${output}        Hello SSHLibrary!

Execute Command And Verify Return Code
    [Documentation]    Often getting the return code of the command is enough.
    ...                This behaviour can be adjusted as Execute Command arguments.
    ${rc}=             Execute Command  echo Success guaranteed.    return_stdout=False    return_rc=True
    Should Be Equal    ${rc}            ${0}

Executing Commands In An Interactive Session
    [Documentation]    Execute Command always executes the command in a new shell.
    ...                This means that changes to the environment are not persisted
    ...                between subsequent Execute Command keyword calls.
    ...                Write and Read Until variants can be used to operate in the same shell.
    Write              cd ..
    Write              echo Hello from the parent directory!
    ${output}=         Read Until       directory!
    Should End With    ${output}        Hello from the parent directory!

Accessing C++ TestApp And Testing Square Fuctionality
    [Documentation]    
    Write              cd /git/TestApp/dist/Debug/GNU-Linux
    Write              ./testapp
    Write              5                
    ${output}=         Read             delay=0.5s
    Should Contain     ${output}        25

*** Keywords ***
Open Connection And Log In
   Open Connection     localhost    port=3000
   Login               root        screencast