#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdbool.h>
#include <unistd.h>
#include <ctype.h>

void * spellChecker(void *arg);

typedef struct {
    char word[1023];
    int count;
} Mistake;

typedef struct{
    Mistake top3[3];
    char filename[50];
    int totalErrors;
}Message;

typedef struct messageNode{
    Message msg;
    struct messageNode * next;
}messageNode;

typedef struct messageQueue{
    messageNode * head;
    messageNode * tail;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
}messageQueue;


typedef struct {
    FILE * fileptr;
    FILE * dictptr;
    char filename[50];
    messageQueue *q;
    Mistake **mistakearr;
} ThreadArgs;


messageQueue * createMessageQueue(){
    messageQueue* q = (messageQueue*)malloc(sizeof(messageQueue));
    q->head = q->tail = NULL;
    pthread_mutex_init(&q->mutex, NULL);
    pthread_cond_init(&q->cond, NULL);
    return q;
}

void sendMessage(messageQueue* q, char filename[50], Mistake top3[3],int totalErrors, Mistake *** mistakearr, int arrCounter2){
    messageNode* node = (messageNode*)malloc(sizeof(messageNode));
    strcpy(node->msg.filename,filename);
    node->msg.totalErrors= totalErrors;
    for(int i = 0; i < 3; i++){
        strcpy(node->msg.top3[i].word, top3[i].word);
        node->msg.top3->count = top3[i].count;
    }
    node->next = NULL;

    pthread_mutex_lock(&q->mutex);
    if(q->tail != NULL){
        q->tail->next = node;
        q->tail = node;
    }
    else{
        q->tail = q->head = node;
    }
    pthread_cond_signal(&q->cond);
    pthread_mutex_unlock(&q->mutex);
    sleep(5);
}

int getMessage(messageQueue *q, Message * msg_out, FILE * outputFile, int *errors){
    int success = 0;
   
    pthread_mutex_lock(&q->mutex);

    while(q->head == NULL){
        pthread_cond_wait(&q->cond, &q->mutex);
    }
    messageNode * oldHead = q->head;
    *msg_out = oldHead->msg;
    q->head = oldHead->next;
    if (q->head == NULL){
        q->tail = NULL;
    }
    free(oldHead);

    fprintf(outputFile, "%s ", msg_out->filename);
    fprintf(outputFile, "%d ", msg_out->totalErrors);
    for(int i = 0; i < 3; i++){
        fprintf(outputFile, "%s ", msg_out->top3[i].word);
    }
    fprintf(outputFile, "%s", "\n");
    success = 1;
    *errors += msg_out->totalErrors;


    pthread_mutex_unlock(&q->mutex);
    return success;
}

void printmenu(void){
    printf("\nMain Menu\n\n");
    printf("1.Start A New Spellchecking Task\n");
    printf("2.Exit\n");
    printf("Please enter an option: ");
}

Mistake ** createMistakearr(){
    Mistake ** q = (Mistake**)malloc(10 * sizeof(Mistake*));
    for(int i = 0; i < 10; i++){
        q[i] = (Mistake*)malloc(sizeof(Mistake));
        q[i]->count = 0;
    }
    return q;
}



