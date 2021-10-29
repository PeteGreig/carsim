#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdbool.h>
#include <sys/mman.h>
#include <sys/wait.h>

#define LP_LEN 6
#define SHMSZ 2920

//Car Variables
char plates[100][7];
char *car;
int time_stay;
int level;
int delay;
int entrance;
int car_exit;

//Shared Memory Variables
char c;
int shm_fd;
const char *key = "PARK";
char *shm, *s;

//Component Structs
typedef struct lpr_sensor lpr_sensor_t;
struct lpr_sensor {
	pthread_mutex_t m;
	pthread_cond_t c;
	char lp[LP_LEN];
};

typedef struct boomgate boomgate_t;
struct boomgate {
	pthread_mutex_t m;
	pthread_cond_t c;
	char s;
};

typedef struct parkingsign parkingsign_t;
struct parkingsign {
	pthread_mutex_t m;
	pthread_cond_t c;
	char display;
};

typedef struct temp_sensor temp_sensor_t;
struct temp_sensor {
	uint16_t temp;
};

typedef struct fire_alarm fire_alarm_t;
struct fire_alarm {
	uint8_t state;
};

//Shared Memory Struct
typedef struct shared_mem shared_mem_t;
struct shared_mem {
	/* Entrances */
	lpr_sensor_t entrance_sensor_one;
	boomgate_t entrance_gate_one;
	parkingsign_t parksign_one;
	
	lpr_sensor_t entrance_sensor_two;
	boomgate_t entrance_gate_two;
	parkingsign_t parksign_two;
	
	lpr_sensor_t entrance_sensor_three;
	boomgate_t entrance_gate_three;
	parkingsign_t parksign_three;
	
	lpr_sensor_t entrance_sensor_four;
	boomgate_t entrance_gate_four;
	parkingsign_t parksign_four;
	
	lpr_sensor_t entrance_sensor_five;
	boomgate_t entrance_gate_five;
	parkingsign_t parksign_five;
	
	/* Exits */
	lpr_sensor_t exit_sensor_one;
	boomgate_t exit_gate_one;
	
	lpr_sensor_t exit_sensor_two;
	boomgate_t exit_gate_two;
	
	lpr_sensor_t exit_sensor_three;
	boomgate_t exit_gate_three;
	
	lpr_sensor_t exit_sensor_four;
	boomgate_t exit_gate_four;
	
	lpr_sensor_t exit_sensor_five;
	boomgate_t exit_gate_five;
	
	/* Levels */	
	lpr_sensor_t level_sensor_one;
	temp_sensor_t temp_sensor_one;
	fire_alarm_t fire_alarm_one;
	
	lpr_sensor_t level_sensor_two;
	temp_sensor_t temp_sensor_two;
	fire_alarm_t fire_alarm_two;
	
	lpr_sensor_t level_sensor_three;
	temp_sensor_t temp_sensor_three;
	fire_alarm_t fire_alarm_three;
	
	lpr_sensor_t level_sensor_four;
	temp_sensor_t temp_sensor_four;
	fire_alarm_t fire_alarm_four;
	
	lpr_sensor_t level_sensor_five;
	temp_sensor_t temp_sensor_five;
	fire_alarm_t fire_alarm_five;
};

//Car Struct
typedef struct car car_t;

struct car {
    char* plate;
    int time_stay;
    int entrance;
    int level;
    int car_exit;
};

//Linked List of Cars
typedef struct node node_t;

struct node
{
    car_t *list_car;
    node_t *next;
};

//Entrances
node_t *e1head = NULL;
node_t *e1curr = NULL;

node_t *e2head = NULL;
node_t *e2curr = NULL;

node_t *e3head = NULL;
node_t *e3curr = NULL;

node_t *e4head = NULL;
node_t *e4curr = NULL;

node_t *e5head = NULL;
node_t *e5curr = NULL;

//Levels
node_t *l1head = NULL;
node_t *l1curr = NULL;

