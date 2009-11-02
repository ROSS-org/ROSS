/***************************************************************************/
/* Includes ***************************************************************/
/***************************************************************************/

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<errno.h>
#include<stdarg.h>
#include"tcp.h"

/***************************************************************************/
/* Defines *****************************************************************/
/***************************************************************************/

#define MAX_NODES 100000
#define MAX_LINKS_PER_NODE 128

#define MAX_LINE_WIDTH 2048
#define MAX_BUFFER_SIZE  80

/***************************************************************************/
/* Type Decs ***************************************************************/
/***************************************************************************/

/***************************************************************************/
/* Function Decs ***********************************************************/
/***************************************************************************/

void init_node_list();
void print_node_list();
int read_rocketfuel_file( char *filename );
void parse_rocketfuel_line( char *buffer );
char *get_rocketfuel_item( char *item, char *buffer);

/***************************************************************************/
/* Global Vars *************************************************************/
/***************************************************************************/


rocket_fuel_node *g_node_list;
unsigned long g_node_count=0;
FILE *g_rocketfuel_file = NULL;
unsigned long g_max_links =  0;

/***************************************************************************/
/* Function Bodys **********************************************************/
/***************************************************************************/

/* Start: init_node_list ***************************************************/

void init_node_list()
{
  int i;
  g_node_list = malloc(sizeof(rocket_fuel_node) *100000);

  for( i = 0; i < MAX_NODES; i++ )
    g_node_list[i].used = 0;
}

/* Start: print_node_list **************************************************/
void print_node_list()
{
  int i, j;

  for( i = 0; i < g_node_count; i++ )
    {
      if( g_node_list[i].used )
	{
	  printf("Node %d has the following links \n", i );

	  for( j = 0; j < g_node_list[i].num_links; j++ )
	    printf("   Link %d: %d \n", j, g_node_list[i].link_list[j].node_id );

	  if( g_node_list[i].num_links > g_max_links )
	    g_max_links = g_node_list[i].num_links;
	}
    }

  printf("max links is %d\n", g_max_links);
}

/* Start: read_grade_file **************************************************/
int read_rocketfuel_file( char *filename )
{
  int i;
  char buffer[MAX_LINE_WIDTH];
  int number_of_nodes = 0;

  if( NULL == (g_rocketfuel_file = fopen( filename, "r" )) )
    {
      printf("Error in main: failed to open file for reason: %s \n",
	      strerror( errno ) );
      exit(1);
    }

  while( NULL != fgets( buffer, MAX_LINE_WIDTH, g_rocketfuel_file ) )
    {
      i = number_of_nodes;
      number_of_nodes++;

      if( number_of_nodes == MAX_NODES )
	{
	  printf("Error in read_rocketfuel_file: number of nodes > MAX \n" );
	  exit(1);
	}

      parse_rocketfuel_line( buffer );
    }

  printf("read_rocketfuel_file: File %s completely read, found %d nodes and ready for next task \n",
          filename, number_of_nodes );

  fclose( g_rocketfuel_file );
  g_rocketfuel_file = NULL;

  return( number_of_nodes );
}

/* Start: parse_rocketfuel_line ********************************************/
void parse_rocketfuel_line( char *buffer )
{
  int i, j;
  char item[MAX_LINE_WIDTH];
  char *position = buffer;
  int node = 0;

  /* factor out lines that are comments */
  if( buffer[0] == '#' )
    return;

  /* get uid */
  position = get_rocketfuel_item( item, position );
  node = atoi( item );

  /* nodes < 0 are external to this AS */
  if( node < 0 )
    return;

  /* 
   *nodes are not quite sequential but they are
   *monotonically increasing
   *so, we always record the last node found.
   */
  g_node_count = node;
  g_node_list[node].used = 1;

  /* consume location */
  position = get_rocketfuel_item( item, position );

  /* next check to see if next character is a
   * +, and or bb 
   */
  position = get_rocketfuel_item( item, position );
  if( item[0] == '+' )
    {

      /* consume the bb, if exists */
      position = get_rocketfuel_item( item, position );
      if( item[0] == 'b' )
	{
	  g_node_list[node].level = 0;

	  /* get the number of links */
	  position = get_rocketfuel_item( item, position );
	}
    }
  else
      g_node_list[node].level = 1;

  for( i = 1; i < strlen(item); i++)
    {
      item[i-1] = item[i];
      if( item[i] == ')' )
	{
	  item[i-1] = '\n';
	  continue;
	}
    }
  g_node_list[node].num_links = atoi( item );
  /* consume -> or &# */
  position = get_rocketfuel_item( item, position );
  if( item[0] == '&')
    {
      /* consume the -> */
      position = get_rocketfuel_item( item, position );
    }

  for( j = 0; j < g_node_list[node].num_links; j++)
    {
      position = get_rocketfuel_item( item, position );
      
      for( i = 1; i < strlen(item); i++)
	{
	  item[i-1] = item[i];
	  if( item[i] == '>' || item[i] == '}')
	    {
	      item[i-1] = (char)0;
	      continue;
	    }
	}

      if( atoi( item ) >= 0 )
	g_node_list[node].link_list[j].node_id = atoi( item );
      else
	break;
    }
  g_node_list[node].num_links = j;
}

/* Start: get_rocketfuel_item **********************************************/
char *get_rocketfuel_item( char *item, char *buffer)
{
  int i;
  int n;
  char *position = buffer;
  int j = 0;

  n = strlen(position);

  /* consume initial tab or space characters */
  for( i = 0; position[i] == '\t' || position[i] == ' '; i++ );

  /* copy all non-space chars into item */
  for( ; i < n; i++ )
    {
      if( position[i] == '\t' || position[i] == ' ' )
	break;
      else
	{
	  item[j] = position[i];
	  j++;
	}
    }
  
  /* null terminate the item string */
  item[j] = (char)0;

  /* return start of next item in buffer */
  return( &position[i] );
}

/***************************************************************************/
/* Main ********************************************************************/
/***************************************************************************/

/*
int main( int argc, char *argv[] )
{
  int number_of_nodes=0;

  if( 2 > argc )
    {
      printf("Error in main: missing grade file argument \n");
      exit(1);
    }

  init_node_list();

  number_of_nodes = read_rocketfuel_file( argv[1] );

  print_node_list();

  return(1);
}
*/

