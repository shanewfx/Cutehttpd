
/**
  @author Ying Zeng (YinzCN_at_Gmail.com)
**/


#ifndef CHTD_BASE_H
#define CHTD_BASE_H


struct usa_t   /* unified socket address */
{
    int len;
    union
    {
        struct sockaddr sa;
        struct sockaddr_in sin;
    } u;
};


struct sock_t
{
    SOCKET socket;
    struct usa_t lsa;
    struct usa_t rsa;
};


int
substr_count(char *, char *);


char *
str_replace(char *, char *, char *);


void *
memndup(void *, int);


char *
strndup(char *, int);


int
is_file(char *);


int
is_dir(char *);


int
is_absolute_path(char *);


void
path_tidy(char *);


void
real_path(char *, char *);


void
url_decode(char *, char *);


void
url_encode(char *, char *, int);


char *
get_file_extname(char *);


int
file_put(char *filename, char *, int);


#endif