node_t *l2head = NULL;
node_t *l2curr = NULL;

node_t *l3head = NULL;
node_t *l3curr = NULL;

node_t *l4head = NULL;
node_t *l4curr = NULL;

node_t *l5head = NULL;
node_t *l5curr = NULL;

//Exits
node_t *x1head = NULL;
node_t *x1curr = NULL;

node_t *x2head = NULL;
node_t *x2curr = NULL;

node_t *x3head = NULL;
node_t *x3curr = NULL;

node_t *x4head = NULL;
node_t *x4curr = NULL;

node_t *x5head = NULL;
node_t *x5curr = NULL;

//Waiting Between Level and Exit
node_t *wxhead = NULL;
node_t *wxcurr = NULL;

shared_mem_t *shm_struct;

void create_shared_object() {

    shm_unlink(key);

	if ((shm_fd = shm_open(key, O_CREAT | O_RDWR, 0666)) < 0) {
        perror("shm_open");
        exit(1);
    }

    ftruncate(shm_fd, SHMSZ);

    if ((shm = mmap(0, SHMSZ, PROT_WRITE, MAP_SHARED, shm_fd, 0)) == (char *)-1)
    {
        perror("mmap");
        exit(1);
    }

    shm_struct = (shared_mem_t *)shm;
}

void get_plates() {
    char str1[7];
    int i = 0;
    int j = 0;
    FILE *fp;
    fp = fopen("plates.txt", "r");
    
    while(fscanf(fp, "%s", str1) == 1) {
        for(j = 0; j < 7; j++) {
            plates[i][j] = str1[j];
        } 
        i++;
    }

    fclose(fp);
}

char *rand_plate() {
    int r = rand() % 1000;
    char car2[sizeof(int)*3];
    sprintf(car2,"%03d", r);
    car = car2;

    int d = (rand() % 26) + 65;
    int e = (rand() % 26) + 65;
    int f = (rand() % 26) + 65;

    char str[sizeof(int)*3 + sizeof(char)*3];
    sprintf(str, "%s%c%c%c",car, d, e, f);

    char *p = (char *)malloc(strlen(str) ); // allocate enough space to hold the copy in p
    if (!p) { // malloc returns a NULL pointer when it fails
        puts("malloc failed.");
        exit(-1);
    }
    strcpy(p, str);

    return p;
}

char* assign_plate() {
    int r = rand() % 200;
    if (r < 100) {
        car = plates[r];
    }

    else {
        car = rand_plate();
    }
    return car;
}

int generate_time() {
    time_stay = ((rand() % 900) + 101);
    return time_stay;
}

int generate_entrance(){
    entrance = (rand()%5)+1;
    return entrance;
}

int generate_exit(){
    car_exit = (rand()%5)+1;
    return car_exit;
}

void node_add(car_t *car) {
    node_t *new_node = (node_t*)malloc(sizeof(node_t));
    new_node->list_car = car;
    new_node->next = NULL;

    node_t *head = NULL;
    node_t *curr = NULL;

    switch(car->entrance){
        case 1: head = e1head; curr = e1curr;
                break;
        case 2: head = e2head; curr = e2curr;
                break;
        case 3: head = e3head; curr = e3curr;
                break;
        case 4: head = e4head; curr = e4curr;
                break;
        case 5: head = e5head; curr = e5curr;
                break;
    }
    if (head == NULL){
        head = new_node;
        curr = head;
    }

    else{
        curr->next = new_node;
        curr = new_node;
    }
    switch(car->entrance){
        case 1: e1head = head; e1curr = curr;
                break;
        case 2: e2head = head; e2curr = curr;
                break;
        case 3: e3head = head; e3curr = curr;
                break;
        case 4: e4head = head; e4curr = curr;
                break;
        case 5: e5head = head; e5curr = curr;
                break;
    }
    return;
}

