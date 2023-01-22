** The functionality of freqalert is now included in the Motionplus application **

freqalert 0.1.0 - MrDave

freqalert is a very simple program to detect a particular frequency and if it is detected, execute a external command.  To make the program the following libraries are needed (based upon current apt repo).
 
sudo apt-get install libasound-dev libfftw3-dev

After the packages are installed, type

autoreconf -fiv
./configure
make
make install

