/* Memory manager
 *
 * Skeleton program written by Ben Rubinstein, May 2014
 *
 * Modifications by Manan Ahuja (655959), May 2014
 *
 * Algorithms are fun!
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <assert.h>

#define TOTALMEM	1048576
#define MAXVARS		1024
#define ERROR		-1
#define SUCCESS		1
#define LINELEN		5000
#define MAXLINES	100000
#define INPUT_INTS	'd'
#define INPUT_CHARS	'c'
#define FREE_DATA	'f'
#define INT_DELIM_S	","
#define INT_DELIM_C	','

typedef struct {
	char memory[TOTALMEM];	/* TOTALMEM bytes of memory */
	void *null;		/* first address will be  unusable */
	void *vars[MAXVARS];	/* MAXVARS variables, each at an address */
	size_t var_sizes[MAXVARS];	/* number of bytes per variable */
} mmanager_t;

mmanager_t manager;

/****************************************************************/

/* function prototypes */
int read_line(char *line, int maxlen);
void process_input_char(char *line, char *commands, void *stored[],
			int *storeLen, int numCommands);
void process_input_int(char *line, char *commands, void *stored[],
		       int *storeLen, int numCommands);
void process_free(char *line, char *commands, void *stored[],
		  int *storeLen, int numCommands);
int count_char(char c, char *s);
int parse_integers(char *str, char *delim, int results[], int max_results);
void print_ints(int *intArray, size_t size);
void print_chars(char *charArray);
void print_memory(char isInt[MAXVARS]);
void *mm_malloc(size_t size);
void mm_free(void *ptr);
int is_vacant(void *first, void *last); 
void *select_address(size_t size); 
int select_var(void); 
void core_dump(char *filename_mem, char *filename_vars); 

/****************************************************************/

/* orchestrate the entire program
 */
int
main(int argc, char *argv[]) {
	char line[LINELEN+1];
	char cmd[MAXLINES];
	void *stored[MAXLINES];
	int storeLen[MAXLINES];
	int i, numCmd = 0;

	/* initialise our very own NULL */
	manager.null = manager.memory;

	/* process input commands that make use of memory management */
	while (numCmd<MAXLINES && read_line(line,LINELEN)) {
		if (strlen(line) < 2) {
			fprintf(stderr, "Invalid line %s\n", line);
			return EXIT_FAILURE;
		}
		if (line[0] == INPUT_CHARS) {
			process_input_char(line, cmd, stored, storeLen, numCmd);
		} else if (line[0] == INPUT_INTS) {
			process_input_int(line, cmd, stored, storeLen, numCmd);
		} else if (line[0] == FREE_DATA) {
			process_free(line, cmd, stored, storeLen, numCmd);
		} else {
			fprintf(stderr, "Invalid input %c.\n", line[0]);
			return EXIT_FAILURE;
		}
		numCmd++;
	}

	/* print out what we are left with
	 * after creating variables, deleting some, creating more, ...
	 */
	printf("Cmd#\tOffset\tValue\n");
	printf("====\t======\t=====\n");
	for (i=0; i<numCmd; i++) {
		if (storeLen[i] > 0) {
			printf("%d\t%d\t", i, 
				(int)((char*)stored[i]-manager.memory));
			if (cmd[i] == INPUT_CHARS) {
				print_chars((char*)stored[i]);
			} else {
				print_ints((int*)stored[i], storeLen[i]);
			}
		}
	}
	/* write a copy of memory to disk */
	core_dump("core_mem", "core_vars"); 

	return 0;
}

/****************************************************************/

/* read in a line of input
 */
int
read_line(char *line, int maxlen) {
	int n = 0;
	int oversize = 0;
	int c;
	while (((c=getchar())!=EOF) && (c!='\n')) {
		if (n < maxlen) {
			line[n++] = c;
		}
		else {
			oversize++;
		}
	}
	line[n] = '\0';
	if (oversize > 0) {
		fprintf(stderr, "Warning! %d over limit. Line truncated.\n",
		        oversize);
	}
	return ((n>0) || (c!=EOF));
}