void generate_car(){
    car_t *car = (car_t *)malloc(sizeof(car_t));
    car->plate = assign_plate();
    car->time_stay = generate_time();
    car->entrance = generate_entrance();
    car->level = 0;
    car->car_exit = generate_exit();
    node_add(car);
    delay = ((rand()%99000)+1000);
    usleep(delay);
}

void node_print(node_t *head)
{
    for (; head != NULL; head = head->next)
    {
        printf("car = %s\ntime = %d\nentrance = %d\nlevel = %d\n",head->list_car->plate, head->list_car->time_stay, head->list_car->entrance, head->list_car->level);
    }
}

node_t* pop_car(node_t *head) {
    head = head->next;
    return head;
}

node_t *delete_car(node_t *head) {
    node_t* prev;
    node_t* curr = head;

    for (; curr != NULL; curr = curr->next) {
        if (curr->list_car->plate == NULL) {
            if (curr == head){
                node_t* temp = head->next;
                free(head);
                return temp;
            }

            else {
                prev->next = curr->next;
                free(curr);
                return head;
            }
        }
        prev = curr;
    }
    return head;
}

bool enter_plate(node_t *head, lpr_sensor_t *lpr, parkingsign_t *sign){
    if (head == NULL){
        return false;
    }
    strcpy(lpr->lp, head->list_car->plate);
    int sign_conv;
    sign_conv = sign->display;

    if (1 <= sign_conv && 5>= sign_conv) {
        head->list_car->level = sign_conv;
        return true;
    }
    return false;
}

void exit_plate(car_t *car, lpr_sensor_t *lpr){
    strcpy(lpr->lp, car->plate);
}

node_t *get_level(node_t *entrance_head, lpr_sensor_t *lpr, parkingsign_t *sign) {
    if (entrance_head == NULL){
        return entrance_head;
    }

    if (enter_plate(entrance_head, lpr, sign)){
        node_t *new_node = (node_t*)malloc(sizeof(node_t));
        new_node->list_car = entrance_head->list_car;
        new_node->next = NULL;
    
        node_t *head = NULL;
        node_t *curr = NULL;

        switch(entrance_head->list_car->level){
            case 1: head = l1head; curr = l1curr;
                    break;
            case 2: head = l2head; curr = l2curr;
                    break;
            case 3: head = l3head; curr = l3curr;
                    break;
            case 4: head = l4head; curr = l4curr;
                    break;
            case 5: head = l5head; curr = l5curr;
                    break;
            }
        if (head == NULL){
            head = new_node;
            curr = head;
        }

        else{
            curr->next = new_node;
            curr = new_node;
        }
        switch(entrance_head->list_car->level){
            case 1: l1head = head; l1curr = curr;
                    break;
            case 2: l2head = head; l2curr = curr;
                    break;
            case 3: l3head = head; l3curr = curr;
                    break;
            case 4: l4head = head; l4curr = curr;
                    break;
            case 5: l5head = head; l5curr = curr;
                    break;
        }
        entrance_head = pop_car(entrance_head);
        return entrance_head;
    }
    else {
        entrance_head->list_car->plate = NULL;
        entrance_head = delete_car(entrance_head);
        return entrance_head;
    }
    return entrance_head;
}

node_t *check_level(node_t *level) {
    if (level == NULL){
        return level;
    }
    node_t* lprev;
    node_t* lcurr = level;

    node_t* head = NULL;
    node_t* curr = NULL;

    for (; lcurr != NULL; lcurr = lcurr->next) {
        if (lcurr->list_car->time_stay <= 0) {
            node_t *new_node = (node_t*)malloc(sizeof(node_t));
            head = wxhead;
            curr = wxcurr;
            if (head == NULL){
                head = new_node;
                curr = head;
            }

            else{
                curr->next = new_node;
                curr = new_node;
            }

            wxhead = head;
            wxcurr = curr;

            if (lcurr == level){
                node_t* temp = level->next;
                return temp;
            }

            else {
                lprev->next = lcurr->next;
                level->list_car->time_stay = level->list_car->time_stay - 1;
                return level;
            }
        }
        lprev = lcurr;
    }
    return level;
}

