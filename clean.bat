@echo off

cd libisp
del /q *.user
rd /q /s debug
rd /q /s release
cd ..

cd repl
del /q repl.aps
del /q *.user
rd /q /s debug
rd /q /s release
cd ..

attrib -h libisp.suo
attrib -h ipch

del /q libisp.suo
del /q libisp.ncb
del /q libisp.sdf

rd /q /s debug
rd /q /s release
rd /q /s ipch
