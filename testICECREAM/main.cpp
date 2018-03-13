//
//  main.cpp
//  testICECREAM
//
//  Created by tombp on 13/03/2018.
//  Copyright © 2018 tombp. All rights reserved.
//

#include <iostream>
#include <stdio.h>
#include <semaphore.h>
#include <unistd.h>

#define NUM_CUSTOMERS 10
#define SECOND 1000000
static void* Cashier(void*); //void Cashier(void);
static void *Clerk(void* done);//void Clerk(sem_t* done);
static void *Manager(void* totalNeeded); //void Manager(int totalNeeded);
static void *Customer(void* numToBuy);  //void Customer(int numToBuy);
static void SetupSemaphores(void);
static void FreeSemaphores(void);
static int RandomInteger(int low,int high);
static void MakeCone(void);
static bool InspectCone(void);
static void Checkout(int linePosition);
static void Browse(void);

char sem_request[] = "Sem Inspection Requested";
char sem_finished[] = "Sem Inspection Finished";
char sem_available[] = "Sem Inspection Available";
char sem_customerready[] = "Sem Customer Ready";
char sem_lock[] = "Sem Line Lock";
char sem_customers[NUM_CUSTOMERS][50];

struct inspection {
    sem_t *available;
    sem_t *requested;
    sem_t *finished;
    bool passed;
} inspection;
struct line {
    sem_t *lock;
    int nextPlaceInLine;
    sem_t *customers[NUM_CUSTOMERS];
    sem_t *customerReady;
} line;

int main(int argc, const char * argv[]) {
    // insert code here...
//    std::cout << "Hello, World!\n";
    
    for (int j = 0; j < NUM_CUSTOMERS; j++) {
        sprintf(sem_customers[j], "Sem Customer %d in Line",j);
    }
    
    
    pthread_t pth_customer[NUM_CUSTOMERS];
    pthread_t pth_cashier;
    pthread_t pth_manager;
    
    
    int i, numCones,totalCones = 0;
    int arr_numCones[NUM_CUSTOMERS];
    SetupSemaphores();
    for (i = 0; i < NUM_CUSTOMERS; i++) {
        char name[32];
        sprintf(name, "Customer %d",i);
        numCones = RandomInteger(1, 4);
        arr_numCones[i]= numCones;
        pthread_create(&pth_customer[i], NULL, Customer, (void*)&arr_numCones[i]);
        totalCones+= numCones;
    }
    printf("Total Cone Num:%d\n",totalCones);
    pthread_create(&pth_cashier, NULL, Cashier, NULL);
    pthread_create(&pth_manager, NULL, Manager, (void*)&totalCones);
    
    
//    sleep(20);
    pthread_join(pth_cashier, NULL);
    pthread_join(pth_manager, NULL);
    printf("All done!\n");
    FreeSemaphores();
    return 0;
}

static void* Manager(void *totalNeeded)
{
    int l_totalNeeded = *(int*)totalNeeded;
    int numPerfect = 0, numInspections = 0;
    while (numPerfect < l_totalNeeded) {
        sem_wait(inspection.requested);
        inspection.passed = InspectCone();
        numInspections++;
        if (inspection.passed)
            numPerfect++;
        sem_post(inspection.finished);
    }
    printf("Inspection success rate %d%%\n", (100*numPerfect)/numInspections);
    return NULL;
}
static void *Clerk(void *done)
{
    sem_t *l_done = (sem_t*)done;
    bool passed = false;
    while (!passed) {
        MakeCone();
        sem_wait(inspection.available);
        sem_post(inspection.requested);
        sem_wait(inspection.finished);
        passed = inspection.passed;
        sem_post(inspection.available);
    }
    sem_post(l_done);
    return NULL;
}
static void* Customer(void* numConesWanted)
{
    
    int l_numConesWanted = *(int*)numConesWanted;
    char sem_name[l_numConesWanted][50] ;
    pthread_t pth_clerk[l_numConesWanted];
    sem_t* sem_clerk[l_numConesWanted];
    int i, myPlace;
    for (i = 0; i < l_numConesWanted; i++) {
        sprintf(sem_name[i], "sem_clerk_%d",i);
        sem_open(sem_name[i], O_CREAT, 0777, 0);
        pthread_create(&pth_clerk[i], NULL, Clerk, (void*)sem_clerk[i]);
    }
    Browse();
    
    for (i = 0;  i < l_numConesWanted; i++) {
        sem_wait(sem_clerk[i]);
    }
    for (i = 0; i < l_numConesWanted; i++) {
        sem_unlink(sem_name[i]);
    }
    sem_wait(line.lock);
    myPlace = line.nextPlaceInLine++;
    sem_post(line.lock);
    sem_post(line.customerReady);
    sem_wait(line.customers[myPlace]);
    printf("Customer %d done!\n",i);
    return NULL;
}

static void * Cashier(void* arg)
{
    int i;
    for (i = 0; i < NUM_CUSTOMERS; i++) {
        sem_wait(line.customerReady);
        Checkout(i);
        sem_post(line.customers[i]);
    }
    return NULL;
}
static void SetupSemaphores(void)
{
    int i;
    char sem_request[] = "Sem Inspection Requested";
    char sem_finished[] = "Sem Inspection Finished";
    char sem_available[] = "Sem Inspection Available";
    char sem_customerready[] = "Sem Customer Ready";
    char sem_lock[] = "Sem Line Lock";
    char sem_customers[NUM_CUSTOMERS][50];
    inspection.requested = sem_open(sem_request, O_CREAT, 0777, 0);
    inspection.finished = sem_open(sem_finished, O_CREAT, 0777, 0);
    inspection.available = sem_open(sem_available, O_CREAT, 0777, 1);
    inspection.passed = false;
    line.customerReady = sem_open(sem_customerready, O_CREAT, 0777, 0);
    line.lock = sem_open(sem_lock, O_CREAT, 0777, 1);
    line.nextPlaceInLine = 0;
    for (i = 0;  i < NUM_CUSTOMERS; i++) {
        line.customers[i] = sem_open(sem_customers[i], O_CREAT, 0777, 0);
    }
}
static void FreeSemaphores(void)
{
    int i;
    sem_unlink(sem_request);
    sem_unlink(sem_finished);
    sem_unlink(sem_available);
    sem_unlink(sem_customerready);
    sem_unlink(sem_lock);
    for (i = 0; i < NUM_CUSTOMERS; i++) {
        sem_unlink(sem_customers[i]);
    }
}
static void MakeCone(void)
{
    usleep(RandomInteger(0, 3*SECOND));
    printf("making an ice cream cone. \n");
}
static bool InspectCone(void)
{
    bool passed = (RandomInteger(1, 2)==1);
    printf("examining cone ,did it pass? %c\n",(passed? 'Y':'N'));
    usleep(RandomInteger(0, 0.5*SECOND));
    return passed;
}
static void Checkout(int linePosition)
{
    printf("checking out customer in line at postion #%d .\n",linePosition);
    usleep(RandomInteger(0, SECOND));
}
static void Browse(void)
{
    usleep(RandomInteger(0, 5*SECOND));
    printf("browsing .\n");
}
static int RandomInteger(int low,int high)
{
    int result= low;
    result += rand()%(high+1-low);
    return result;
}






























