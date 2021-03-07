# Simple http server

## TODO
- program does not close (client?) socket on exit. Thus new http_server is
unable to bind fd.
- pthreads implementation

## References
- IBM example: https://www.ibm.com/developerworks/systems/library/es-nweb/
- sockets: https://en.wikipedia.org/wiki/Berkeley_sockets