node_t *wait_to_exit(node_t *waiting_head){
    if (waiting_head == NULL){
        return waiting_head;
    }
    node_t* wxprev;
    node_t* wxcurr = waiting_head;

    node_t* head = NULL;
    node_t* curr = NULL;

    for (; wxcurr != NULL; wxcurr = wxcurr->next) {
        if (wxcurr->list_car->time_stay >= 10) {
            node_t *new_node = (node_t*)malloc(sizeof(node_t));
            switch(waiting_head->list_car->car_exit){
                case 1: head = x1head; curr = x1curr;
                        break;
                case 2: head = x2head; curr = x2curr;
                        break;
                case 3: head = x3head; curr = x3curr;
                        break;
                case 4: head = x4head; curr = x4curr;
                        break;
                case 5: head = x5head; curr = x5curr;
                        break;
                }
            if (head == NULL){
                head = new_node;
                curr = head;
            }

            else{
                curr->next = new_node;
                curr = new_node;
            }
            switch(waiting_head->list_car->car_exit){
                case 1: x1head = head; x1curr = curr;
                        break;
                case 2: x2head = head; x2curr = curr;
                        break;
                case 3: x3head = head; x3curr = curr;
                        break;
                case 4: x4head = head; x4curr = curr;
                        break;
                case 5: x5head = head; x5curr = curr;
                        break;
            }

            if (wxcurr == waiting_head){
                node_t* temp = waiting_head->next;
                return temp;
            }

            else {
                wxprev->next = wxcurr->next;
                waiting_head->list_car->time_stay = waiting_head->list_car->time_stay + 1;
                return waiting_head;
            }
        }
        wxprev = wxcurr;
    }
    return waiting_head;
}

void leave_carpark(car_t *car, lpr_sensor_t *lpr, node_t *exit_head) {
    if (exit_head == NULL){
        return;
    }

    exit_plate(car, lpr);
    
    //WAIT FOR SIGNAL THAT BOOM GATE IS OPEN

    car->plate = NULL;
    exit_head = delete_car(exit_head);
}



int main() {
    create_shared_object();
    get_plates();
    shm_struct->parksign_one.display = 5;
    shm_struct->parksign_two.display = 4;
    shm_struct->parksign_three.display = 3;
    shm_struct->parksign_four.display = 2;
    shm_struct->parksign_five.display = 1;
    int i;
    for(i = 0; i<10; i++){
        generate_car();
        e1head = get_level(e1head, &shm_struct->entrance_sensor_one, &shm_struct->parksign_one);
        e2head = get_level(e2head, &shm_struct->entrance_sensor_two, &shm_struct->parksign_two);
        e3head = get_level(e3head, &shm_struct->entrance_sensor_three, &shm_struct->parksign_three);
        e4head = get_level(e4head, &shm_struct->entrance_sensor_four, &shm_struct->parksign_four);
        e5head = get_level(e5head, &shm_struct->entrance_sensor_five, &shm_struct->parksign_five);
    }
    node_print(e2head);

    printf("lp1 = %d\n",shm_struct->parksign_one.display);
    printf("lp1 = %d\n",shm_struct->parksign_two.display);
    printf("lp1 = %d\n",shm_struct->parksign_three.display);
    printf("lp1 = %d\n",shm_struct->parksign_four.display);
    printf("lp1 = %d\n",shm_struct->parksign_five.display);


    node_print(e1head);
    node_print(e4head);
    node_print(e3head);
    node_print(e5head);
    
    printf("lp1 = %s\n",shm_struct->entrance_sensor_three.lp);

    node_print(l1head);
    node_print(l2head);
    node_print(l3head);
    node_print(l4head);
    node_print(l5head);
	return 0;
}