/****************************************************************/

/* process an input-char command from stdin by storing the string
 */
void
process_input_char(char *line, char *commands, void *stored[], 
		   int *storeLen, int numCommands) {
	size_t lineLen = strlen(line);
	commands[numCommands] = line[0];
	stored[numCommands] = mm_malloc(lineLen);
	assert(stored[numCommands] != manager.null);
	strcpy(stored[numCommands], line+1);
	storeLen[numCommands] = lineLen;
}

/****************************************************************/

/* process an input-int command from stdin by storing the ints
 */
void
process_input_int(char *line, char *commands, void *stored[], 
		  int *storeLen, int numCommands) {
	int intsLen = count_char(INT_DELIM_C, line+1) + 1;
	size_t size = sizeof(intsLen) * intsLen;
	commands[numCommands] = line[0];
	stored[numCommands] = mm_malloc(size);
	assert(stored[numCommands] != manager.null);
	parse_integers(line+1, INT_DELIM_S, stored[numCommands], intsLen);
	storeLen[numCommands] = intsLen;
}

/****************************************************************/

/* process a free command from stdin
 */
void
process_free(char *line, char *commands, void *stored[], 
	     int *storeLen, int numCommands) {
	int line_index;
	void *ptr;
	commands[numCommands] = line[0];
	stored[numCommands] = manager.null;
	storeLen[numCommands] = 0;
	/*get line index by converting string to int*/
	line_index = atoi(&(line[1])) - 1;

	if ((line_index < 0) || (line_index > numCommands)){
		fprintf(stderr, "Invalid input %c.\n", line[0]);
		return;
	}else{
		ptr = stored[line_index];
		mm_free(ptr);
		stored[line_index] = manager.null;
		storeLen[line_index] = 0;
	}
}


/****************************************************************/
/* if the pointer that is passed as argument to this function exists
in the manager.vars array, then it is put equal to manager.null 
and its size is put equal to 0. This is equivalent to freeing
memmory earlier used by the object that the pointer 
was pointing at. */

void mm_free(void *ptr){
	int i, found = 0;
	
	assert(ptr != manager.null);
	for(i=0;i<MAXVARS;i++){
		if((manager.vars[i]) == ptr){
			found = 1;
			break;
		}
	}
	if(!found){
		return;
	}else{
		manager.vars[i] = manager.null;
		manager.var_sizes[i] = 0;
		ptr = manager.null;
	}
}

/***************************************************************/


/* Count the number of occurences of a char in a string
 */
int
count_char(char c, char *s) {
	int count = 0;
	if (!s) {
		return count;
	}
	while (*s != '\0') {
		if (*(s++) == c) {
			count++;
		}
	}
	return count;
}

/****************************************************************/

/* parse string for a delimited-list of positive integers.
 * Returns number of ints parsed. If invalid input is detected
 * or if more than max_results ints are parsed, execution
 * will halt. Note! str will be modified as a side-effect of
 * running this function: delims replaced by \0
 */
int
parse_integers(char *str, char *delim, int results[], int max_results) {
	int num_results = 0;
	int num;
	char *token;
	token = strtok(str, delim);
	while (token != NULL) {
		if ((num=atoi(token)) <= 0) {
			fprintf(stderr, "Non-int %s.\n", token);
			exit(EXIT_FAILURE);
		}
		if (num_results >= max_results) {
			fprintf(stderr, "Parsing too many ints.\n");
			exit(EXIT_FAILURE);
		}
		results[num_results++] = num;
		token = strtok(NULL, delim);
	}
	return num_results;
}

/****************************************************************/

/* print an array of ints
 */
void
print_ints(int *intArray, size_t size) {
	int i;
	assert(size > 0);
	printf("ints: %d", intArray[0]);
	for (i=1; i<size; i++) {
		printf(", %d", intArray[i]);
	}
	putchar('\n');
}

/****************************************************************/

/* print an array of chars
 */
void
print_chars(char *charArray) {
	printf("chars: %s\n", charArray);
}

/****************************************************************/

