PLEASE REFER TO THE APPLICATION NOTE FOR THIS MIDDLEWARE FOR MORE INFORMATION

r_t4_driver_rx
=========

Document Number
---------------
R20AN0311JJ0102_RX_T4
R20AN0311EJ0102_RX_T4

Version
-------
v1.02

Overview
--------
Convert the TCP/IP(T4) - RX Ether Driver Interface

Features
--------
* Convert the TCP/IP(T4) - RX Ether Driver Interface

Supported MCUs
--------------
* RX64M Group
* RX71M Group

Boards Tested On
----------------
* RSKRX64M
* RSKRX71M

Limitations
-----------
None

Peripherals Used Directly
-------------------------
None

Required Packages
-----------------
None

How to add to your project
--------------------------
Please refer to the Adding Firmware Integration Technology Modules to Projects.
"r01an1723ju0111_rx.pdf/r01an1826jj0102_rx.pdf(R20AN0311JJ0101_RX_T4)"
"r01an1723eu0111_rx.pdf/r01an1826ej0102_rx.pdf(R20AN0311EJ0101_RX_T4)".

Toolchain(s) Used
-----------------
* Renesas RX Compiler V.1.02 R01+

File Structure
--------------
r_t4_driver_rx
|   readme.txt
|
+---doc
|   |
|   +---ja
|   |     r20an0311jj0102_rx_t4.pdf
|   +---en
|         r20an0311ej0102_rx_t4.pdf
|
+---ref
|       r_t4_driver_rx_config_reference.h
|
+---src
|       ether_callback.c
|       t4_driver.c
|       timer.c
|       timer.h
|
r_config
        r_t4_driver_rx_config.h
