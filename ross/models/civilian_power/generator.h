
 #include <ross.h>
 
 //By Curtis Antolik

 typedef struct Generator_State Generator_State;

 struct Generator_State{
    double MyCoordsX;
    double MyCoordsY;
    int nextJumpIDs[100];
    int jumpArrayLength;
    int health;
    int draw;
    int available;
    
};

