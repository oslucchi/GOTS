GOTS - Good Old Times Spooler

GOTS is a spooler derived from the GCOS 4 spooling server
It allows to configure printers with the ability of mounting different 
paper types.
Reports are queued up via spooling directory and UNIX signals to wake up
the spooler process. 
Once a printer has a report queued up and the related printer is ready
and mounting the required paper, the report is spooled to the printer
If the printer is mounting a different paper then a mountpaper request 
is sent to the host configured to be the printer master (it doesn't have
to be physically connected to the printer, it just need to be configured
as responsible for it)
Once the master confirms the paper is mounted, the report is spooled to
the printer.

A server process is also provided to the clients in order for them to connect
and add on the printers configured
