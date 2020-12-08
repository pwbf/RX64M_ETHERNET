PLEASE REFER TO THE APPLICATION NOTE FOR THIS MIDDLEWARE FOR MORE INFORMATION

r_t4_ftp_server_rx
==================

Document Number
---------------
R20AN0078JJ0103_RX_T4
R20AN0078EJ0103_RX_T4

Version
-------
v1.03

Overview
--------
T4 FTP server provides simple FTP server function.

 http://japan.renesas.com/mw/t4 (Japanese)
 http://www.renesas.com/mw/t4 (English)

These configuration options can be found in "r_config\r_t4_ftp_server_rx_config.h". 
An original copy of the configuration file is stored in 
"r_t4_ftp_server_rx\ref\r_t4_ftp_server_rx_config_reference.h".

Features
--------
* Simple FTP server function

Supported MCUs
--------------
* RX62N Group
* RX63N Group
* RX64M Group
* RX71M Group

Boards Tested On
----------------
* RSKRX62N
* RSKRX63N
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
"r01an1723ju0111_rx.pdf/r01an1826jj0102_rx.pdf(R20AN0078JJ0103_RX_T4)"
"r01an1723eu0111_rx.pdf/r01an1826ej0102_rx.pdf(R20AN0078EJ0103_RX_T4)".

Toolchain(s) Used
-----------------
* Renesas RXC Toolchain v2.01.00+

File Structure
--------------
r_t4_ftp_server_rx
|   readme.txt
|   r_t4_ftp_server_rx_if.h
|
+---doc
|   |
|   +---ja
|   |        r20an0078jj0103_rx_t4.pdf
|   +---en
|            r20an0078ej0103_rx_t4.pdf
|
+---ref
|       r_t4_ftp_server_rx_config_reference.h
|
+---src
|       r_ftp_server.c
|
r_config
    r_t4_ftp_server_rx_config.h
