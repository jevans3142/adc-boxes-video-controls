# ADC Theatre - Control Boxes Video Switch Panels
## Configuration file format

An example configuration file for the SD card is included in this folder.
Any line beginning with // is regarded as a comment. 
The variable names must not be changed otherwise they will not be recognised. The equals sign also must be present. 

### Routing panel sources/destinations
Controls which source is routed to destination for each button and which output way on the router is used.
Allowed values for sources: 1-40 = sources 1-40 on router
Allowed values for destinations: 1-40 = destinations on router

| Variable name  | Format |
| ------------- | ------------- |
| routing_sources | Comma seperated list of numbers |
| routing_destination  | Single number  |


### Router properties

| Variable name  | Format |
| ------------- | ------------- |
| router_ip | IP address in x.x.x.x format, no quotes |
| router_port  | Single number  |