int main(int argc, char* argv[]){
    int userChoice = 0;
    int numThreads = 0;
    int check;
    int check2;
    char dictname[40];
    int fileprocessed = 0;
    int totalErrors = 0;
    int lenofarray = 10;
    int counter = 0;
    int checkarr[3] = {0,0,0};
    int filesRunning = 0;
    Mistake top3[3];
    pthread_t tid;
    ThreadArgs args;
    messageQueue *q = createMessageQueue();
    Mistake ** mistakearr = createMistakearr();
    Mistake ** mistakearr2 = createMistakearr();
    FILE * fp = fopen("tottenj_A2.out", "w");

    for(int i = 0 ; i < 3; i++){
        top3[i].count = 0;
    }

    while(true){
        printmenu();
        lenofarray = 10;
        scanf("%d", &userChoice);
        scanf("%*[^\n]%*1[\n]");
  
        switch (userChoice){
        case 1:
            check = 0;
            check2 = 0;

            while (check == 0)
            {
                check = 1;
                printf("\nPlease enter name of dictionary file (Enter quit to go back): ");
                scanf("%s", dictname);
                if(strcmp(dictname, "quit") == 0){
                    check2 = 1;
                    break;
                }
                printf("\nPlease enter name of input text file (Enter q to go back): ");
                scanf("%s", args.filename);
                if(strcmp(args.filename, "quit") == 0){
                    check2 = 1;
                    break;
                }

                args.dictptr = fopen(dictname, "r");
                if(args.dictptr == NULL){
                    printf("File error");
                    check = 0;
                }

                args.fileptr = fopen(args.filename, "r");
                if(args.fileptr == NULL){
                    printf("File Error");
                    check = 0;
                }
            }
            if(check2 != 1){
                args.q = q;
                args.mistakearr = mistakearr;
                pthread_create(&tid, NULL,spellChecker, &args);
                filesRunning +=1;
                numThreads +=1;

                Message msg;

                while(fileprocessed < numThreads){
                    if(getMessage(q, &msg,fp, &totalErrors)){
                       
                        filesRunning -= 1;
                        fileprocessed +=1;

                        if(counter >= lenofarray - 3){
                            lenofarray *=2;
                            mistakearr2 = realloc(mistakearr2, lenofarray * sizeof(Mistake*));
                            for(int i = counter; i < lenofarray; i++){
                                mistakearr2[i] = (Mistake*)malloc(sizeof(Mistake));
                                mistakearr2[i]->count = 0;
                            }
                        }
                        checkarr[0] = 0;
                        checkarr[1] = 0;
                        checkarr[2] = 0;
                        for(int i = 0; i< counter; i++){
                            for(int j = 0; j<3; j++){
                                if(strcmp(mistakearr2[i]->word, msg.top3[j].word) == 0){
                                    mistakearr2[i]->count += 1;
                                    checkarr[j] = 1;
                                }

                            }
                        }


                        for(int i = 0; i<3; i++){
                            if(checkarr[i] != 1){
                                strcpy(mistakearr2[counter]->word,msg.top3[i].word);
                                mistakearr2[counter]->count = 1;
                                counter += 1;
                            }
                        }
                    }
                }
                break;
            }
            else{
                break;
            }

        case 2:
            printf("\n");
            free(q);
            if(filesRunning > 0){
                printf("%d Threads still running\n", filesRunning);
            }
            else{
                printf("No Threads Running\n");
            }

            
            for(int i =0; i< counter; i++){
            if(mistakearr2[i]->count > top3[0].count){
                top3[0].count = mistakearr2[i]->count;
                strcpy(top3[0].word,mistakearr2[i]->word);
            }
            else if(mistakearr2[i]->count > top3[1].count){
                top3[1].count = mistakearr2[i]->count;
                strcpy(top3[1].word,mistakearr2[i]->word);
            }
            else if(mistakearr2[i]->count > top3[2].count){
                top3[2].count = mistakearr2[i]->count;
                strcpy(top3[2].word,mistakearr2[i]->word);
            }
            }
            for(int i = 0; i< counter; i++){
                free(mistakearr2[i]);
            }
            free(mistakearr2);
            

            

            //Summary with -l
            if(argc >=2){
                if(strcmp(argv[1], "-l") == 0){
                fprintf(fp,"\nNumber of Files Processed: %d\n",fileprocessed);
                fprintf(fp, "Number of Spelling Errors: %d\n", totalErrors);
                fprintf(fp,"Three most common mispelllings: ");
                for(int i = 0; i<3; i++){
                    fprintf(fp, "%s(%d times) ",top3[i].word, top3[i].count);
                }
                }
              
            }
            //Summary without -l
            else{
                printf("Number of Files Processed: %d\n", fileprocessed);
                printf("Number of Spelling Errors: %d\n", totalErrors);
                printf("Three most common mispellings: ");
                for(int i = 0; i <3; i++){
                    printf("%s (%d times) ", top3[i].word, top3[i].count);
                }
                printf("\n");
            }
            
            fclose(fp);
            return 0;

        default:
            printf("\nPlease choose either 1 or 2\n");
            fflush(stdin);
            break;
            }
        }
    }
    



void * spellChecker(void *arg){
    ThreadArgs* args = (ThreadArgs*)arg;
    char x[1024];
    char y[1024];
    int arrCounter = 10;
    int arrCounter2 = 0;
    int check = 0;
    int check2 = 0;
    
    //Initialize mistake array with 10
    Mistake **mistakearr = (Mistake**)malloc(arrCounter * sizeof(Mistake*));
    for(int i = 0; i < arrCounter; i++){
        mistakearr[i] = (Mistake*)malloc(sizeof(Mistake));
        mistakearr[i]->count = 0;
    }

   //Check every word in text file
    while (fscanf(args->fileptr, " %1023s", y) != EOF)
    {
        for(int i = 0; i < strlen(y); i++){
            y[i] = tolower(y[i]);
        }
      
        
        //Every word in dictionary
        while(fscanf(args->dictptr, " %1023s", x) != EOF){
            for(int i = 0; i < strlen(x); i++){
                x[i] = tolower(x[i]);
            }
            
            //Found word in dict
            if(strcmp(y, x) == 0){
                check = 1;
                break;
            }
        }
        rewind(args->dictptr);
        //mistake found
        if(check == 0){
           
            if(arrCounter2 == arrCounter){
                
                arrCounter *= 2;
                mistakearr = (Mistake**)realloc(mistakearr, arrCounter * sizeof(Mistake*));
                for(int i = arrCounter2; i < arrCounter; i++){
                    mistakearr[i] = (Mistake*)malloc(sizeof(Mistake));
                    mistakearr[i]->count = 0;
                }
            }
            
            for(int i = 0; i < arrCounter2; i++){
                if(strcmp(y,mistakearr[i]->word) == 0){
                    mistakearr[i]->count += 1;
                    check2 = 1;
                    break;
                }
            }
           
            if(check2 == 0){
                
                strcpy(mistakearr[arrCounter2]->word,y);
                mistakearr[arrCounter2]->count += 1;
                arrCounter2 +=1;
            }            

        }
       
        check = 0;
        check2 = 0;
    }
    fclose(args->fileptr);
    fclose(args->dictptr);

    Mistake top3[3];
    for(int i =0; i < 3; i++){
        top3[i].count = 0;
    }
    int totalErrors = 0;

    for(int i =0; i< arrCounter2; i++){
        totalErrors += mistakearr[i]->count;
        if(mistakearr[i]->count > top3[0].count){
            top3[0].count = mistakearr[i]->count;
            strcpy(top3[0].word,mistakearr[i]->word);
        }
        else if(mistakearr[i]->count > top3[1].count){
            top3[1].count = mistakearr[i]->count;
            strcpy(top3[1].word,mistakearr[i]->word);
        }
        else if(mistakearr[i]->count > top3[2].count){
            top3[2].count = mistakearr[i]->count;
            strcpy(top3[2].word,mistakearr[i]->word);
        }
    }


    sendMessage(args->q, args->filename, top3, totalErrors, &args->mistakearr, arrCounter2);
    for(int i = 0; i < arrCounter2; i++){
        free(mistakearr[i]);
    }
    free(mistakearr);
    return NULL;
}


