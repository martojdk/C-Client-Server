#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>
#define NAME_MAX_LENGTH 100
#define ADD_REQUEST_CODE 1
#define KILL_REQUEST_CODE 2
#define SHMEM_SIZE 202

typedef struct Person {
    char name[NAME_MAX_LENGTH];
} sPerson;

typedef struct nodePerson {
    sPerson person;
    struct nodePerson *leftChild;
    struct nodePerson *rightChild;
} sNodePerson;

void loadPersonsFromFile(char fileName[],sNodePerson **head);
void addPersonToTree(sPerson person,sNodePerson **head);
void addSomeonesChild(char parentName[], sPerson child,sNodePerson **head);
void killPerson(char name[],sNodePerson* head);
sNodePerson* findPerson(char name[],sNodePerson* head);
void* create_shared_memory(size_t size);

/* void writePersonsToFile(char filename[], sPerson persons[], int size); */
void printTree(sNodePerson **head);

void listen(sNodePerson **head,void *shmem);
void serve(sNodePerson **head,void *shmem);

void getInputAndRequestAddingSomeonesChild(sNodePerson **head,void *shmem);
void getInputAndRequestKillingSomeone(sNodePerson **head,void *shmem);
int main()
{
    // add memset
    sNodePerson *head;
    head=NULL;
    loadPersonsFromFile("data.bin",&head);
    void* shmem = create_shared_memory(1024); // for two names and a number with a space character
    listen(&head,shmem);
    return 0;
}

void serve(sNodePerson **head,void *shmem){

    while(1){

        if(strlen((char*)shmem) != 0){
            char *requestCode;
            requestCode = strtok(shmem," ");
            switch(requestCode[0]){
                case '1' :
                {
                    char * parentName;
                    parentName = strtok(shmem," ");
                    char * childName;
                    childName = strtok(shmem," ");
                    sPerson child;
                    strcpy(child.name,childName);
                    addSomeonesChild(parentName,child,head);
                    break;
                }
                case '2':
                {
                    killPerson(strtok(shmem," "),head);
                    printTree(head);
                    break;
                }
                default:
                {
                    printf("Some garbage is written in the shmem");
                    exit(3);
                    break;
                }
            }
        }

    }

}

void getInputAndRequestKillingSomeone(sNodePerson **head,void *shmem){
    char name[NAME_MAX_LENGTH];
    printf("Insert the name of the one you want to kill down below:\n");
    scanf("%s",name);
    char buffer[SHMEM_SIZE];
    char choice[] = {"2"};
    snprintf(buffer,sizeof(buffer),"%s %s",choice,name);
    memcpy(shmem,buffer,sizeof(buffer));
}

void getInputAndRequestAddingSomeonesChild(sNodePerson **head,void *shmem){
    char parentName[NAME_MAX_LENGTH];
    printf("Insert the name of the parent here:\n");
    scanf("%s",parentName);
    char childName[NAME_MAX_LENGTH];
    printf("Insert the name of the child here:\n");
    scanf("%s",childName);
    char buffer[202];
    char choice[] = {"1"};
    sprintf(buffer,"%s %s %s",choice,parentName,childName);
    printf("%s\n",buffer);
    memcpy(shmem,buffer,sizeof(buffer));
}

void listen(sNodePerson **head,void *shmem){
    printf("Hi. This is your family application. When you started this application a tree with some folks was loaded.\n");
    printf("You can add child to already existing person and you can kill whoever you like.The choice is yours.\n");
    printf("*************************************\n\n");
    while(1){
        printf("0.Show all people\n");
        printf("1.Add someone's child\n");
        printf("2.Kill someone\n");
        printf("3.Exit\n");
        int choice;
        if(scanf("%d",&choice)!=1){
            printf("Exiting now..");
            exit(-1);
        }
        switch(choice){
            case 0:
                printTree(head);
                break;
            case 1:
                getInputAndRequestAddingSomeonesChild(head,shmem);
                break;
            case 2:
                getInputAndRequestKillingSomeone(head,shmem);
                break;
            case 3:
                exit(0);
                break;
            default:
                printf("Wrong input.Exiting now.");
                exit(-2);

        }
    }
}

void* create_shared_memory(size_t size) {
  int protection = PROT_READ | PROT_WRITE;
  int visibility = MAP_SHARED | MAP_ANONYMOUS;
  return mmap(NULL, size, protection, visibility, 0, 0);
}

void printTree(sNodePerson **head){
    if (*head == NULL){
        return;
    }
    printTree(&(*head)->leftChild);
    if(*head != NULL){
        printf("%s\n",&(*head)->person.name);
    }
    printTree(&(*head)->rightChild);
}
/*
void writePersonsToFile(char filename[], sPerson persons[], int size){
     FILE *fp = fopen(filename,"w+");
     if(!fp){
        printf("Could not open file %s\n",filename);
    }
    for(int i=0;i<size;i++){
        if(fwrite(&persons[i],sizeof(sPerson),1,fp)!=1)
        {
            printf("Error: Code 4\n");
            return ;
        }
    }
    fclose(fp);
}
*/
void loadPersonsFromFile(char filename[],sNodePerson **head){
    FILE *fp = fopen(filename,"rb");
    if(!fp){
        printf("Could not open file %s\n",filename);
    }
    while(1){
        sPerson person;

        if(fread(&person,sizeof(sPerson),1,fp)!= 1){
            break;
        }
        addPersonToTree(person,head);
    }
    fclose(fp);
}

void addPersonToTree(sPerson person,sNodePerson **head){
    if(*head == NULL){
        sNodePerson *head1 = (sNodePerson*)malloc(sizeof(sNodePerson));
        *head = head1;
        head1->leftChild = NULL;
        head1->rightChild = NULL;
        head1->person = person;
    } else {

        if(strcmp(&(*head)->person.name, person.name) == 0 || strcmp(&(*head)->person.name, person.name) == -1){
            addPersonToTree(person,&(*head)->leftChild);
        } else {
            addPersonToTree(person,&(*head)->rightChild);
        }

    }
}

void addSomeonesChild(char parentName[], sPerson child,sNodePerson **head){
    sNodePerson* foundPerson = findPerson(parentName,*head);
    if(foundPerson == NULL){
       printf("Could not find person because they do not exist\n");
    }
    addPersonToTree(child,&foundPerson);
    printTree(head);
}

void killPerson(char name[],sNodePerson *head){
    sNodePerson* foundPerson = findPerson(name,head);
    if(foundPerson == NULL){
        printf("Could not find person because they do not exist\n");
    }
    strcpy(foundPerson->person.name,"X\0");
    printTree(&head);
}

sNodePerson* findPerson(char name[],sNodePerson* head){
    if(head == NULL){
        return head;
    } else {
        if(strcmp(head->person.name,name) == 0){
            return head;
        } else if(strcmp(head->person.name, name) == -1){
            return findPerson(name,head->leftChild);
        } else {
            return findPerson(name,head->rightChild);
        }
    }

}