/* prints what is stored in memory given an array specifying
 * how previously allocated variables should be interpreted
 * (as int or char arrays)
 */
void
print_memory(char isInt[MAXVARS]) {
	void *start;
	int i, num_bytes;
	for (i=0; i<MAXVARS && manager.var_sizes[i]>0; i++) {
		num_bytes = manager.var_sizes[i];
		start = manager.vars[i];
		if (isInt[i]) {
			print_ints((int*)start, num_bytes/sizeof(i));
		} else {
			print_chars((char*)start);
		}
	}
}

/*****************************************************************/
/* following function takes 2 addresses - first and last and 
* returns a logical int indicating whether the range is completely
* available or not. var_firsts are accessed from manager.vars and 
* var_lasts are obtained by adding (size-1) to those values. Then all 
* values from var_first to var_last are checked to see if they are in
* the range from first to last. If they are, then the range being 
* checked is not vacant and 0 is returned. Else, 1 is returned. 
*/
int 
is_vacant(void *first, void *last){
	int i, size, vacant = 1;
	char *var_first, *var_last, *j;

	for (i=0; i<MAXVARS; i++){
		if((manager.vars[i] != manager.null) && 
			(manager.var_sizes[i] > 0)){

			var_first = (char *)manager.vars[i];
			size = manager.var_sizes[i];
			var_last = var_first + (size - 1);

			for(j=var_first; j<=var_last; j++){
				if((j>=(char *)first) && (j<=(char *)last)){
					vacant = 0;
				}
			}
		}
	}
	return vacant;
}

/*****************************************************************/
/* returns a start address pointing to a contiguous block of 
*  available memory of size bytes ending within manager.memory. 
*/
void *
select_address(size_t size){
	char *min, *max, *i;
	/* min and max values of addresses can be the ones pointed by
	the 2nd object and last object in manager.memory respectively. */

	min = &(manager.memory[0]) + 1;
	max = &(manager.memory[0]) + (TOTALMEM - 1);
	for(i=min;i<=max;i++){
		if(is_vacant(i, i + (size-1))){
			return i;
		}
	}
	return manager.null;
}
/*****************************************************************/
/* returns the earliest index into manager.vars to a variable slot
*  that is not currently assigned. A slot is assigned if its address
*  is not equal to manager.null and its size is greater than 0. 
*/
int 
select_var(void){
	int i;
	for(i=0;i<MAXVARS;i++){
		if(!((manager.vars[i] != manager.null) && 
			(manager.var_sizes[i] > 0))){
			return i;
		}
	}
	return ERROR; 
} 

/****************************************************************/
/* returns a valid address within manager.memory that points to an 
*  available block of size bytes. In doing so, it is actually making this
*  memory available, hence it updates manager.vars and manager.vars_sizes
*  accordingly to make sure the memory is allocated. 
*/
void *
mm_malloc(size_t size) {
	void *address;
	int index;

	address = select_address(size);
	index = select_var();

	if((address != manager.null) && (index != ERROR)){
		manager.var_sizes[index] = size;
		manager.vars[index] = address;
		return address;	
		}
		return manager.null;
}

/****************************************************************/
/* writes contents of manager.memory to binary file filename_mem
*  and variables' integer offsets and sizes to another file with name
*  filename_vars.
*/
void 
core_dump(char *filename_mem, char *filename_vars){
	FILE *fp_mem, *fp_vars;
	int i, offset;

	fp_mem = fopen(filename_mem, "w");
	assert(fp_mem != NULL);
	fwrite(manager.memory, sizeof(*manager.memory), TOTALMEM, fp_mem);
	fclose(fp_mem);

	fp_vars = fopen(filename_vars, "w");
	assert(fp_vars != NULL);
	for(i=0;i< MAXVARS;i++){
		if (manager.var_sizes[i] != 0){
			offset = (int)((char *)manager.vars[i] - 
				  manager.memory);
			fprintf(fp_vars, "%d\t%d\n", offset, 
				(int)manager.var_sizes[i]);
		}
	}
	fclose(fp_vars);
}



