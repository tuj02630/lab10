#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#define MAX_NAME_SIZE 32
#define NUMBER_OF_QUESTIONS 11
#define MAX_RESPONSE_COUNT 233
#define bool int

typedef struct 
{
	int id;
	char full_name[ MAX_NAME_SIZE ];
	char responses[ NUMBER_OF_QUESTIONS ];
} bubble_response;

typedef struct {
	char * delim;
	unsigned int rows;
	unsigned int cols;
	char ** table;
} CSV;
 
 
/** 
 * Utility function to trim whitespaces from left & right of a string
 */
int trim(char ** str) {
	int trimmed;
	int n;
	int len;
 
	len = strlen(*str);
	n = len - 1;
	/* from right */
	while((n>=0) && isspace((*str)[n])) {
		(*str)[n] = '\0'; 
		trimmed += 1;
		n--;
	}
 
	/* from left */
	n = 0;
	while((n < len) && (isspace((*str)[0]))) {
		(*str)[0] = '\0'; 
		*str = (*str)+1;
		trimmed += 1;
		n++;
	}
	return trimmed;
} 
/** 
 * De-allocate csv structure 
 */
int csv_destroy(CSV * csv) {
	if (csv == NULL) { return 0; }
	if (csv->table != NULL) { free(csv->table); }
	if (csv->delim != NULL) { free(csv->delim); }
	free(csv);
	return 0;
}
/**
 * Allocate memory for a CSV structure 
 */
CSV * csv_create(unsigned int cols, unsigned int rows) {
	CSV * csv;
 
	csv = malloc(sizeof(CSV));
	csv->rows = rows;
	csv->cols = cols;
	csv->delim = strdup(",");
 
	csv->table = malloc(sizeof(char *) * cols * rows);
	if (csv->table == NULL) { goto error; }
 
	memset(csv->table, 0, sizeof(char *) * cols * rows);
 
	return csv;
 
error:
	csv_destroy(csv);
	return NULL;
}
/**
 * Get value in CSV table at COL, ROW
 */
char * csv_get(CSV * csv, unsigned int col, unsigned int row) {
	unsigned int idx;
	idx = col + (row * csv->cols);
	return csv->table[idx];
} 
/**
 * Set value in CSV table at COL, ROW
 */
int csv_set(CSV * csv, unsigned int col, unsigned int row, char * value) {
	unsigned int idx;
	idx = col + (row * csv->cols);
	csv->table[idx] = value;
	return 0;
} 
void csv_display(CSV * csv) {
	int row, col;
	char * content;
	if ((csv->rows == 0) || (csv->cols==0)) {
		printf("[Empty table]\n");
		return ;
	}
 
	printf("\n[Table cols=%d rows=%d]\n", csv->cols, csv->rows);
	for (row=0; row<csv->rows; row++) {
		printf("[|");
		for (col=0; col<csv->cols; col++) {
			content = csv_get(csv, col, row);
            printf("%s\t|", content);
		}
        printf("]\n");
	}
	printf("\n");
}
/**
* Resize CSV table
*/
int csv_resize(CSV * old_csv, unsigned int new_cols, unsigned int new_rows) {
	unsigned int cur_col, cur_row, max_cols, max_rows;
	CSV * new_csv;
	char * content;
	bool in_old, in_new;
 
	/* Build a new (fake) csv */
	new_csv = csv_create(new_cols, new_rows);
	if (new_csv == NULL) { goto error; }
 
	new_csv->rows = new_rows;
	new_csv->cols = new_cols;
 
 
	max_cols = (new_cols > old_csv->cols)? new_cols : old_csv->cols;
	max_rows = (new_rows > old_csv->rows)? new_rows : old_csv->rows;
 
	for (cur_col=0; cur_col<max_cols; cur_col++) {
		for (cur_row=0; cur_row<max_rows; cur_row++) {
			in_old = (cur_col < old_csv->cols) && (cur_row < old_csv->rows);
			in_new = (cur_col < new_csv->cols) && (cur_row < new_csv->rows);
 
			if (in_old && in_new) {
				/* re-link data */
				content = csv_get(old_csv, cur_col, cur_row);
				csv_set(new_csv, cur_col, cur_row, content);
			} else if (in_old) {
				/* destroy data */
				content = csv_get(old_csv, cur_col, cur_row);
				free(content);
			} else { /* skip */ }
		}
	}
	/* on rows */		
	free(old_csv->table);
	old_csv->rows = new_rows;
	old_csv->cols = new_cols;
	old_csv->table = new_csv->table;
	new_csv->table = NULL;
	csv_destroy(new_csv);
 
	return 0;
 
error:
	printf("Unable to resize CSV table: error %d - %s\n", errno, strerror(errno));
	return -1;
}
/**
 * Open CSV file and load its content into provided CSV structure
 **/
