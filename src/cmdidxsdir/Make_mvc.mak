
cmdidxs: cmdidxs.exe

cmdidxs.exe: cmdidxs.c ..\ex_cmds.h
	cl /nologo cmdidxs.c
	cmdidxs.exe

clean:
	- if exist cmdidxs.obj del cmdidxs.obj
	- if exist cmdidxs.exe del cmdidxs.exe
