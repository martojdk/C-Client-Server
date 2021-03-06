#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>
#define NAME_MAX_LENGTH 100
#define ADD_REQUEST_CODE 1
#define KILL_REQUEST_CODE 2
#define SHMEM_SIZE 4096

typedef struct Person
{
    char name[NAME_MAX_LENGTH];
} sPerson;

typedef struct nodePerson
{
    sPerson person;
    struct nodePerson *leftChild;
    struct nodePerson *rightChild;
} sNodePerson;

void loadPersonsFromFile(char fileName[],sNodePerson **head);
void addPersonToTree(sPerson person,sNodePerson **head);
void addSomeonesChild(char parentName[], sPerson child,sNodePerson **head);
void killPerson(char name[],sNodePerson** head);
sNodePerson* findPerson(char name[],sNodePerson* head);
void* create_shared_memory(size_t size);

void printTree(sNodePerson **head);

void *listen(sNodePerson **head);
void *serve(sNodePerson **head);

void getInputAndRequestAddingSomeonesChild(sNodePerson **head);
void getInputAndRequestKillingSomeone(sNodePerson **head);

void writeRandomDataToFile(char fileName[]);

sem_t sem;
void *shmem;
int main()
{
    pthread_t listener;
    pthread_t server;

    if (sem_init(&sem, 0, 1))
    {
        perror("sem_init_failed!");
    } 
    else
    {
        sNodePerson *head;
        head=NULL;
        // writeRandomDataToFile("data.bin");
        loadPersonsFromFile("data.bin",&head);
        shmem = create_shared_memory(SHMEM_SIZE);

        if(pthread_create(&listener, NULL, listen, &head) != 0)
        {
            printf("Could not create listener\n");
            return 1;
        }
        if(pthread_create(&server, NULL, serve, &head) != 0)
        {
            printf("Could not create server\n");
            return 2;
        }

        if(pthread_join(server, NULL) != 0)
        {
            printf("Could not join threads\n");
            return 3;
        }
        if(pthread_join(listener, NULL) != 0)
        {
            printf("Could not join threads\n");
            return 3;
        }

        if(sem_destroy(&sem) != 0)
        {
            printf("Could not destroy semaphore\n");
        }
        pthread_exit(NULL);
    }
    return 0;
}

void writeRandomDataToFile(char fileName[])
{
    int fd = open(fileName,O_CREAT | O_WRONLY);
    if(fd < 0) 
    {
        printf("Couldnt open file");
    }
    if(chmod(fileName,S_IRUSR | S_IWUSR) != 0)
    {
        printf("Couldnt change file %s's mode\n", fileName);
    }
    sPerson p1;
    strcpy(p1.name,"Max");
    sPerson p2;
    strcpy(p2.name,"Dustin");
    sPerson p3;
    strcpy(p3.name,"Tony");

    if(write(fd, &p1, sizeof(p1)) != sizeof(p1))
    {
        printf("Couldnt write to file\n");
    }
    if(write(fd, &p2, sizeof(p2)) != sizeof(p2))
    {
        printf("Couldnt write to file\n");
    }

    if(write(fd, &p3, sizeof(p3)) != sizeof(p2))
    {
        printf("Couldnt write to file\n");
    }
    if(close(fd) != 0)
    {
        printf("Couldnt close file %s\n", fileName);
    }
}

void *listen(sNodePerson **head)
{
    printf("Hi. This is your family application. When you started this application a tree with some folks was loaded.\n");
    printf("You can add child to already existing person and you can kill whoever you like.The choice is yours.\n");
    printf("*************************************\n\n");
    while(1)
    {
        if(sem_wait(&sem) != 0)
        {
            printf("Could not decrement semaphore\n");
        }
        printf("0.Show all people\n");
        printf("1.Add someone's child\n");
        printf("2.Kill someone\n");
        printf("3.Exit\n");
        int choice;
        if(scanf("%d",&choice)!=1)
        {
            printf("Exiting now..\n");
            exit(-1);
        }
        switch(choice)
        {
            case 0:
                printTree(head);
                break;
            case 1:
                getInputAndRequestAddingSomeonesChild(head);
                break;
            case 2:
                getInputAndRequestKillingSomeone(head);
                break;
            case 3:
                exit(0);
                break;
            default:
                printf("Wrong input.Exiting now.\n");
                exit(-2);
        }
        if(sem_post(&sem) != 0)
        {
            printf("Could not increment semaphore\n");
        } 
        if(sleep(3) != 0)
        {
            printf("Sleep was interrupted\n");
        }

    }
}

