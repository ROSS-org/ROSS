#include "Util.h"

/*
 * Read a line and eliminate the comments.
 * @return number of bytes read. May return EOF. < 0 on error.
 */
int ReadLine (int fd, char * buf)
{
  ssize_t n;
  char * linefeed, * comment;
  
  if (buf == NULL) return -1;
  
  if ((n = read (fd, buf, MAX_LINE_LEN)) <= 0) return n;

  if ((linefeed = strstr (buf, "\n")) == NULL) return -2;
  
  if (lseek (fd, - (n - (linefeed - buf + 1)), SEEK_CUR) < 0) return -3;
  
  *linefeed = '\0';      
  
  if ((comment = strstr (buf, "#")) != NULL) *comment = '\0';
  
  return (linefeed - buf + 1);
}
