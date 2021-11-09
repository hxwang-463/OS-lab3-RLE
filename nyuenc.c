#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#define handle_error(msg) do { perror(msg); exit(EXIT_FAILURE); } while (0)

typedef struct link{
    int index;
    char *content;
    int length;
    struct link *next;
}Link;
Link *head_work = NULL;
Link *tail_work = NULL;
Link *head_result = NULL;

int not_end = 1;
pthread_mutex_t mutex_work = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_result = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t work_not_empty = PTHREAD_COND_INITIALIZER;
pthread_cond_t result_not_empty = PTHREAD_COND_INITIALIZER;


void *thread_work(void *name){
    do{
        pthread_mutex_lock(&mutex_work);
        while(head_work->next==NULL)pthread_cond_wait(&work_not_empty, &mutex_work);
        Link* work = head_work->next;
        if(head_work->next!=tail_work){
            head_work->next = work->next;
        }
        else{
            tail_work = head_work;
            head_work->next = NULL;
        }
        pthread_mutex_unlock(&mutex_work);
        if(work->index==-1)break;
        char *result = (char*)malloc((2*work->length+1)*sizeof(char));
        char *p = work->content;
        char *q = result;
        int pre = *p;
        *q++ = pre;
        int count = 0;
        int i;
        for(i=0;i<work->length;i++){
            if(*p!=pre){
                *q++ = count;
                count = 1;
                pre = *p;
                *q++ = pre;
            }
            else count++;
            p++;   
        }
        *q++ = count;
        *q = 0;
        Link *one_result = (Link*)malloc(sizeof(Link));
        one_result->index = work->index;
        one_result->content = (char*)malloc((strlen(result)+1)*sizeof(char));
        strcpy(one_result->content, result);
        pthread_mutex_lock(&mutex_result);
        one_result->next = head_result;
        head_result = one_result;
        pthread_cond_signal(&result_not_empty);
        pthread_mutex_unlock(&mutex_result);
    }while(1);
    pthread_exit(NULL);
}


void *thread_result(void *name){
    char reserve[2];
    int i=0;
    Link *head = NULL;
    Link *temp;
    while(1){
        pthread_mutex_lock(&mutex_result);
        while(head_result == NULL && not_end)
            pthread_cond_wait(&result_not_empty, &mutex_result);
        if(head_result == NULL) break;
        Link *one_result = head_result;
        head_result = head_result->next;
        pthread_mutex_unlock(&mutex_result);
        if(i==one_result->index){
            if(i==0){
                reserve[0] = one_result->content[0];
                reserve[1] = 0;
            }   
            if(one_result->content[0]==reserve[0]){
                one_result->content[1] = one_result->content[1] + reserve[1];
            }
            else{
                printf("%c", reserve[0]);
                printf("%c", reserve[1]);
            }
            reserve[0] = one_result->content[strlen(one_result->content)-2];
            reserve[1] = one_result->content[strlen(one_result->content)-1];
            one_result->content[strlen(one_result->content)-2] = 0;
            if(one_result->content[0]!=0)
                printf("%s", one_result->content);
            free(one_result->content);
            free(one_result);
            i++;
            while(head && head->index==i){
                if(head->content[0]==reserve[0]){
                    head->content[1] = head->content[1] + reserve[1];
                }
                else{
                    printf("%c", reserve[0]);
                    printf("%c", reserve[1]);
                }
                reserve[0] = head->content[strlen(head->content)-2];
                reserve[1] = head->content[strlen(head->content)-1];
                head->content[strlen(head->content)-2] = 0;
                if(head->content[0]!=0)
                    printf("%s", head->content);
                fflush(stdout);
                temp=head;
                head=head->next;
                free(temp->content);
                free(temp);
                i++;
            }
        }
        else{
            temp=head;
            if(head==NULL || one_result->index < head->index){
                one_result->next=head;
                head=one_result;
            }
            else{
                while(1){
                    if(temp->next==NULL){
                        temp->next=one_result;
                        one_result->next=NULL;
                        break;
                    }
                    if(temp->next->index>one_result->index){
                        one_result->next=temp->next;
                        temp->next=one_result;
                        break;
                    }
                    else{
                        temp=temp->next;
                    }
                }
            }
        }
    }
    pthread_mutex_unlock(&mutex_result);
    printf("%c", reserve[0]);
    printf("%c", reserve[1]);
    pthread_exit(NULL);
}


