#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>     // for read/write/close
#include <sys/types.h>  /* standard system types        */
#include <netinet/in.h> /* Internet address structures */
#include <sys/socket.h> /* socket interface functions  */
#include <netdb.h>      /* host to IP resolution            */
#include <sys/stat.h>
#include <errno.h>

#define BUFLEN 2048
#define ATTRIBUTES 5
#define GETREQ 26
#define FAILED -1
#define DEFAULTVALUE 0
#define CALLOC "Calloc"

/**
 * gets: 2d array ** , and release all the inner cells first if they are initialized and last frees the 2d array pointer
*/
void free2DMem(char *requestHolder[])
{
    for(int i = 0; i < ATTRIBUTES ; i++)
    {
        if(requestHolder[i] != NULL)
        {
            free(requestHolder[i]);
        }
    }
    free(requestHolder);
}

/**1
 * gets: pointer we want to free, name of the system call that failed.
*/
void perrorFree(char* toFree, char* Syscall)
{
    free(toFree);
    perror(Syscall);
    exit(EXIT_FAILURE);

}

/**
 *Gets: int that marks if theres a -s flag, argc , argv[], Returns: incase of invalid input the function retrurns -1 else returns 0.  
 * invalid input count as: flag which is not -s , no http:// at the start of the host name , less than 2 or greater than 3 arguments.
*/
int InputIsValid(int *flag,int argc,char *argv[])
{
    if(argc < 2 || argc >3)
    {
        return FAILED;
    }
    if(argc == 3)
    {
        if(!(strstr(argv[2],"-s\0")) || strlen(argv[2]) > 2)
        {
            return FAILED;
        }
        *flag = 1; 
    }
    if(!strstr(argv[1],"http://"))
    {
        return FAILED;
    }

    return 1;
}

/**
 * function that receive the URL and seperate it to strings in the following order: httpRequest[x] : 0 = hostname,1 = port, 2 = path.
*/
void ParseURL(char *argv[],char *httpRequest[])
{
    char *check_chr,*port_chr,*host,*port,*path; /**Init of variables*/
    int hostLen = 1, pathLen = 1, portLen = 3, urlSize = strlen(argv[1]), http = 7;
    check_chr = strchr(&argv[1][http],'/');
    if(check_chr == NULL) // if we dont find '/' in the url after the host name we set up the sizes of the path and the host name
    {
        hostLen += urlSize - http;
        pathLen++;
    }else
    {
        hostLen += (int)(check_chr - &argv[1][http]);
        pathLen += urlSize - (hostLen+http)+1;
    }

    host = (char*)calloc(hostLen,sizeof(char)); // first we start with seperating the host from the url. (includes the port)
    if(!host)
    {
        free2DMem(httpRequest);
        perrorFree(host,CALLOC);
        
    }
    strncpy(host,&argv[1][http],hostLen-1);
    strcat(host,"\0");
    path = (char*)calloc(pathLen,sizeof(char)); // 2nd we go and seperate path. if there isnt path specified we add '/' and this will be our path.
    if(!path)
    {
        free(host);
        free2DMem(httpRequest);
        perrorFree(path,CALLOC);
    }
    if(pathLen == 2)
    {
        strcat(path,"/");
    }else
    {
        strncpy(path,&argv[1][hostLen+http-1],pathLen);
    }
    strcat(path,"\0");
    httpRequest[2]= path;
    port_chr = strchr(host,':'); // if we found that the user inserted port, we sepereate the port from the host name.
    if(port_chr!=NULL)
    {
        portLen = (int)(&host[strlen(host)] - port_chr)+1;
        hostLen = (int)(port_chr - &host[0])+1;
        char* hostName = (char*)calloc(hostLen+1,sizeof(char));
        if(!hostName)
        {
            free(host);
            free2DMem(httpRequest);
            perrorFree(hostName,CALLOC);
        }
        strncpy(hostName,&host[0],hostLen-1);
        strcat(hostName,"\0");
        port = (char*)calloc(portLen,sizeof(char));
        if(!port)
        {
            free(host);
            free2DMem(httpRequest);
            perrorFree(port,CALLOC);
        }
        strncpy(port,&host[hostLen],portLen);
        strcat(port,"\0");
        httpRequest[0] = hostName;
        httpRequest[1] = port;
        free(host);
    }
    else // if there isnt any port specified the default value will be port 80.
    {
        strcat(host,"\0");
        httpRequest[0]= host;
        port = (char*)calloc(portLen,sizeof(char));
        if(!port)
        {
            free2DMem(httpRequest);
            perrorFree(port,CALLOC);
        }
        strcpy(port,"80\0");
        httpRequest[1]=port;
    }
    return;
}
/**gets: 2d array of strings (char*) and format is to get request in the following format:
 * "GET <Path> HTTP/1.0\r\nHost: <URL>\r\n\r\n" */
