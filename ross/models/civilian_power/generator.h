
 #include <ross.h>
 
 //By Curtis Antolik

typedef struct {
    double MyCoordsX;
    double MyCoordsY;
    int nextJumpIDs[100];
    int jumpArrayLength;
    int health;
    int draw;
    int available;
    
} Generator_State;

