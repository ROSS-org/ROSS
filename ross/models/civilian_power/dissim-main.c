 
 #include "civilian.h"
 #include "powerline.h"
 #include "substation.h"
 #include "generator.h"
 #include "building.h"
 #include "dissim-main.h"
 #include <stdio.h>
 #include <stdlib.h>
 
 //By Curtis Antolik

 tw_peid
 mapping(tw_lpid gid)
 {
         return (tw_peid) gid / g_tw_nlp;
 }
 
 void
 init(Dissim_State * d, tw_lp * lp)
 {
    if(lp->gid < 4){
        d->dissimType = CIVILIAN;
        printf("Assigned civilian\n");
    }
    else if(lp->gid < 6){
        d->dissimType = BUILDING;
        printf("Assigned building\n");
    }
    else if(lp->gid < 10){
        d->dissimType = POWERLINE;
        printf("Assigned powerline\n");
    }
    else if(lp->gid < 12){
        d->dissimType = SUBSTATION;
        printf("Assigned substation\n");
    }
    else if(lp->gid == 12){
        d->dissimType = GENERATOR;
        printf("Assigned generator\n");
    }
   switch(d->dissimType){
        case CIVILIAN:
        {

            int i;
            int temp, temp1;
            temp = 0.0;
            temp1 = 1.0;
            d->c.workPath[0][0] = temp;
            d->c.workPath[0][1] = temp1;
            d->c.workLength = 1;
            int z = 1;
            for(z;z<5;z++){
                temp = 0.0;
                temp1 = 1.0;
                d->c.workPath[z][0] = temp;
                d->c.workPath[z][1] = temp1;
                d->c.workLength++;
            }
            d->c.workPath[6][0] = temp;
            d->c.workPath[6][0] = temp1;
            d->c.workLength++;
            
            temp = 0.0;
            temp = -1.0;
            d->c.homePath[0][0] = temp;
            d->c.homePath[0][1] = temp1;
            d->c.homeLength = 1;
            z = 1;
            for(z;z<5;z++){
                temp = 0.0;
                temp1 = -1.0;
                d->c.homePath[z][0] = temp;
                d->c.homePath[z][1] = temp1;
                d->c.homeLength++;
            }
            d->c.homePath[6][0] = temp;
            d->c.homePath[6][1] = temp1;
            d->c.homeLength++;
            
            temp = 1.0;
            temp1 = 0.0;
            d->c.leisurePath[0][0] = temp;
            d->c.leisurePath[0][1] = temp1;
            d->c.leisureLength = 1;
            z = 1;
            for(z;z<5;z++){
                temp = 1.0;
                temp1 = 0.0;
                d->c.leisurePath[z][0] = temp;
                d->c.leisurePath[z][1] = temp1;
                d->c.leisureLength++;
            }
            d->c.leisurePath[6][0] = temp;
            d->c.leisurePath[6][0] = temp1;
            d->c.leisureLength++;  
            
            temp = -1.0;
            temp1 = 0.0;
            d->c.hospitalPath[0][0] = temp;
            d->c.hospitalPath[0][1] = temp1;
            d->c.hospitalLength = 1;
            z = 1;
            for(z;z<5;z++){
                temp = -1.0;
                temp1 = 0.0;
                d->c.hospitalPath[z][0] = temp;
                d->c.hospitalPath[z][1] = temp1;
                d->c.hospitalLength++;
            }
            d->c.hospitalPath[6][0] = temp;
            d->c.hospitalPath[6][0] = temp1;
            d->c.hospitalLength++;
            
            temp = 1.0;
            temp1 = 1.0;
            d->c.escapePath[0][0] = temp;
            d->c.escapePath[0][1] = temp1;
            d->c.escapeLength = 1;
            z = 1;
            for(z;z<5;z++){
                temp = 1.0;
                temp1 = 1.0;
                d->c.escapePath[z][0] = temp;
                d->c.escapePath[z][1] = temp1;
                d->c.escapeLength++;
            }
            d->c.escapePath[6][0] = temp;
            d->c.escapePath[6][1] = temp1;
            d->c.escapeLength++;
            
            tw_event *e, *ec, *ec2, *e3;
            tw_stime ts;
            Msg_Data *m, *m1, *m2, *m3;
            
            d->c.HomeCoordsX = 0;
            d->c.HomeCoordsY = 0;
            d->c.MyCoordsX = d->c.HomeCoordsX;
            d->c.MyCoordsY = d->c.HomeCoordsY;
            int k = lp->gid%5;
            d->c.Home = 1;
            d->c.homeID = 4;
            d->c.workID = 5;
            d->c.Working = 0;
            d->c.Traveling = 0;
            d->c.Leisure = 0;
            d->c.CrisisLevel = 0;
            d->c.Money = 100;
            d->c.Stubbornness = rand() % 2;
            d->c.evacuated = 0;
            d->c.familySize = 3;
            d->c.familyPresent = 3;
            d->c.homePowerUsage = 50;
            d->c.workPowerUsage = 80;
            d->c.leisurePowerUsage = 10;
            d->c.hospitalPowerUsage = 30;
            
            
            
            ts = tw_rand_exponential(lp->rng, A);
            e = tw_event_new(lp->gid, ts, lp);
            m = (Msg_Data *)tw_event_data(e);
            m->event_type = RELEASEHOME;
            tw_event_send(e);
            
            ts = ts + 800;
            ec = tw_event_new(lp->gid,ts,lp);
            m1 = (Msg_Data *)tw_event_data(ec);
            m1->event_type = CRISISLEVELRISES;
            tw_event_send(ec);
            
            ts = ts + 500;
            ec2 = tw_event_new(lp->gid,ts,lp);
            m2 = (Msg_Data *)tw_event_data(ec2);
            m2->event_type = CRISISLEVELRISES;
            tw_event_send(ec2);
	     break;
        }
       case BUILDING:
        {	
            int i;
            tw_event *e, *e1, *e2, *e3;
            tw_stime ts;
            Msg_Data *m, *m1, *m2, *m3;
            //Coords awaiting file parsing
            d->b.hasPower = 1;
            d->b.MyCoordsX = 0;
            d->b.MyCoordsY = 0;
            d->b.powerlineServicing= lp->gid+2;
            d->b.health = 100.0;
            d->b.draw = 400;
            d->b.capacity = 4;
            d->b.occupancy = 4;
            d->b.occupants[0] = 0;
            d->b.occupants[1] = 1;
            d->b.occupants[2] = 2;
            d->b.occupants[3] = 3;
            d->b.occupantPresent[0] = 1;
            d->b.occupantPresent[1] = 1;
            d->b.occupantPresent[2] = 1;
            d->b.occupantPresent[3] = 1;
            ts = tw_rand_exponential(lp->rng, A);
            e = tw_event_new(lp->gid, 2, lp);
            m = (Msg_Data *)tw_event_data(e);
            m->event_type = UPDATEDRAW;
            tw_event_send(e);
            /*e1 = tw_event_new(lp->gid, 2, lp);
            m1 = (Msg_Data *)tw_event_data(e1);
            m1->InfoBlock1 = -1;
            m1->event_type = UPDATEAVAILABLE;
            tw_event_send(e1);*/
            /*e2 = tw_event_new(lp->gid, 3, lp);
            m2 = (Msg_Data *)tw_event_data(e2);
            m2->InfoBlock1 = 1;
            m2->event_type = UPDATEOCCUPANCY;
            tw_event_send(e2);
            e3 = tw_event_new(lp->gid, 4, lp);
            m3 = (Msg_Data *)tw_event_data(e3);
            m3->InfoBlock1 = 1;
            m3->event_type = UPDATEOCCUPANCY;
            tw_event_send(e3);*/
		break;
        }
        case POWERLINE:
        {
            int i;
            tw_event *e, *ec, *ec2;
            tw_stime ts;
            Msg_Data *m, *m1, *m2;
            d->p.MyCoordsX = 0;
            d->p.MyCoordsY = lp->gid;
            if(lp->gid == 6){
                d->p.nextJumpID = 4;
                d->p.prevJumpID = 10;
            }
            if(lp->gid == 7){
                d->p.nextJumpID = 5;
                d->p.prevJumpID = 11;
            }
            if(lp->gid == 8){
                d->p.nextJumpID = 10;
                d->p.prevJumpID = 12;
            }
            if(lp->gid == 9){
                d->p.nextJumpID = 11;
                d->p.prevJumpID = 12;
            }
            d->p.health = 100.0;
            d->p.draw = 0;
            d->p.available = 900000;
            /*if(lp->gid%2000 == 0 && lp->gid < 200000){
                //ts = tw_rand_exponential(lp->rng, A);
                e = tw_event_new(lp->gid, 0, lp);
                m = (Msg_Data *)tw_event_data(e);
                m->event_type = UPDATEAVAILABLE;
                m->InfoBlock1 = 950;
                tw_event_send(e);
            }*/
            /*if(lp->gid%2000 == 1999){

                //ts = tw_rand_exponential(lp->rng, A);
                e = tw_event_new(lp->gid, 1, lp);
                m = (Msg_Data *)tw_event_data(e);
                m->event_type = UPDATEDRAW;
                m->InfoBlock1 = 50;
                tw_event_send(e);
            }*/

		break;
        }
        case SUBSTATION:
        {
            //if(lp->gid%40 != 19)
              //  printf("%d \n",d->dissimType);
            int i;
            tw_event *e, *ec, *ec2;
            tw_stime ts;
            Msg_Data *m, *m1, *m2;
            d->s.MyCoordsX = 0;
            d->s.MyCoordsY = lp->gid;
            d->s.nextIDEnd = 1;
            d->s.nextJumpIDs[0] = lp->gid-4;
            d->s.prevJumpID = 12;
            d->s.health = 100.0;
            d->s.draw = 0;
            d->s.available = 1000000;
            /*if(tw_rand_unif(lp->rng) > 0.85){
                ts = tw_rand_exponential(lp->rng, 500);
                e = tw_event_new(lp->gid, ts, lp);
                m = (Msg_Data *)tw_event_data(e);
                m->event_type = DAMAGE;
                m->InfoBlock1 = 100;
                tw_event_send(e);
            }*/
            /*
            if(lp->gid == 1023){
                //ts = tw_rand_exponential(lp->rng, A);
                e = tw_event_new(lp->gid, 1, lp);
                m = (Msg_Data *)tw_event_data(e);
                m->event_type = UPDATEDRAW;
                m->InfoBlock1 = 50;
                tw_event_send(e);
            }*/

		break;
        }
        case GENERATOR:
        {
            int i;
            tw_event *e, *ec, *ec2;
            tw_stime ts;
            Msg_Data *m, *m1, *m2;
            d->g.MyCoordsX = 0;
            d->g.MyCoordsY = lp->gid;
            d->g.jumpArrayLength = 2;
            d->g.nextJumpIDs[0] = 8;
            d->g.nextJumpIDs[1] = 9;
            d->g.health = 100.0;
            d->g.draw = 0;
            d->g.available = 10000000;
                //ts = tw_rand_exponential(lp->rng, A);
                e = tw_event_new(lp->gid, 0, lp);
                m = (Msg_Data *)tw_event_data(e);
                m->event_type = UPDATEAVAILABLE;
                m->InfoBlock1 = d->g.available;
                tw_event_send(e);
                ts = tw_rand_exponential(lp->rng, 500);
                ec = tw_event_new(lp->gid, ts, lp);
                m1 = (Msg_Data *)tw_event_data(ec);
                m1->event_type = DAMAGE;
                m1->InfoBlock1 = 100;
                tw_event_send(ec);
            /*if(lp->gid == 1023){
                //ts = tw_rand_exponential(lp->rng, A);
                e = tw_event_new(lp->gid, 1, lp);
                m = (Msg_Data *)tw_event_data(e);
                m->event_type = UPDATEDRAW;
                m->InfoBlock1 = 50;
                tw_event_send(e);
            }*/
		break;
        }
   }
 }
 
 void
 event_handler(Dissim_State * d, tw_bf * bf, Msg_Data * msg, tw_lp * lp)
 {
   char buffer[50];
   char buffer2[100];
   switch(d->dissimType){
        case CIVILIAN:
        {
            int rand_result;
           tw_lpid dst_lp;
           tw_stime ts;
           tw_event *e, *e1, *e2, *e3;
           Msg_Data *m, *m1, *m2, *m3;
           FILE *fp;
           char buffer[50];
           char buffer2[100];
           ts = tw_rand_unif(lp->rng)+0.05;
           //if(lp->gid == 200001) printf("checkin %d\n",msg->event_type);
           switch(msg->event_type)
             {
            
                case HOME:
                    d->c.Home=1;
                    d->c.Traveling=0;
                    ts = ts + 480; //Sets them out for work at 8 am the next day
                    e2 = tw_event_new(d->c.homeID,tw_rand_unif(lp->rng)+0.05,lp);
                    m2 = (Msg_Data*)tw_event_data(e2);
                    m2->event_type = UPDATEOCCUPANCY;
                    m2->InfoBlock1 = lp->gid;
                    tw_event_send(e2);
                    //Update the power usage at the home
                    e = tw_event_new(d->c.homeID, tw_rand_unif(lp->rng)+0.05,lp);
                    m = (Msg_Data *)tw_event_data(e);
                    m->event_type = UPDATEDRAW;
                    m->InfoBlock1 = d->c.homePowerUsage;
                    tw_event_send(e);
                    e1 = tw_event_new(lp->gid, ts, lp);
                    m1 = (Msg_Data *)tw_event_data(e1);
                    m1->event_type = RELEASEHOME;
                    tw_event_send(e1);
                    break;
                case WORK:
                    d->c.Working=1;
                    d->c.Traveling=0;
                    ts = ts + 480;  //They leave work in 8 hours
                    e2 = tw_event_new(d->c.workID,tw_rand_unif(lp->rng)+0.05,lp);
                    m2 = (Msg_Data*)tw_event_data(e2);
                    m2->event_type = UPDATEOCCUPANCY;
                    m2->InfoBlock1 = lp->gid;
                    tw_event_send(e2);
                    e1 = tw_event_new(d->c.workID, tw_rand_unif(lp->rng)+0.05, lp);
                    m1 = (Msg_Data *)tw_event_data(e1);
                    m1->event_type = UPDATEDRAW;
                    m1->InfoBlock1 = d->c.workPowerUsage;
                    tw_event_send(e1);
                    e = tw_event_new(lp->gid, ts, lp);
                    m = (Msg_Data *)tw_event_data(e);
                    m->event_type = RELEASEWORK;
                    tw_event_send(e);
                    break;
                case LEISURE:
                    d->c.Leisure=1;
                    d->c.Traveling=0;
                    ts = ts+60;
                    e2 = tw_event_new(d->c.leisureID,tw_rand_unif(lp->rng)+0.05,lp);
                    m2 = (Msg_Data*)tw_event_data(e2);
                    m2->event_type = UPDATEOCCUPANCY;
                    m2->InfoBlock1 = lp->gid;
                    tw_event_send(e2);
                    e1 = tw_event_new(d->c.leisureID,tw_rand_unif(lp->rng)+0.05,lp);
                    m1 = (Msg_Data *)tw_event_data(e1);
                    m1->event_type = UPDATEDRAW;
                    m1->InfoBlock1 = d->c.leisurePowerUsage;
                    tw_event_send(e1);
                    e = tw_event_new(lp->gid, ts, lp);
                    m = (Msg_Data *)tw_event_data(e);
                    m->event_type = RELEASELEISURE;
                    tw_event_send(e);
                    break;
                case HOSPITAL:
                    d->c.Hospital = 1;
                    d->c.Traveling = 0;
                    ts = ts + 480;
                    e2 = tw_event_new(d->c.hospitalID, tw_rand_unif(lp->rng)+0.05,lp);
                    m2 = (Msg_Data *)tw_event_data(e2);
                    m2->event_type = UPDATEOCCUPANCY;
                    m2->InfoBlock1 = lp->gid;
                    tw_event_send(e2);
                    e1 = tw_event_new(lp->gid, tw_rand_unif(lp->rng)+0.05, lp);
                    m1 = (Msg_Data *)tw_event_data(e1);
                    m1->event_type = UPDATEDRAW;
                    m1->InfoBlock1 = d->c.hospitalPowerUsage;
                    tw_event_send(e1);
                    e =  tw_event_new(lp->gid, ts, lp);
                    m = (Msg_Data *)tw_event_data(e);
                    m->event_type = RELEASEHOSPITAL;
                    tw_event_send(e);
                    break;
                case TRAVELHOME:
                    if((bf->c1 = (d->c.Traveling == 0))){
                        d->c.Traveling=1;
                        if((bf->c5 = (d->c.Working == 1))){
                            d->c.Working=0;
                        }
                        if((bf->c6 = (d->c.Leisure == 1))){
                            d->c.Leisure = 0;
                        }
                        if((bf->c7 = (d->c.Hospital == 1))){
                            d->c.Hospital = 0;
                        }
                        d->c.curTravelNode = 0;
                    }
                    else{
                        if((bf->c2 = (d->c.curTravelNode <= (d->c.homeLength - 2)))){
                            d->c.curTravelNode++;
                        }
                    }
                    d->c.MyCoordsX += d->c.homePath[d->c.curTravelNode][0];
                    d->c.MyCoordsY += d->c.homePath[d->c.curTravelNode][1];
                    ts = ts + 15 * (1 + (1-(d->c.health/100)));
                    e = tw_event_new(lp->gid, ts, lp);
                    m = (Msg_Data *)tw_event_data(e);
                    if((bf->c3 = (d->c.curTravelNode >= (d->c.homeLength-1)))){
                        if((bf->c4 = (d->c.CrisisLevel > d->c.Stubbornness))){
                            d->c.Traveling = 0;
                            m->event_type = TRAVELEVACUATE;
                        }
                        else{
                            m->event_type = HOME;
                        }
                    }
                    else{
                        m->event_type = TRAVELHOME;
                    }
                    tw_event_send(e);
                    break;
                case TRAVELWORK:
                    if((bf->c1 = (d->c.Traveling == 0))){
                        d->c.Traveling=1;
                        d->c.Home=0;
                        d->c.curTravelNode = 0;
                    }
                    else{
                        if((bf->c2 = (d->c.curTravelNode <= (d->c.workLength - 2)))){
                            d->c.curTravelNode++;
                        }
                    }
                    d->c.MyCoordsX += d->c.workPath[d->c.curTravelNode][0];
                    d->c.MyCoordsY += d->c.workPath[d->c.curTravelNode][1];
                    ts = ts + 15 * (1 + (1-(d->c.health/100)));
                    e = tw_event_new(lp->gid, ts, lp);
                    m = (Msg_Data *)tw_event_data(e);
                    if((bf->c3 = (d->c.curTravelNode >= (d->c.workLength - 1)))){
                        if((bf->c4 = (d->c.CrisisLevel > d->c.Stubbornness))){
                            d->c.Traveling = 0;
                            m->event_type = TRAVELHOME;
                        }
                        else{
                            m->event_type = WORK;
                        }
                    }
                    else{
                        m->event_type = TRAVELWORK;
                    }
                    tw_event_send(e);
                    break;
                case TRAVELLEISURE:
                    if((bf->c1 = (d->c.Traveling == 0))){
                        d->c.Traveling=1;
                        if((bf->c5 = (d->c.Home == 1))){
                            d->c.Home=0;
                        }
                        if((bf->c6 = (d->c.Working == 1))){
                            d->c.Working = 0;
                        }
                        d->c.curTravelNode = 0;
                    }
                    else{
                        if((bf->c2 = (d->c.curTravelNode <= (d->c.leisureLength - 2)))){
                            d->c.curTravelNode++;
                        }
                    }
                    d->c.MyCoordsX += d->c.leisurePath[d->c.curTravelNode][0];
                    d->c.MyCoordsY += d->c.leisurePath[d->c.curTravelNode][1];
                    ts = ts + 15 * (1 + (1-(d->c.health/100)));
                    e = tw_event_new(lp->gid, ts, lp);
                    m = (Msg_Data *)tw_event_data(e);
                    if((bf->c3 = (d->c.curTravelNode >= (d->c.workLength - 1)))){
                        if((bf->c4 = (d->c.CrisisLevel > d->c.Stubbornness))){
                            d->c.Traveling = 0;
                            m->event_type = TRAVELHOME;
                        }
                        else{
                            m->event_type = LEISURE;
                        }
                    }
                    else{
                        m->event_type = TRAVELLEISURE;
                    }
                    tw_event_send(e);
                    break;
                case TRAVELHOSPITAL:
                    if((bf->c1 = (d->c.Traveling == 0))){
                        d->c.Traveling=1;
                        if((bf->c5 = (d->c.Home == 1))){
                            d->c.Home = 0;
                        }
                        else if((bf->c6 = (d->c.Working == 1))){
                            d->c.Working = 0;
                        }
                        else if ((bf->c7 = (d->c.Leisure == 1))){
                            d->c.Leisure = 0;
                        }
                        d->c.curTravelNode = 0;
                    }
                    else{
                        if((bf->c2 = (d->c.curTravelNode <= (d->c.hospitalLength - 2)))){
                            d->c.curTravelNode++;
                        }
                    }
                    d->c.MyCoordsX += d->c.hospitalPath[d->c.curTravelNode][0];
                    d->c.MyCoordsY += d->c.hospitalPath[d->c.curTravelNode][1];
                    ts = ts + 15 * (1 + (1-(d->c.health/100)));
                    e = tw_event_new(lp->gid, ts, lp);
                    m = (Msg_Data *)tw_event_data(e);
                    if((bf->c3 = (d->c.curTravelNode >= (d->c.workLength - 1)))){
                        if((bf->c4 = (d->c.CrisisLevel > d->c.Stubbornness))){
                            d->c.Traveling = 0;
                            m->event_type = TRAVELHOME;
                        }
                        else{
                            m->event_type = HOSPITAL;
                        }
                    }
                    else{
                        m->event_type = TRAVELHOSPITAL;
                    }
                    tw_event_send(e);
                    break;
                case CRISISLEVELRISES:
                    d->c.CrisisLevel+=1;
                    if((bf->c1 = (d->c.CrisisLevel == 1)))
                        d->c.CrisisLevelStart = tw_now(lp);
                    if((bf->c2 = (d->c.Stubbornness < d->c.CrisisLevel))){
                        if((bf->c3 = (d->c.Traveling == 0))){
                            ts = ts + 0;
                            e = tw_event_new(lp->gid, 0, lp);
                            m = (Msg_Data *)tw_event_data(e);
                            m->event_type = TRAVELEVACUATE;
                            tw_event_send(e);
                        }
                    }
                    break;
                case CRISISLEVELFALLS:
                    d->c.CrisisLevel-=1;
                    if((bf->c1 = (d->c.CrisisLevel == 0)))
                        d->c.TimeInCrisisLevel += tw_now(lp) - d->c.CrisisLevelStart;
                    if((bf->c2 = (d->c.Stubbornness < d->c.CrisisLevel))){
                    }
                    break;
                case CURIOUS:
                    printf("Like a whisper\n");
                    break;
                case DAMAGE:
                {
                    int update = msg->InfoBlock1;
                    d->c.health -= update;
                    break;
                }
                case REPAIR:
                {
                    int update = msg->InfoBlock1;
                    d->c.health += update;
                    break;
                }
                case TRAVELEVACUATE:
                    if((bf->c1 = ((d->c.Working == 1) || (d->c.Leisure == 1) || d->c.Hospital == 1))){
                        ts = ts + 0;
                        e = tw_event_new(lp->gid,ts,lp);
                        m = (Msg_Data *)tw_event_data(e);
                        m->event_type = TRAVELHOME;
                        tw_event_send(e);
                    }
                    else{
                        if((bf->c2 = ((d->c.Traveling == 0) && (d->c.familySize = d->c.familyPresent)))){
                            d->c.Traveling=1;
                            d->c.Home=0;
                            d->c.curTravelNode = 0;
                        }
                        else{
                            if((bf->c3 = (d->c.curTravelNode <= (d->c.escapeLength - 2)))){
                                d->c.curTravelNode++;
                            }
                        }
                        d->c.MyCoordsX += d->c.escapePath[d->c.curTravelNode][0];
                        d->c.MyCoordsY += d->c.escapePath[d->c.curTravelNode][1];
                        ts = ts + 15 * (1 + (1-(d->c.health/100)));
                        e = tw_event_new(lp->gid,ts,lp);
                        m = (Msg_Data *)tw_event_data(e);
                        if((bf->c4 = (d->c.curTravelNode >= (d->c.escapeLength - 1)))){
                            d->c.evacuated = 1;
                            m->event_type = CURIOUS;
                        }
                        else{
                            m->event_type = TRAVELEVACUATE;
                        }
                        tw_event_send(e);
                    }
                    break;
                case RELEASEHOME:
                    if((bf->c1 = (d->c.Home == 1))){
                        e1 = tw_event_new(d->c.homeID, ts, lp);
                        m1 = (Msg_Data *)tw_event_data(e1);
                        m1->event_type = UPDATEDRAW;
                        m1->InfoBlock1 = -1*(d->c.homePowerUsage);
                        tw_event_send(e1);
                        e = tw_event_new(lp->gid, tw_rand_unif(lp->rng)+0.05, lp);
                        m = (Msg_Data *)tw_event_data(e);
                        m->event_type = TRAVELWORK;  
                        tw_event_send(e);
                        e2 = tw_event_new(d->c.homeID,tw_rand_unif(lp->rng)+0.05,lp);
                        m2 = (Msg_Data*)tw_event_data(e2);
                        m2->event_type = UPDATEOCCUPANCY;
                        m2->InfoBlock1 = lp->gid;
                        tw_event_send(e2);
                    }
                    break;
                case RELEASEWORK:
                    if((bf->c1 = (d->c.Working == 1))){
                        e1 = tw_event_new(d->c.workID, ts, lp);
                        m1 = (Msg_Data *)tw_event_data(e1);
                        m1->event_type = UPDATEDRAW;
                        m1->InfoBlock1 = -1*(d->c.workPowerUsage);
                        tw_event_send(e1);
                        e = tw_event_new(lp->gid, tw_rand_unif(lp->rng)+0.05, lp);
                        m = (Msg_Data *)tw_event_data(e);
                        m->event_type = TRAVELHOME;  
                        tw_event_send(e);
                        e2 = tw_event_new(d->c.workID,tw_rand_unif(lp->rng)+0.05,lp);
                        m2 = (Msg_Data*)tw_event_data(e2);
                        m2->event_type = UPDATEOCCUPANCY;
                        m2->InfoBlock1 = lp->gid;
                        tw_event_send(e2);
                    }
                    break;
                 case RELEASELEISURE:
                    if((bf->c1 = (d->c.Leisure == 1))){
                        e1 = tw_event_new(d->c.leisureID, ts, lp);
                        m1 = (Msg_Data *)tw_event_data(e1);
                        m1->event_type = UPDATEDRAW;
                        m1->InfoBlock1 = -1*(d->c.leisurePowerUsage);
                        tw_event_send(e1);
                        e = tw_event_new(lp->gid, tw_rand_unif(lp->rng)+0.05, lp);
                        m = (Msg_Data *)tw_event_data(e);
                        m->event_type = TRAVELHOME;  
                        tw_event_send(e);
                        e2 = tw_event_new(d->c.leisureID,tw_rand_unif(lp->rng)+0.05,lp);
                        m2 = (Msg_Data*)tw_event_data(e2);
                        m2->event_type = UPDATEOCCUPANCY;
                        m2->InfoBlock1 = lp->gid;
                        tw_event_send(e2);
                    }
                    break;
                case RELEASEHOSPITAL:
                    if((bf->c1 = (d->c.Hospital== 1))){
                        e1 = tw_event_new(d->c.hospitalID, ts, lp);
                        m1 = (Msg_Data *)tw_event_data(e1);
                        m1->event_type = UPDATEDRAW;
                        m1->InfoBlock1 = -1*(d->c.hospitalPowerUsage);
                        tw_event_send(e1);
                        e = tw_event_new(lp->gid, ts, lp);
                        m = (Msg_Data *)tw_event_data(e);
                        m->event_type = TRAVELHOME;  
                        tw_event_send(e);
                        e2 = tw_event_new(lp->gid,tw_rand_unif(lp->rng)+0.05, lp);
                        m2 = (Msg_Data *)tw_event_data(e2);
                        m2->event_type = REPAIR;
                        m2->InfoBlock1 = 100 - d->c.health;
                        tw_event_send(e2);
                        e3 = tw_event_new(d->c.hospitalID,tw_rand_unif(lp->rng)+0.05,lp);
                        m3 = (Msg_Data*)tw_event_data(e3);
                        m3->event_type = UPDATEOCCUPANCY;
                        m3->InfoBlock1 = lp->gid;
                        tw_event_send(e2);
                    }
                    break;
                case UPDATEOCCUPANCY:
                {
                    int update = msg->InfoBlock1;
                    msg->InfoBlock1 = d->c.familyPresent;
                    d->c.familyPresent = update;
                    if((bf->c1 = (d->c.CrisisLevel > d->c.Stubbornness))){
                        e = tw_event_new(lp->gid,ts,lp);
                        m = (Msg_Data *)tw_event_data(e);
                        m->event_type = TRAVELEVACUATE;
                        tw_event_send(e);
                    }
                break;
                }
             }
             break;
        }
        case BUILDING:
        {
           int rand_result;
           tw_lpid dst_lp;
           tw_stime ts, ts1;
           tw_event *e, *e1, *e2;
           Msg_Data *m, *m1, *m2;
           FILE *fp;
           char buffer[50];
           char buffer2[100];
           MPI_Status status;
           ts = tw_rand_unif(lp->rng)+0.05;
           //ts1 = tw_rand_unif(lp->rng);
           switch(msg->event_type)
             {
                case UPDATEDRAW:
                {
                    if((bf->c1 = (d->b.health > 0.0))){
                        if((bf->c2 = (d->b.powerlineServicing >= 0))){
                            d->b.draw += msg->InfoBlock1;
                        printf("dark4\n");
                            e = tw_event_new(d->b.powerlineServicing, ts+0, lp);
                            m = (Msg_Data *)tw_event_data(e);
                            m->event_type = UPDATEDRAW;  
                            m->InfoBlock1 = d->b.draw;
                            tw_event_send(e);
                        }
                        break;
                    }
                    break;
                }
                case UPDATEAVAILABLE:
                {
                    if((bf->c1 = (d->b.health > 0.0))){
                        if((bf->c3 = (msg->InfoBlock1 <= 0 && d->b.hasPower == 1))){
                            e = tw_event_new(lp->gid, ts + 0, lp);
                            m = (Msg_Data *)tw_event_data(e);
                            m->event_type = POWEROFF;
                            tw_event_send(e);
                        }
                        else if((bf->c2 = (msg->InfoBlock1 > 0 && d->b.hasPower == 0))){
                            e2 = tw_event_new(lp->gid, ts + 0, lp);
                            m2 = (Msg_Data *)tw_event_data(e2);
                            m2->event_type = POWERON;
                            tw_event_send(e2);
                        }
                        else if((bf->c4 = (d->b.hasPower == 1))){
                            e1 = tw_event_new(lp->gid, ts + 0, lp);
                            m1 = (Msg_Data *)tw_event_data(e1);
                            m1->event_type = UPDATEDRAW;
                            m1->InfoBlock1 = 0;
                            tw_event_send(e1);
                        }
                    }
                    break;
                }
                case DAMAGE:
                {
                    int update = msg->InfoBlock1;
                    d->b.health -= update;
                    if((bf->c1 = (d->b.health <= 0.0))){
                        e = tw_event_new(lp->gid, ts + 0, lp);
                        m = (Msg_Data *)tw_event_data(e);
                        m->event_type = POWEROFF;
                        tw_event_send(e);
                    }
                    break;
                }
                case REPAIR:
                {
                    int update = msg->InfoBlock1;
                    d->b.health += update;
                    break;
                }
                case UPDATEOCCUPANCY:
                {
                    int z = 0;
                    int q = 0;
                    int occupant = msg->InfoBlock1;
                    //If the person is present and telling us they are leaving, remove them
                    for(z; z < d->b.capacity; z++){
                        if((bf->c1 = (d->b.occupantPresent[z] == 1))){
                            if((bf->c2 = (d->b.occupants[z] == occupant))){
                                msg->InfoBlock2 = z+1;
                                d->b.occupants[z] = -1;
                                d->b.occupantPresent[z] = 0;
                                d->b.occupancy--;
                        printf("dark4\n");
                                int i = 0;
                                tw_event *stuff[d->b.occupancy];
                                Msg_Data *msgs[d->b.occupancy];
                                for(i; i < d->b.capacity; i++){
                                    if(d->b.occupantPresent[i] == 1){
                                        stuff[i] = tw_event_new(d->b.occupants[i], tw_rand_unif(lp->rng)+0.05,lp);
                                        msgs[i] = (Msg_Data *)tw_event_data(stuff[i]);
                                        msgs[i]->event_type = UPDATEOCCUPANCY;
                                        msgs[i]->InfoBlock1 = d->b.occupancy;
                                        tw_event_send(stuff[i]);
                                    }
                                }
                                q = 1;
                                break;
                            }
                        }
                    }
                    if(q==1){
                        break;
                    }
                    //If the person is new and we have room, add them
                    z = 0;
                    for(z; z < d->b.capacity; z++){
                        if((bf->c3 = (d->b.occupantPresent[z] == 0))){
                            msg->InfoBlock2 = -1*z;
                        printf("dark4\n");
                            d->b.occupants[z] = occupant;
                            d->b.occupantPresent[z] = 1;
                            d->b.occupancy++;
                            int i = 0;
                            tw_event *stuff[d->b.occupancy];
                            Msg_Data *msgs[d->b.occupancy];
                            for(i; i < d->b.capacity; i++){
                                if(d->b.occupantPresent[i] == 1){
                                    stuff[i] = tw_event_new(d->b.occupants[i], tw_rand_unif(lp->rng)+0.05,lp);
                                    msgs[i] = (Msg_Data *)tw_event_data(stuff[i]);
                                    msgs[i]->event_type = UPDATEOCCUPANCY;
                                    msgs[i]->InfoBlock1 = d->b.occupancy;
                                    tw_event_send(stuff[i]);
                                }
                            }
                            break;
                        }
                    }
                    break;
                    //Send message that no room is available.
                            
                }
                case POWERON:
                {
                    if((bf->c1 = (!d->b.hasPower))){
                        d->b.hasPower = 1;
                        int i = 0;
                        tw_event *stuff[d->b.occupancy];
                        printf("dark4\n");
                        Msg_Data *msgs[d->b.occupancy];
                        for(i; i < d->b.capacity; i++){
                            if(d->b.occupantPresent[i] == 1){
                                stuff[i] = tw_event_new(d->b.occupants[i], tw_rand_unif(lp->rng)+0.05,lp);
                                msgs[i] = (Msg_Data *)tw_event_data(stuff[i]);
                                msgs[i]->event_type = CRISISLEVELFALLS;
                                tw_event_send(stuff[i]);
                            }
                        }
                        break;
                    }
                    break;
                }
                case POWEROFF:
                {
                    if((bf->c1 = (d->b.hasPower))){
                        printf("dark4\n");
                        d->b.hasPower = 0;
                        int z = 0;
                        tw_rand_unif(lp->rng);
                        tw_rand_reverse_unif(lp->rng);
                        int i = 0;
                        tw_event *stuff[d->b.occupancy];
                        Msg_Data *msgs[d->b.occupancy];
                        for(i; i < d->b.capacity; i++){
                            if(d->b.occupantPresent[i] == 1){
                                stuff[i] = tw_event_new(d->b.occupants[i], tw_rand_unif(lp->rng)+0.05,lp);
                                msgs[i] = (Msg_Data *)tw_event_data(stuff[i]);
                                msgs[i]->event_type = CRISISLEVELRISES;
                                tw_event_send(stuff[i]);
                            }
                        }
                        break;
                    }
                }
                break;
            }
            break;
        }
        case POWERLINE:
        {
           int rand_result;
           tw_lpid dst_lp;
           tw_stime ts, ts1;
           tw_event *e, *e1;
           Msg_Data *m, *m1;
           FILE *fp;
           char buffer[50];
           char buffer2[100];
           MPI_Status status;
            ts = tw_rand_unif(lp->rng)+0.05;
            //ts1 = tw_rand_unif(lp->rng);
           switch(msg->event_type)
             {
                case UPDATEDRAW:
                {
                    if((bf->c1 = (d->p.health > 0.0))){
                        
                        printf("dark3\n");
                        int update = msg->InfoBlock1;
                        msg->InfoBlock1 = d->p.draw;
                        d->p.draw = update;
                        e = tw_event_new(d->p.prevJumpID, ts+0, lp);
                        m = (Msg_Data *)tw_event_data(e);
                        m->event_type = UPDATEDRAW;  
                        m->InfoBlock1 = update;
                        tw_event_send(e);
                    }
                    break;
                }
                case UPDATEAVAILABLE:
                {
                    if((bf->c1 = (d->p.health > 0.0))){
                        printf("dark3\n");
                        int update = msg->InfoBlock1;
                        msg->InfoBlock1 = d->p.available;
                        d->p.available = update;
                        e = tw_event_new(d->p.nextJumpID, ts+0, lp);
                        m = (Msg_Data *)tw_event_data(e);
                        m->event_type = UPDATEAVAILABLE;
                        m->InfoBlock1 = update;
                        tw_event_send(e);
                    }
                    break;
                }
                case DAMAGE:
                {
                    int update = msg->InfoBlock1;
                    d->p.health -= update;
                    if((bf->c1 = (d->p.health <= 0.0))){
                        printf("dark3\n");
                        msg->InfoBlock2 = d->p.available;
                        d->p.available = 0;
                        e = tw_event_new(d->p.nextJumpID, ts+0, lp);
                        m = (Msg_Data *)tw_event_data(e);
                        m->event_type = UPDATEAVAILABLE;
                        m->InfoBlock1 = d->p.available;
                        tw_event_send(e);
                        e1 = tw_event_new(d->p.prevJumpID, ts+0, lp);
                        m1 = (Msg_Data *)tw_event_data(e1);
                        m1->event_type - UPDATEDRAW;
                        m1->InfoBlock1 = d->p.available;
                        tw_event_send(e1);
                    }
                    break;
                }
                case REPAIR:
                {
                    int update = msg->InfoBlock1;
                    d->p.health += update;
                    break;
                }
                break;
             }
             break;
        }
        case SUBSTATION:
        {
           int rand_result;
           tw_lpid dst_lp;
           tw_stime ts, ts1;
           tw_event *e, *e1;
           Msg_Data *m, *m1;
           FILE *fp;
           char buffer[50];
           char buffer2[100];
           MPI_Status status;
           ts = tw_rand_unif(lp->rng)+0.05;
           //ts1 = tw_rand_unif(lp->rng);
           switch(msg->event_type)
             {
                case UPDATEDRAW:
                {
                    if((bf->c1 = (d->s.health > 0.0))){
                        int update = msg->InfoBlock1;
                        msg->InfoBlock1 = d->s.draw;
                        d->s.draw = update;
                        printf("dark11\n");
                        if((bf->c4 = (d->s.draw > d->s.available))){
                            e = tw_event_new(lp->gid, ts+0, lp);
                            m = (Msg_Data *)tw_event_data(e);
                            m->event_type = DAMAGE;  
                            m->InfoBlock1 = d->s.health;
                            tw_event_send(e);
                        }
                        else {
                            e = tw_event_new(d->s.prevJumpID, 0, lp);
                            m = (Msg_Data *)tw_event_data(e);
                            m->event_type = UPDATEDRAW;  
                            m->InfoBlock1 = update;
                            tw_event_send(e);
                        }
                    }
                    break;
                }
                case UPDATEAVAILABLE:
                {
                    if((bf->c1 = (d->s.health > 0.0))){
                        printf("dark12\n");
                        int update = msg->InfoBlock1;
                        msg->InfoBlock1 = d->s.available;
                        d->s.available = update;
                        int i = 0;
                        tw_event *es[d->s.nextIDEnd];
                        Msg_Data *ms[d->s.nextIDEnd];
                        for (i; i < d->s.nextIDEnd; i++){
                            es[i] = tw_event_new(d->s.nextJumpIDs[i], ts+0, lp);
                            ms[i] = (Msg_Data *)tw_event_data(es[i]);
                            ms[i]->event_type = UPDATEAVAILABLE;
                            ms[i]->InfoBlock1 = update;
                            tw_event_send(es[i]);
                        }
                    }
                    //printf("substation %llu hit %d\n",lp->gid,d->dissimType);
                    break;
                }
                case DAMAGE:
                {
                    
                    /*e1 = tw_event_new(lp->gid+1500000, 0 + 0, lp);
                    m1 = (Msg_Data *)tw_event_data(e1);
                    m1->event_type = CRISISLEVELRISES;
                    tw_event_send(e1);*/
                    //if(lp->gid == 199) printf("Substation %llu TAKING THE FALL FOR THE MOTHERLAND\n",lp->gid);
                    int update = msg->InfoBlock1;
                    d->s.health -= update;
                        printf("dark13\n");
                    if((bf->c1 = (d->s.health <= 0.0))){
                        msg->InfoBlock2 = d->s.available;
                        d->s.available = 0;
                        int i = 0;
                        tw_event *es[d->s.nextIDEnd];
                        Msg_Data *ms[d->s.nextIDEnd];
                        for (i; i < d->s.nextIDEnd; i++){
                            es[i] = tw_event_new(d->s.nextJumpIDs[i], ts+0, lp);
                            ms[i] = (Msg_Data *)tw_event_data(es[i]);
                            ms[i]->event_type = UPDATEAVAILABLE;
                            ms[i]->InfoBlock1 = d->s.available;
                            tw_event_send(es[i]);
                        }
                    }
                    break;
                }
                case REPAIR:
                {
                    int update = msg->InfoBlock1;
                    d->s.health += update;
                    break;
                }
                break;
             }
             break;
        }
        case GENERATOR:
        {
           int rand_result;
           tw_lpid dst_lp;
           tw_stime ts;
           tw_event *e;
           Msg_Data *m;
           Msg_Data *ms[d->g.jumpArrayLength];
           tw_event *es[d->g.jumpArrayLength];
           FILE *fp;
           char buffer[50];
           char buffer2[100];
           MPI_Status status;
           //ts = tw_rand_unif(lp->rng);
           switch(msg->event_type)
             {
                case UPDATEDRAW:
                {
                    //printf("generatore %llu hit %d\n",lp->gid,d->dissimType); 
                    if((bf->c1 = (d->g.health > 0.0))){
                        int update = msg->InfoBlock1;
                        msg->InfoBlock1 = d->g.draw;
                        d->g.draw = update;
                        printf("dark2\n");
                        //if(lp->gid == 0) printf("%d %d\n",d->g.draw,d->g.available);
                        if((bf->c2 = (d->g.draw > d->g.available))){
                            //printf("blowout\n");
                            e = tw_event_new(lp->gid, 0 + 0, lp);
                            m = (Msg_Data *)tw_event_data(e);
                            m->event_type = DAMAGE;
                            m->InfoBlock1 = d->g.health;
                            tw_event_send(e);
                        }
                    }
                    break;
                }
                case UPDATEAVAILABLE:
                {
                    //printf("Reached Update Available - Generator %llu\n",lp->gid);
                    if((bf->c1 = (d->g.health > 0.0))){
                        int update = msg->InfoBlock1;
                        msg->InfoBlock1 = d->g.available;
                        d->g.available = update;
                        int i = 0;
                        printf("dark2\n");
                        for(i; i < d->g.jumpArrayLength; i++){
                        //if(lp->gid == 0)    printf("Generator - %d Messages sent %llu\n",i,lp->gid);
                            es[i] = tw_event_new(d->g.nextJumpIDs[i], 0 + 0, lp);
                            ms[i] = (Msg_Data *)tw_event_data(es[i]);
                            ms[i]->event_type = UPDATEAVAILABLE;
                            ms[i]->InfoBlock1 = d->g.available;
                            tw_event_send(es[i]);
                        }
                        /*e = tw_event_new(lp->gid, 200,lp);
                        m = (Msg_Data *)tw_event_data(e);
                        m->event_type = UPDATEAVAILABLE;
                        m->InfoBlock1 = d->g.available - 50;
                        tw_event_send(e);*/
                    }
                    break;
                }
                case DAMAGE:
                {
                    //printf("stop\n");
                    int update = msg->InfoBlock1;
                    d->g.health -= update;
                        printf("dark2\n");
                    if((bf->c1 = (d->g.health <= 0.0))){
                        msg->InfoBlock2 = d->g.available;
                        d->g.available = 0;
                        int i = 0;
                        for(i; i < d->g.jumpArrayLength; i++){
                            es[i] = tw_event_new(d->g.nextJumpIDs[i], 0 + 0, lp);
                            ms[i] = (Msg_Data *)tw_event_data(es[i]);
                            ms[i]->event_type = UPDATEAVAILABLE;
                            ms[i]->InfoBlock1 = d->g.available;
                            tw_event_send(es[i]);
                        }
                    }
                    break;
                }
                case REPAIR:
                {
                    int update = msg->InfoBlock1;
                    d->g.health += update;
                    break;
                }
                break;
             }
             break;
        }
 }
}
void rc_event_handler(Dissim_State * d, tw_bf * bf, Msg_Data * msg, tw_lp * lp)
 {
   char buffer[50];
   char buffer2[100];
   /*if(lp->gid%2000 == 1999){
   printf("hit %d %llu\n", msg->event_type, lp->gid);
   }*/

   //if(tw_now(lp) > 220) printf("%d %llu %f\n",msg->event_type, lp->gid, tw_now(lp));
   switch(d->dissimType){
        case CIVILIAN:
        {
            int rand_result;
           tw_lpid dst_lp;
           tw_stime ts;
           tw_event *e;
           Msg_Data *m;
           FILE *fp;
           char buffer[50];
           char buffer2[100];
           tw_rand_reverse_unif(lp->rng);
           switch(msg->event_type)
             {
            
                case HOME:
                    d->c.Home=0;
                    d->c.Traveling=1;
                    tw_rand_reverse_unif(lp->rng);
                    tw_rand_reverse_unif(lp->rng);
                    break;
                case WORK:
                    d->c.Working=0;
                    d->c.Traveling=1;
                    tw_rand_reverse_unif(lp->rng);
                    tw_rand_reverse_unif(lp->rng);
                    break;
                case LEISURE:
                    d->c.Leisure=0;
                    d->c.Traveling=1;
                    tw_rand_reverse_unif(lp->rng);
                    tw_rand_reverse_unif(lp->rng);
                    break;
                case HOSPITAL:
                    d->c.Hospital = 0;
                    d->c.Traveling = 1;
                    tw_rand_reverse_unif(lp->rng);
                    tw_rand_reverse_unif(lp->rng);
                case TRAVELHOME:
                    if(bf->c1){
                    d->c.Traveling = 1;
                    if(bf->c5){
                        d->c.Working = 1;
                    }
                    if(bf->c6){
                        d->c.Leisure = 1;
                    }
                    if(bf->c7){
                        d->c.Hospital = 1;
                    }
                    else{
                        if(bf->c2){
                            d->c.curTravelNode--;
                        }
                    }
                    d->c.MyCoordsX -= d->c.homePath[d->c.curTravelNode][0];
                    d->c.MyCoordsY -= d->c.homePath[d->c.curTravelNode][1];
                    if(bf->c3){
                        if(bf->c4){
                            d->c.Traveling = 1;
                        }
                    }
                    break;
                case TRAVELWORK:
                    if(bf->c1){
                        d->c.Traveling=1;
                        d->c.Home=1;
                        d->c.curTravelNode = 0;
                    }
                    else{
                        if(bf->c2){
                            d->c.curTravelNode--;
                        }
                    }
                    d->c.MyCoordsX -= d->c.workPath[d->c.curTravelNode][0];
                    d->c.MyCoordsY -= d->c.workPath[d->c.curTravelNode][1];
                    if(bf->c3){
                        if(bf->c4){
                            d->c.Traveling = 1;
                        }
                        else{
                        }
                    }
                    else{
                    }
                    break;
                case TRAVELLEISURE:
                    if(bf->c1){
                        d->c.Traveling = 0;
                        if(bf->c5){
                            d->c.Home=1;
                        }
                        if(bf->c6){
                            d->c.Working=1;
                        }
                    }
                    else{
                        if(bf->c2){
                            d->c.curTravelNode--;
                        }
                    }
                    d->c.MyCoordsX -= d->c.leisurePath[d->c.curTravelNode][0];
                    d->c.MyCoordsY -= d->c.leisurePath[d->c.curTravelNode][1];
                    if(bf->c3){
                        if(bf->c4){
                            d->c.Traveling=1;
                        }
                    }
                    break;
                case TRAVELHOSPITAL:
                    if(bf->c1){
                        d->c.Traveling = 0;
                        if(bf->c5){
                            d->c.Home=1;
                        }
                        if(bf->c6){
                            d->c.Working=1;
                        }
                        if(bf->c7){
                            d->c.Leisure=1;
                        }
                    }
                    else{
                        if(bf->c2){
                            d->c.curTravelNode--;
                        }
                    }
                    d->c.MyCoordsX -= d->c.hospitalPath[d->c.curTravelNode][0];
                    d->c.MyCoordsY -= d->c.hospitalPath[d->c.curTravelNode][1];
                    if(bf->c3){
                        if(bf->c4){
                            d->c.Traveling=1;
                        }
                    }
                    break;
                case CRISISLEVELRISES:
                    d->c.CrisisLevel-=1;
                    if(bf->c1)
                        d->c.CrisisLevelStart = 0;
                    if(bf->c2){
                        if(bf->c3){
                        }
                    }
                    break;
                case CRISISLEVELFALLS:
                    d->c.CrisisLevel+=1;
                    if(bf->c1)
                        d->c.TimeInCrisisLevel = 0;
                    if(bf->c2){
                    }
                    break;
                case CURIOUS:
                    break;
                case DAMAGE:
                    d->c.health += msg->InfoBlock1;
                    break;
                case REPAIR:
                    d->c.health -= msg->InfoBlock1;
                    break;
                case TRAVELEVACUATE:
                    if(bf->c1){
                    }
                    else{
                        if(bf->c2){
                            d->c.Traveling=0;
                            d->c.Home=1;
                            d->c.curTravelNode = 0;
                        }
                        else{
                            if(bf->c3){
                                d->c.curTravelNode--;
                            }
                        }
                        d->c.MyCoordsX -= d->c.escapePath[d->c.curTravelNode][0];
                        d->c.MyCoordsY -= d->c.escapePath[d->c.curTravelNode][1];
                        if(bf->c4){
                            d->c.evacuated = 0;
                        }
                        else{
                        }
                    }
                    break;
                case RELEASEHOME:
                    if(bf->c1){
                        tw_rand_reverse_unif(lp->rng);
                        tw_rand_reverse_unif(lp->rng);
                    }
                    break;
                case RELEASEWORK:
                    if(bf->c1){
                        tw_rand_reverse_unif(lp->rng);
                        tw_rand_reverse_unif(lp->rng);
                    }
                    break;
                case RELEASELEISURE:
                    if(bf->c1){
                        tw_rand_reverse_unif(lp->rng);
                        tw_rand_reverse_unif(lp->rng);
                    }
                    break;
                case RELEASEHOSPITAL:
                    if(bf->c1){
                        tw_rand_reverse_unif(lp->rng);
                        tw_rand_reverse_unif(lp->rng);
                    }
                    break;
                case UPDATEOCCUPANCY:
                    d->c.familyPresent=msg->InfoBlock1;
                    break;
                 
             }
        }
        case BUILDING:
        {
           int rand_result;
           tw_lpid dst_lp;
           tw_stime ts, ts1;
           tw_event *e, *e1;
           Msg_Data *m, *m1;
           FILE *fp;
           char buffer[50];
           char buffer2[100];
           MPI_Status status;
           tw_rand_reverse_unif(lp->rng);
           //tw_rand_reverse_unif(lp->rng);
           switch(msg->event_type)
             {
                case UPDATEDRAW:
                {
                    if(bf->c1){
                        if(bf->c2){
                            d->b.draw -= msg->InfoBlock1;
                        }
                        break;
                    }
                    break;
                }
                case UPDATEAVAILABLE:
                {
                    if(bf->c1){
                        if(bf->c3){
                        }
                        if(bf->c2){
                        }
                        if(bf->c4){
                        }
                        break;  
                    }
                    break;
                }
                case DAMAGE:
                {
                    int update = msg->InfoBlock1;
                    d->b.health += update;
                    if(bf->c1){
                    }
                    break;
                }
                case REPAIR:
                {
                    int update = msg->InfoBlock1;
                    d->b.health -= update;
                    break;
                }                
                case UPDATEOCCUPANCY:
                { 
                    int z = 0;
                    int q = 0;
                    int occupant = msg->InfoBlock1;
                    //If the person is present and telling us they are leaving, remove them
                    if(msg->InfoBlock2 > 0){
                        d->b.occupants[msg->InfoBlock2 - 1] = msg->InfoBlock1;
                        d->b.occupantPresent[msg->InfoBlock2 - 1] = 1;
                        int i = 0;
                        for(i;i<d->b.occupancy;i++){
                            tw_rand_reverse_unif(lp->rng);
                        }
                        d->b.occupancy++;
                    }
                    else if(msg->InfoBlock2 <= 0){
                        d->b.occupants[-1*(msg->InfoBlock2)] = -1;
                        d->b.occupantPresent[-1*(msg->InfoBlock2)] = 0;
                        int i = 0;
                        for(i;i<d->b.occupancy;i++){
                            tw_rand_reverse_unif(lp->rng);
                        }
                        d->b.occupancy--;
                    }
                    break;
                }
                case POWERON:
                {
                    if(bf->c1){
                        d->b.hasPower = 0;
                        int i = 0;
                        for(i;i<d->b.occupancy;i++){
                            tw_rand_reverse_unif(lp->rng);
                        }
                        break;
                    }
                    break;
                }
                case POWEROFF:
                {
                             
                //printf("Substation %llu TAKING THE FALL FOR THE MOTHERLAND\n",lp->gid);

                    if(bf->c1){
                        d->b.hasPower = 1;
                        int i = 0;
                        for(i;i<d->b.occupancy;i++){
                            tw_rand_reverse_unif(lp->rng);
                        }
                        break;
                    }
                }
            }
        }
        case POWERLINE:
        {
           int rand_result;
           tw_lpid dst_lp;
           tw_stime ts, ts1;
           tw_event *e, *e1;
           Msg_Data *m, *m1;
           FILE *fp;
           char buffer[50];
           char buffer2[100];
           MPI_Status status;
            tw_rand_reverse_unif(lp->rng);
            //tw_rand_reverse_unif(lp->rng);
           switch(msg->event_type)
             {
                case UPDATEDRAW:
                {
                    if(bf->c1){
                        int update = msg->InfoBlock1;
                        msg->InfoBlock1 = d->p.draw;
                        d->p.draw = update;
                    }
                    break;
                }
                case UPDATEAVAILABLE:
                {
                    if(bf->c1){
                        int update = msg->InfoBlock1;
                        msg->InfoBlock1 = d->p.available;
                        d->p.available = update;
                    }
                    break;
                }
                case DAMAGE:
                {
                    int update = msg->InfoBlock1;
                    d->p.health += update;
                    if(bf->c1){
                        d->p.available = msg->InfoBlock2;
                    }
                    break;
                }
                case REPAIR:
                {
                    int update = msg->InfoBlock1;
                    d->p.health -= update;
                    break;
                }
             }
        }
        case SUBSTATION:
        {
           int rand_result;
           tw_lpid dst_lp;
           tw_stime ts, ts1;
           tw_event *e, *e1;
           Msg_Data *m, *m1;
           FILE *fp;
           char buffer[50];
           char buffer2[100];
           MPI_Status status;
           tw_rand_reverse_unif(lp->rng);
           //tw_rand_reverse_unif(lp->rng);
           switch(msg->event_type)
             {
                case UPDATEDRAW:
                {
                    if(bf->c1){
                        int update = msg->InfoBlock1;
                        msg->InfoBlock1 = d->s.draw;
                        d->s.draw = update;
                        if(bf->c4){
                        }
                        else {
                        }
                    }
                    break;
                }
                case UPDATEAVAILABLE:
                {
                    if(bf->c1){
                        int update = msg->InfoBlock1;
                        msg->InfoBlock1 = d->s.available;
                        d->s.available = update;
                        int i = 0;
                        for (i; i < d->s.nextIDEnd; i++){
                            tw_rand_reverse_unif(lp->rng);
                        }
                    }
                    break;
                }
                case DAMAGE:
                {                       //printf("Substation %llu TAKING THE FALL FOR THE MOTHERLAND\n",lp->gid);

                    int update = msg->InfoBlock1;
                    d->s.health += update;
                    if(bf->c1){
                        d->s.available = msg->InfoBlock2;
                        int i = 0;
                        for (i; i < d->s.nextIDEnd; i++){
                            tw_rand_reverse_unif(lp->rng);
                        }
                    }
                    break;
                }
                case REPAIR:
                {
                    int update = msg->InfoBlock1;
                    d->s.health -= update;
                    break;
                }
             }
        }
        case GENERATOR:
        {
           int rand_result;
           tw_lpid dst_lp;
           tw_stime ts;
           tw_event *e;
           Msg_Data *m;
           FILE *fp;
           char buffer[50];
           char buffer2[100];
           MPI_Status status;
         //tw_rand_reverse_unif(lp->rng);
           switch(msg->event_type)
             {
                case UPDATEDRAW:
                {
                    if(bf->c1){
                        int update = msg->InfoBlock1;
                        msg->InfoBlock1 = d->g.draw;
                        d->g.draw = update;
                        if(bf->c2){
                        }
                    }
                    break;
                }
                case UPDATEAVAILABLE:
                {
                    if(bf->c1){
                        int update = msg->InfoBlock1;
                        msg->InfoBlock1 = d->g.available;
                        d->g.available = update;
                        int i = 0;
                        for(i; i < d->g.jumpArrayLength; i++){
                            //tw_rand_reverse_unif(lp->rng);
                        }
                        //tw_rand_reverse_unif(lp->rng);
                    }
                    break;
                }
                case DAMAGE:
                {
                    int update = msg->InfoBlock1;
                    d->g.health += update;
                    if(bf->c1){
                        d->g.available = msg->InfoBlock2;
                        int i = 0;
                        for(i; i < d->g.jumpArrayLength; i++){
                            //tw_rand_reverse_unif(lp->rng);
                        }
                    }
                    break;
                }
                case REPAIR:
                {
                    int update = msg->InfoBlock1;
                    d->g.health -= update;
                    break;
                }
             }
        }
   }
 }
} 
void
 final(Dissim_State * d, tw_lp * lp)
 {
         //wait_time_avg += ((s->waiting_time / (double) s->landings) / nlp_per_pe);
 }
  tw_lptype disaster_lps[] =
 {
         {
                 (init_f) init,
                 (pre_run_f) NULL,
                 (event_f) event_handler,
                 (revent_f) rc_event_handler,
                 (final_f) final,
                 (map_f) mapping,
                 sizeof(Dissim_State),
         },
         {0},
 };
 const tw_optdef app_opt [] =
 {
         TWOPT_GROUP("Civilian Model"),
         //TWOPT_UINT("nairports", nlp_per_pe, "initial # of airports(LPs)"),
         //TWOPT_UINT("nplanes", planes_per_airport, "initial # of planes per airport(events)"),
         //TWOPT_STIME("mean", mean_flight_time, "mean flight time for planes"),
         //TWOPT_UINT("memory", opt_mem, "optimistic memory"),
         TWOPT_END()
 };
 
 int
 main(int argc, char **argv, char **env)
 {
         int i;
 
         //tw_opt_add(app_opt);
         tw_init(&argc, &argv);
 
         nlp_per_pe /= (tw_nnodes() * g_tw_npe);
         g_tw_events_per_pe =(planes_per_airport * nlp_per_pe / g_tw_npe) + opt_mem;
 
         tw_define_lps(nlp_per_pe, sizeof(Msg_Data), 0);
 
         for(i = 0; i < g_tw_nlp; i++)
                 tw_lp_settype(i, &disaster_lps[0]);
 
         tw_run();
 
         if(tw_ismaster())
         {
         }
 
         tw_end();
         
         return 0;
 }