char* makeGetRequest(char *httpRequest[])
{
    char *getReq;
    int hostName = strlen(httpRequest[0]);
    int path = strlen(httpRequest[2]);
    int sum = hostName + path;
    getReq = (char*)calloc(sum+GETREQ,sizeof(char));
    if(!getReq)
    {
        free2DMem(httpRequest);
        perrorFree(getReq,CALLOC);
    }
    strcat(getReq, "GET ");
    strcat(getReq, httpRequest[2]);
    strcat(getReq, " HTTP/1.0\r\nHost: ");
    strcat(getReq, httpRequest[0]);
    strcat(getReq, "\r\n\r\n");
    strcat(getReq, "\0");
    
    return getReq;
}
/**
 * recieves the 2d array that stores all the URL info and creates a long path in the following format: <hostname>/<path>
*/
char* FilePath(char *httpRequest[])
{
    char* path;
    int fullPathLen = strlen(httpRequest[0]) + strlen(httpRequest[2]) + strlen(httpRequest[1]) + 2; // we calculate whats the length needed for hold host+path.
    path = (char*)calloc(fullPathLen,sizeof(char));
    if(!path)
    {
        free2DMem(httpRequest);
        perrorFree(path,CALLOC);
    }
    strcat(path,httpRequest[0]);
    strcat(path,httpRequest[2]);
    char* check = strrchr(path,'/'); // we check if the last char is '/' if yes, we will strcat index.html to that path
    int sum = (int)(check - &path[strlen(path)-1]);
    if(sum==0)
    {
        path = (char*)realloc(path,fullPathLen+11);
        if(!path)
        {
            free2DMem(httpRequest);
            perrorFree(path,CALLOC);
        }
        strcat(path,"index.html");
    }
    else // if '/' aint the last character, we check if theres a "." that marks the suffix of that file. if it doesnt exist we add ".txt" by default.
    {
        char* txtend = strchr(check,'.');
        if(txtend == NULL)
        {
            path = (char*)realloc(path,fullPathLen+4);
            strcat(path,".txt");
        }
    }
    strcat(path,"\0");
    return path;
}

/**
 * function that recieve a path and creates directories + file and returns *fp of the file it created. we divide the full path with strtok and were using "/" as a divider and then that function start creating the path.
*/
FILE* CreateFileDir(char* fullPath)
{
    FILE *fp;
    int dirLen = 0;
    char *chr = strrchr(fullPath,'/');
    dirLen = (int)(chr - &fullPath[0])+1;
    char* dirPath = (char*)calloc(dirLen,sizeof(char));
    if(!dirPath)
    {
        free(dirPath);
        perror(CALLOC);
    }
    strncpy(dirPath,fullPath,dirLen-1);
    strcat(dirPath,"\0");
    char* pathToCreate = (char*)calloc(strlen(fullPath),sizeof(char));
   // Extract the first token
    char * token = strtok(dirPath, "/");
   // loop through the string to extract all other tokens
    while( token != NULL ) {
        strcat(pathToCreate,token);
        mkdir(pathToCreate,0777); // for each new token we get, we concat it to pathToCreate and we create the folder and we create that folder.
        token = strtok(NULL, "/");
        strcat(pathToCreate,"/");
        if(token == NULL) // if token is NULL it means weve reached the end for the directories path, and now we will need to create a new file.
        {
            strcat(pathToCreate,"\0");
            break;
        }
    }
    
    // creating the file and returning *fp to that file.
    fp = fopen(fullPath,"wb");
    if(!fp)
    {
        free(pathToCreate);
        free(dirPath);
        perror("fopen");
        return NULL;
    }
        
    free(pathToCreate);
    free(dirPath);
    return fp;

}

/**
 * function that receive full path and tries to open that file with "r", if fopen returns NULL then it says the file doesnt exist and we return 0 else we return 1; 
*/
int FileExist(char* filename)
{
    FILE *fp = fopen(filename,"r");
    if(fp != NULL)
    {

        fclose(fp);
        return 1;
    }
    return 0;
}



