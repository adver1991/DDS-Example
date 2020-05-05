# DDS-Example
Make Sure OpenDDS env
https://opendds.org/quickstart/GettingStartedLinux.htm
## Ope
1. mpc.pl Messenger.mpc -type gnuace
2. make -f GNUmakefile.Messenger_Idl
3. make -f GNUmakefile.Messenger_Publisher
4. make -f GNUmakefile.Messenger_Subscriber
5. ./subscriber -DCPSConfigFile rtps.ini
6. ./publisher -DCPSConfigFile rtps.ini
