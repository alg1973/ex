#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>

struct http_t {
  char* host;
  short port;
  char* uri;  /* uri doesn't have leading slash. */

  int sock;
  FILE* sockf;
   /* It's a pointer to internal buffer for 
	memory managment simplicity 	*/ 
  char* buf;  
};

#define HTTP_PORT 80
#define DEF_SCHEME "http://"
#define MAXSTACKBUF 2048
#define MAX_HEADERS_LINES 2048
#define MAX_FILE_SIZE (1024*1024*1024)
#define TIMEOUT 6
#define NETREADBUF 65536 


struct http_t*
ht_init_handle(void)
{
  struct http_t* ptr=(struct http_t*)malloc(sizeof(struct http_t));
  if (ptr) {
    memset(ptr,0,sizeof(struct http_t));
    ptr->sock = -1;
  }
  return ptr;
}

void
ht_free_handle(struct http_t* h)
{
  if (h) {
    free(h->buf);
    if (h->sockf)
      fclose(h->sockf);
    else if (h->sock>=0)
      close(h->sock);
    free(h);
  }
}



int  
ht_parse_url(struct http_t* handle,char* t)
{
  /* Only allowed urls with "http://" scheme prefix. */
  if (strncasecmp(DEF_SCHEME,t,sizeof(DEF_SCHEME)-1)==0) {
    free(handle->buf);
    handle->buf=handle->host=strdup(t+sizeof(DEF_SCHEME)-1);
    if (!handle->buf) {
      fprintf(stderr,"Unable to allocate memory for host structure.\n");
      return -1;
    }
    if (strchr(handle->host,'@')) {
      fprintf(stderr,"Authentication url section is not supported.\n");
      return -1;
    }
    /* path is located after the first '/' */  
    if ((handle->uri=strchr(handle->host,'/'))!=NULL) {
      *(handle->uri)='\0';
      handle->uri +=1;
    } else {
      /* Make absent uri path point to empty string not to NULL  */
      handle->uri = handle->host + strlen(handle->host);
    }

    int p=HTTP_PORT;
    char* ports=strchr(handle->host,':');
   
    if (ports!=NULL) {
      *ports='\0';
      /* Skip convertation and syntax errors here */
      p = atoi(ports+1);
      /* Simple check for IP port range */
      if (p<0 && p>0xffff) {
	fprintf(stderr,"Invalid port range, assuming default is 80.\n");
	p=HTTP_PORT;
      } 
    }
    handle->port = p;
    /* minimum check - don't allow empty hosts */
    if (*(handle->host)!='\0')
      return 0;  
  }
  fprintf(stderr,"Invalid url.\n");
  return -1;
}



/* Poor man timeouts. Very ugly timeout handling is here 
   Real system should use timeout handling based on 
   nonblocking io and poll/epoll/select. */

static void
sig_alrm(int signo)
{
/* Do nothing. Networking syscalls should 
   abort with "Interruted system call". */ 
  return; 
}

static void
poor_timer_on(int nsecs)
{
  /* Unrelaible signal syscall - one should use sigaction instead */
  signal(SIGALRM,sig_alrm);
  alarm(nsecs);
}

static void
poor_timer_off(void)
{
  alarm(0);
}

int
ht_connect(struct http_t* h)
{
  if (!h)
    return -1;

  struct addrinfo hints, *records;
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = PF_INET;
  hints.ai_socktype = SOCK_STREAM;
  int error;
  if ((error=getaddrinfo(h->host,NULL, &hints, &records))!=0) {
    fprintf(stderr,"Unable to resolv host: %s.\n", gai_strerror(error));
    return -1;
  }


  uint16_t port = htons(h->port);
  int sock;
  struct addrinfo* r;
  for (r = records; r; r = r->ai_next) {
  // Only IPv4 socket here.
  sock = socket(PF_INET,SOCK_STREAM, 0);
  if (sock < 0) {
    fprintf(stderr,"Unable to create socket endpoint.\n");
    break;
  }
  struct sockaddr_in* iaddr=(struct sockaddr_in*)r->ai_addr;
  iaddr->sin_port = port;
  
  poor_timer_on(TIMEOUT);
  if (connect(sock, r->ai_addr, r->ai_addrlen) < 0) {
    fprintf(stderr,"Unable to connect %s '%s'.\n",
			h->host,strerror(errno));
    close(sock);
    sock=-1;
    continue;
  } else
    break;
  }
  poor_timer_off();
  freeaddrinfo(records);
  if (sock < 0) {
    return -1;
  }
  
  h->sock=sock;
  return 0;
}


/* It's important to send HTTP 1.0 requests since for 1.1 and up
   we need to implement chuncked http read. */

static const char request_template[] = "GET /%s HTTP/1.0\r\n"
  "User-Agent: : Mozilla/1.0 (linux)\r\n"
  "Accept: */*\r\n"
  "Accept-Encoding: identity\r\n"
  "Host: %s\r\n"
  "Connection: close\r\n"
  "\r\n";


int 
ht_send_request(struct http_t* h)
{
  char buf[MAXSTACKBUF];
  int need_write=snprintf(buf,sizeof(buf),request_template,
				h->uri,h->host);

  /* check for buffer overflow with url string */
  if (need_write>sizeof(buf)) {
    fprintf(stderr,"Url is to large.\n");
    return -1;
  }
  int written=0;
  while(need_write>0) {
    poor_timer_on(TIMEOUT);
    int n=write(h->sock,buf+written,need_write);
    /* Don't need timer_off() here */
    if (n<0) {
      fprintf(stderr,"Unable to write to remote server: %s.\n",
		strerror(errno));
      return -1;
    }
    need_write-=n;
    written +=n;
  }
  return 0;
}


