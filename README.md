1. mpc.pl RobotControl.mpc -type gnuace
2. make -f GNUmakefile.RobotControl_Idl
3. make -f GNUmakefile.RobotControl_Service
4. make -f GNUmakefile.RobotControl_Client
5. ./Service -DCPSConfigFile rtps.ini
6. ./client -DCPSConfigFile rtps.ini