int main(int argc, char *argv[])
{
    FILE *fp;
    int screenFlag = DEFAULTVALUE,socketfd,rc;
    struct hostent *server;
    struct sockaddr_in serv_addr;
    char **httpRequest;
    unsigned char recieveBuffer[BUFLEN];
    
    if(InputIsValid(&screenFlag,argc,argv) == FAILED) // we first check the URL if its valid, if not we print a Usage error.
    {
        printf("Usage: cproxy <URL> [-s]");
        return 0;
    }
    httpRequest = (char**)calloc(ATTRIBUTES,sizeof(char*));
    if(!httpRequest) {perrorFree(*httpRequest,CALLOC);}
    ParseURL(argv,httpRequest);    
    httpRequest[3] = makeGetRequest(httpRequest);
    httpRequest[4] = FilePath(httpRequest);
    if(FileExist(httpRequest[4])==0) // here we do the proxy check, if the file exist we dont create socket etc..
    {
        printf("HTTP request =\n%s\nLEN = %d\n", httpRequest[3], (int)strlen(httpRequest[3]));
        server = gethostbyname(httpRequest[0]); // we convert the hostname into IP
        if(!server)
        {
            free2DMem(httpRequest);
            herror("gethostbyname");
            exit(EXIT_FAILURE);
        }
        
        if((socketfd = socket(AF_INET,SOCK_STREAM,0))==-1) // connecting the socket
        {
            perror("Socket");
            free2DMem(httpRequest);
            exit(EXIT_FAILURE);
        }
        memset(&serv_addr, 0, sizeof(struct sockaddr_in)); //setting some variables on the socket struct
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_addr.s_addr = ((struct in_addr*)server->h_addr_list[0])->s_addr;
        serv_addr.sin_port = htons(atoi(httpRequest[1]));
        rc = connect(socketfd,(const struct sockaddr *)&serv_addr,sizeof(serv_addr)); // we connect to the server
        if(rc<0)
        {
            free2DMem(httpRequest);
            close(socketfd);
            perror("Connect");
            exit(EXIT_FAILURE);
        }
        int byteRecieved = 0;
        if((rc = write(socketfd,httpRequest[3],strlen(httpRequest[3]))==-1)) // we write into the server pipe our get request
        {
            free2DMem(httpRequest);
            close(socketfd);
            perror("Connect");
        }
        rc = 0;
        int writeBod = 0;
        unsigned char *header,*headerEnd;
        while (1) // loop for reading the response from server
        {
            if ((rc = read(socketfd, recieveBuffer, BUFLEN - 1)) == -1) // reading the response into buffer 
            {
                free2DMem(httpRequest);
                perror("Read");
            }
            if (rc == 0) // if rc==0 we have no more information to read from server
                break;
            byteRecieved += rc;
            recieveBuffer[rc] = '\0';
            header = (unsigned char*)strstr((const char*)recieveBuffer,"200 OK"); // we search the buffer for "200 OK" status, if it exist, we create the directories and the file.
            if(header !=NULL)
            {
                fp = CreateFileDir(httpRequest[4]);
                headerEnd = (unsigned char*) strstr((const char*)recieveBuffer,"\r\n\r\n"); // we search for \r\n\r\n because we know thats the mark of the end of all the headers in the response and from this add + 4 we get the body of the response.
                if(headerEnd != NULL)
                {
                    int length = (int)(&recieveBuffer[rc] - (headerEnd+4));
                    fwrite(headerEnd+4,sizeof(char),length,fp);
                    writeBod = 1;
                }
                header = NULL;
            }
            if(writeBod == 1) // this flag marks to write the buffer also into the body.
            {
                fwrite(recieveBuffer,sizeof(char),rc,fp);
            }
            printf("%s",recieveBuffer);

        }
        if(writeBod == 1)
            fclose(fp);
        printf("\n Total response bytes: %d\n", byteRecieved);
        close(socketfd);
    }
    else // if file already exist in our filesystem, we open the file and read its content.
    {
        char buffer[BUFLEN];
        printf("File is given from local filesystem\n");
        fp = fopen(httpRequest[4],"r+b");
        if(!fp)
        {
            fclose(fp);
            perror("fopen");
            free2DMem(httpRequest);
        }
        struct stat st;
        stat(httpRequest[4],&st);
        printf("HTTP/1.0 200 OK\r\nContent-Length:%ld\r\n\r\n",st.st_size);
        while (fgets(buffer, BUFLEN, fp) != NULL) {printf("%s", buffer);}      
        printf("\n Total response bytes: %ld\n",st.st_size+39+37);
        fclose(fp);
    }
    if(screenFlag == 1 && FileExist(httpRequest[4])) // if the user inserted -s flag we will create commad and open firefox window showing the file
    {
        char command[150];
        sprintf(command,"firefox %s",httpRequest[4]);
        system(command);
    }
    free2DMem(httpRequest);
    return 0;
}
