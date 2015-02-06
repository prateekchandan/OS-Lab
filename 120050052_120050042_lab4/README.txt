To run the code, follow these steps:
-> Put the input files namely PROCESS_SPEC and SCHEDULER_SPEC in the submission directory
-> compile using: g++ scheduler_simulator.cpp
-> run it using ./a.out

(Sample Input files have been provided in the submission directory which correspond to testcase 6 of the 'testcases' directory)

////////////////////////////////////////////////////////////////////////

The following are the events that we have used in the event handler:

1. Process admission
2. End of time-slice
3. IO start
4. IO end

And all the messages that were to be printed as per the problem statement have been printed. The following are the ones:

1. Process Admitted
2. CPU started
3. CPU burst completed
4. Time slice ended
5. IO started
6. IO burst completed
7. Process pre-empted
8. Process promoted to level <N>
9. Process demoted to level <N>
10. Process terminated


