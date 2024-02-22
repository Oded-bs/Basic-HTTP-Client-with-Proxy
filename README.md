# Basic HTTP Client

HTTP Client, execute HTTP GET requests.
The response is printed to the Terminal. In addition, if the response status is "200 OK" the program will create a new directory with the host name and will save the reponse into
the compatible file format. If the flag -s is inserted as argument the file will be opened in Firefox.

# Usage
To compile: gcc -Wall -o cproxy cproxy.c
How to run: ./cproxy <URL> [-s]
Constrains: URL must start with "http://"