void *serve(sNodePerson **head)
{

    while(1)
    {
        if(sem_wait(&sem) != 0)
        {
            printf("Could not decrement semaphore\n");
        }
        if(strlen((char*)shmem) != 0)
        {
            char *request;
            request = strtok(shmem," ");
            switch(request[0])
            {
                case '1' :
                {
                    char * parentName;
                    request = strtok(NULL," ");
                    parentName = request;
                    char * childName;
                    request = strtok(NULL," ");
                    childName = request;
                    sPerson child;
                    strcpy(child.name,childName);
                    addSomeonesChild(parentName,child,head);
                    break;
                }
                case '2':
                {
                    request = strtok(NULL," ");
                    killPerson(request,head);
                    printf("********* Tree looks like this now ***********\n");
                    printTree(head);
                    printf("**********************************************\n");
                    break;
                }
                default:
                {
                    printf("Some garbage is written in the shmem\n");
                    exit(3);
                    break;
                }
            }
        }
        memset(shmem, '\0', SHMEM_SIZE);
        if(sem_post(&sem) != 0)
        {
            printf("Couldnt increment semaphore\n");
        } 
        if(sched_yield() != 0)
        {
            printf("Couldnt sleep thread\n");
        }
    }

}

void getInputAndRequestKillingSomeone(sNodePerson **head)
{
    char name[NAME_MAX_LENGTH];
    printf("Insert the name of the one you want to kill down below:\n");
    if(scanf("%s",name) != 1)
    {
        printf("Could not read name\n");
    }
    char buffer[SHMEM_SIZE];
    if(snprintf(buffer,sizeof(buffer),"%d %s",KILL_REQUEST_CODE,name) != -1)
    {
        printf("Could not create kill request\n");
    }
    memcpy(shmem,buffer,sizeof(buffer));
}

void getInputAndRequestAddingSomeonesChild(sNodePerson **head)
{
    char parentName[NAME_MAX_LENGTH];
    printf("Insert the name of the parent here:\n");
    if(scanf("%s",parentName) != 1)
    {
        printf("Could not get parent's name\n");
    }
    char childName[NAME_MAX_LENGTH];
    printf("Insert the name of the child here:\n");
    if(scanf("%s",childName) != 1)
    {
        printf("Could not get child's name\n");
    }
    char buffer[SHMEM_SIZE];
    sprintf(buffer,"%d %s %s",ADD_REQUEST_CODE,parentName,childName);
    memcpy(shmem,buffer,sizeof(buffer));
}

void* create_shared_memory(size_t size)
{
  int protection = PROT_READ | PROT_WRITE;
  int visibility = MAP_SHARED | MAP_ANONYMOUS;
  return mmap(NULL, size, protection, visibility, 0, 0);
}

void printTree(sNodePerson **head)
{
    if (*head == NULL)
    {
        return;
    }
    printTree(&(*head)->leftChild);
    if(*head != NULL)
    {
        printf("%s\n",&(*head)->person.name);
    }
    printTree(&(*head)->rightChild);
}

void loadPersonsFromFile(char filename[],sNodePerson **head)
{
    int fd = open(filename,O_RDWR);
    if(fd < 0)
    {
        printf("Could not open file %s\n",filename);
    }
    int totalRead = 0;
    lseek(fd,0,SEEK_SET);
    while(1)
    {
        sPerson person;
        char buffer[100];
        if(read(fd,buffer,sizeof(buffer)) == sizeof(buffer))
        {
            strcpy(person.name,buffer);
            addPersonToTree(person,head);
        } else {
            break;
        }
    }
    if(close(fd) != 0)
    {
        printf("Couldnt close file %s\n", filename);
    }
}

void addPersonToTree(sPerson person,sNodePerson **head)
{
    if(*head == NULL)
    {
        sNodePerson *head1 = (sNodePerson*)malloc(sizeof(sNodePerson));
        *head = head1;
        head1->leftChild = NULL;
        head1->rightChild = NULL;
        head1->person = person;
    } 
    else 
    {

        if(strcmp(&(*head)->person.name, person.name) == 0 || strcmp(&(*head)->person.name, person.name) == -1)
        {
            addPersonToTree(person,&(*head)->leftChild);
        } 
        else 
        {
            addPersonToTree(person,&(*head)->rightChild);
        }

    }
}

void addSomeonesChild(char parentName[], sPerson child,sNodePerson **head)
{
    sNodePerson* foundPerson = findPerson(parentName,*head);
    if(foundPerson == NULL)
    {
       printf("Could not find person because they do not exist\n");
       return;
    }
    addPersonToTree(child,&foundPerson);
    printf("********* Tree looks like this now ***********\n");
    printTree(head);
    printf("**********************************************\n");
}

void killPerson(char name[],sNodePerson **head)
{
    sNodePerson* foundPerson = findPerson(name,*head);
    if(foundPerson == NULL)
    {
        printf("Could not find person because they do not exist\n");
        return;
    }
    strcpy(foundPerson->person.name,"X\0");
}

sNodePerson* findPerson(char name[],sNodePerson* head)
{
    if(head == NULL)
    {
        return head;
    } 
    else 
    {
        if(strcmp(head->person.name,name) == 0)
        {
            return head;
        } 
        else if(strcmp(head->person.name, name) == -1)
        {
            return findPerson(name,head->leftChild);
        } 
        else 
        {
            return findPerson(name,head->rightChild);
        }
    }

}