int main(int argc, char **argv){
    head_work = (Link*)malloc(sizeof(Link));
    tail_work = head_work;
    int opt, num_thread, i, num_file;
    num_thread = 1;

    // man page of getopt()
    extern char *optarg;
    extern int optind, opterr, optopt;
    while((opt = getopt(argc, argv, "j:")) != -1){
        switch(opt){
        case 'j':
            for(i=0;i<strlen(optarg);i++){
                if(!isdigit(optarg[i])){
                    fprintf(stderr, "Uasage: %s [-j thread_number] file file ...\n",argv[0]);
                    exit(EXIT_FAILURE);
                }    
            }
            num_thread = atoi(optarg);
            break;
        default:
            fprintf(stderr, "Usaage: %s [-j thread_number] file file ...\n",argv[0]);
            exit(EXIT_FAILURE);     
        }
    }
    if (optind >= argc) {
        fprintf(stderr, "Usagea: %s [-j thread_number] file file ...\n",argv[0]);
        exit(EXIT_FAILURE);
    }

    num_file = argc - optind;
    char **file_name = (char**)malloc((num_file+1)*sizeof(char *));
    for(i=0;i<num_file;i++){
        file_name[i] = (char*)malloc((int)(strlen(argv[optind+i])+1)*sizeof(char));
        strcpy(file_name[i], argv[optind+i]);
    }

    pthread_t *thread_id = (pthread_t*)malloc(num_thread*sizeof(pthread_t));
    for(i=0;i<num_thread;i++){
        pthread_t tid;  
        pthread_create(&tid, NULL, thread_work, NULL);
        thread_id[i] = tid;
    }
    pthread_t tid; 
    pthread_create(&tid, NULL, thread_result, NULL);

    //man page of mmap()
    char *addr;
    int fd;
    struct stat sb;
    off_t offset, pa_offset;
    size_t length_file;
    int index = 0;
    int length;
    unsigned long begin;
    for(i=0;i<num_file;i++){
        fd = open(file_name[i], O_RDONLY);
        if (fd == -1)
            handle_error("open");
        if (fstat(fd, &sb) == -1) // To obtain file size 
            handle_error("fstat");    

        pa_offset = 0 & ~(sysconf(_SC_PAGE_SIZE) - 1);
        length_file = sb.st_size;
        addr = mmap(NULL, length_file - pa_offset, PROT_READ, MAP_PRIVATE, fd, pa_offset);
        if (addr == MAP_FAILED)
                handle_error("mmap");
        for(begin=0;begin<length_file;begin+=4000){
            length = 4000;
            if (begin + length > length_file) length = length_file - begin;
            Link *one_chunk=(Link*)malloc(sizeof(Link));
            one_chunk->index = index++;
            one_chunk->content = addr + begin;
            one_chunk->length = length;
            pthread_mutex_lock(&mutex_work);
            one_chunk->next = NULL;
            tail_work->next = one_chunk;
            tail_work = tail_work->next;
            pthread_cond_signal(&work_not_empty);
            pthread_mutex_unlock(&mutex_work);
        }
        close(fd);
    }
    for(i=0;i<num_thread;i++){
        Link *empty_chunk=(Link*)malloc(sizeof(Link));
        empty_chunk->index = -1;
        pthread_mutex_lock(&mutex_work);
        empty_chunk->next = NULL;
        tail_work->next = empty_chunk;
        tail_work = tail_work->next;
        pthread_cond_signal(&work_not_empty);
        pthread_mutex_unlock(&mutex_work);
    }
    for(i=0;i<num_thread;i++)
        pthread_join(thread_id[i], NULL);

    pthread_mutex_lock(&mutex_result);
    not_end = 0;
    pthread_cond_signal(&result_not_empty);
    pthread_mutex_unlock(&mutex_result);
    pthread_join(tid, NULL);

}