int
ht_read_response_hdr(struct http_t* h) 
{
  if ((h->sockf=fdopen(h->sock,"r"))==NULL) {
    fprintf(stderr,"unable to fdopen socket.\n");
    return -1;
  }

  /* We are not going to accept HTTP lines more then MAXSTACKBUF. */
  char buf[MAXSTACKBUF],*line;
  poor_timer_on(TIMEOUT);
  if ((line=fgets(buf,sizeof(buf),h->sockf))==NULL) {
    fprintf(stderr,"Coudn't get status response from server.\n");
    return -1;
  }
  
  int status=-1;
  if (sscanf(line,"HTTP/%*d.%*d%*[ \t]%d",&status)!=1 || status!=200) {
    fprintf(stderr,"Invalid status line response. HTTP status=%d\n",
							status);
    return -1;
  }

  
  /* now rewind headers to the http body */
  int max_lines=MAX_HEADERS_LINES;
  while(1) {
    poor_timer_on(TIMEOUT); /* Don't need timer_off below */
    if ((line=fgets(buf,sizeof(buf),h->sockf))==NULL) {
      fprintf(stderr,"Premature eof in server response.\n");
      return -1;
    }
    /* some sanity check for too big headers*/
    if (--max_lines==0) {
            fprintf(stderr,"Http header contains to much lines.\n");
	    return -1;
    }
    /* it's end of headers mark */
    if (strcmp(line,"\r\n")==0 || strcmp(line,"\n")==0)
      break;
  }
  return 0;
}

int
ht_write_file(struct http_t* h, const char* filename)
{
  FILE* outf=fopen(filename,"w+");
  if (!outf) {
    fprintf(stderr,"Unable to open output file.\n");
    return -1;
  }
  int file_limit=MAX_FILE_SIZE;
  char* buf=(char*)malloc(NETREADBUF);
  if (!buf) {
    fprintf(stderr,"Unable to allocate buffer for network read.\n");
    return -1;
  }
  while(1) {
    poor_timer_on(TIMEOUT);
    int n = fread(buf,1,NETREADBUF,h->sockf);
    poor_timer_off();
    /* reading zero bytes means server've closed 
		connection let's ignore errors */
    if (n==0) 
      break;
    file_limit -=n;
    /* Preventing file bombing. */
    if (file_limit<=0) {
      fprintf(stderr,"File is too large. Stop.\n");
      break;
    }
    int wn = fwrite(buf,1,n,outf);
    if (wn!=n) { /* It's a error on the file system */
      fprintf(stderr,"Error happened while writing file.\n");
      break;
    }
  }
  free(buf);
  fclose(outf);
  return 0;
}


int 
main(int ac, char* av[])
{
  if (ac<2) {
    fprintf(stderr,"Invalid usage: %s url [filename]\n",av[0]);
    exit(-1);
  }

  char* filename = "/dev/fd/1";
  if (ac==3)
    filename=av[2];

  int exit_code=0;
  struct http_t* hdl = ht_init_handle();
  if (!hdl || ht_parse_url(hdl,av[1])!=0 ||ht_connect(hdl)!=0 || 
      ht_send_request(hdl)!=0 || ht_read_response_hdr(hdl)!=0 || 
      ht_write_file(hdl,filename)!=0) {
    fprintf(stderr,"Unable to retrive and save file.\n");
    exit_code=-1;
  }

  ht_free_handle(hdl);
  exit(exit_code);
}

#if 0

int
main(int ac, char* av) 
{

  struct http_t* hdl = ht_init_handle();
  printf("ret %d, host '%s', port %hd, uri '%s'\n", ht_parse_url(hdl,"http://"),hdl->host,hdl->port,hdl->uri);
  printf("ret %d, host '%s', port %hd, uri '%s'\n", ht_parse_url(hdl,"httpx://"),hdl->host,hdl->port,hdl->uri);
  printf("ret %d, host '%s', port %hd, uri '%s'\n", ht_parse_url(hdl,""),hdl->host,hdl->port,hdl->uri);
  printf("ret %d, host '%s', port %hd, uri '%s'\n", ht_parse_url(hdl,"http://mail.ru"),hdl->host,hdl->port,hdl->uri);
  printf("ret %d, host '%s', port %hd, uri '%s'\n", 
	 ht_parse_url(hdl,"http://mail.ru:80/"),hdl->host,hdl->port,hdl->uri);
  printf("ret %d, host '%s', port %hd, uri '%s'\n", 
	 ht_parse_url(hdl,"http://mail.ru/bla/bla/bla.h"),hdl->host,hdl->port,hdl->uri);
  printf("ret %d, host '%s', port %hd, uri '%s'\n", ht_parse_url(hdl,"http://:"),hdl->host,hdl->port,hdl->uri);
  printf("ret %d, host '%s', port %hd, uri '%s'\n", ht_parse_url(hdl,"http://mail.ru:"),hdl->host,hdl->port,hdl->uri);
  printf("ret %d, host '%s', port %hd, uri '%s'\n", ht_parse_url(hdl,"http://mvideo.ru:path"),hdl->host,hdl->port,hdl->uri);

}

#endif
