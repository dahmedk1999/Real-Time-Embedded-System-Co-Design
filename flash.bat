@echo off
cd "C:\Real-Time-Embedded-System-Co-Design\EmbeddedSystem"
set arg1=%1
python nxp-programmer/flash.py -i "_build_%1/%1.bin"