int csv_open(CSV * csv, char * filename) {
	FILE * fp;
	unsigned int m_rows;
	unsigned int m_cols, cols;
	char line[2048];
	char * lineptr;
	char * token;
 
 
	fp = fopen(filename, "r");
	if (fp == NULL) { goto error; }
 
	m_rows = 0;
	m_cols = 0;
	while(fgets(line, sizeof(line), fp) != NULL) {
 		m_rows += 1;
 		cols = 0;
 		lineptr = line;
 		while ((token = strtok(lineptr, csv->delim)) != NULL) {
 			lineptr = NULL;
 			trim(&token);
            cols += 1;
        	if (cols > m_cols) { m_cols = cols; }
            csv_resize(csv, m_cols, m_rows);
            csv_set(csv, cols-1, m_rows-1, strdup(token));
        }
	}
 
	fclose(fp);
	csv->rows = m_rows;
	csv->cols = m_cols;
	return 0;
 
error:
	fclose(fp);
	printf("Unable to open %s for reading.", filename);
	return -1;
}
/**
 * Open CSV file and save CSV structure content into it
 **/
int csv_save(CSV * csv, char * filename) {
	FILE * fp;
	int row, col;
	char * content;
 
	fp = fopen(filename, "w");
	for (row=0; row<csv->rows; row++) {
		for (col=0; col<csv->cols; col++) {
			content = csv_get(csv, col, row);
            fprintf(fp, "%s%s", content, 
            		((col == csv->cols-1) ? "" : csv->delim) );
		}
        fprintf(fp, "\n");
	}
 
	fclose(fp);
	return 0;
}
int promptUser()
{
	int result;
	printf("Bubble Sheet Results Data Mining Menu\n");
	printf("1. Display specific id\n");
	printf("2. Display specific name\n");
	printf("3. Display range of ids\n");
	printf("4. Display last name\n");
	printf("5. End the program\n");
	printf("\nEnter choice [1-5]:\t");
	scanf("%d", &result);
	return result;
}
int main ()
{
	int i = 0;
	int j = 0;
	int c, int d;
	CSV * csv = csv_create(0,0);
	csv_open(csv, "full_data.csv");
	bubble_response temp;
	bubble_response full_data[ MAX_RESPONSE_COUNT ];

	for(i = 0; i < MAX_RESPONSE_COUNT; i++)
	{
		full_data[i].id = *csv_get(csv, 0, i+1);	
		full_data[i].full_name = *csv_get(csv, 1, i+1);
	}
	//bubble sort
	for (c = 0 ; c < n - 1; c++)
  	{
    		for (d = 0 ; d < MAX_RESPONSE_COUNT - c - 1; d++)
    		{
      			if (full_data[d].id > full_data[d+1].id) /* For decreasing order use < */
      			{
        			temp = full_data[d];
        			full_data[d]   = full_data[d+1];
        			full_data[d+1] = temp;
      			}
		}
    	}
	int loop = 1;
	while(loop)
	{
		int choice = promptUser();
		switch(choice)
		{
			case 1:
				int tempID;
				printf("Input an Id.\n");
				scanf("%d", &tempID);
				for(i = 0; i < MAX_RESPONSE_COUNT; i++)
				{
					if(full_data[i].id == tempID)
					{
						printf("%5.5d, %s, %s", full_data[i].id, full_data[i].name, full_data[i].responses);
						break;
					}
				}
			case 2:
				for(i = 0; i < MAX_RESPONSE_COUNT; i++)
				{
					
				}
			case 3:
				
			case 4:
				
			case 5:
				loop = 0;
				break;
			default:
				printf("Sorry, I didn't catch that.\n");
		}
	}
	csv_destroy(csv);
}
